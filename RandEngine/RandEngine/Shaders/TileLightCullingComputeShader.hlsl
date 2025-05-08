// 변경 요약:
// - CullLightsForTile 병렬화
// - Heatmap 렌더링 분산
// - 조기 종료 및 unroll 최적화

#include "ComputeDefine.hlsl"

StructuredBuffer<FPointLightGPU>    PointLightBuffer    : register(t0);
StructuredBuffer<FSpotLightGPU>     SpotLightBuffer     : register(t2);

Texture2D<float>                    gDepthTexture       : register(t1);

RWStructuredBuffer<uint>    PerTilePointLightIndexMaskOut   : register(u0);
RWStructuredBuffer<uint>    PerTileSpotLightIndexMaskOut    : register(u1);
RWStructuredBuffer<uint>    CulledPointLightIndexMaskOUT    : register(u2);
RWStructuredBuffer<uint>    CulledSpotLightIndexMaskOUT     : register(u3);
RWTexture2D<float4>         DebugHeatmap                    : register(u4);

groupshared uint tileDepthMask;
groupshared uint groupMinZ;
groupshared uint groupMaxZ;
groupshared uint hitCount;

bool ShouldLightAffectTile(float3 lightVSPos, float radius, float minZ, float maxZ, uint depthMask)
{
    if (depthMask == 0)
        return false;
    float s_minDepth = lightVSPos.z - radius;
    float s_maxDepth = lightVSPos.z + radius;
    if (s_maxDepth < minZ || s_minDepth > maxZ)
        return false;
    float normMin = saturate((s_minDepth - minZ) / max(1e-5, maxZ - minZ));
    float normMax = saturate((s_maxDepth - minZ) / max(1e-5, maxZ - minZ));
    int sliceMin = clamp((int) floor(normMin * NUM_SLICES), 0, NUM_SLICES - 1);
    int sliceMax = clamp((int) ceil(normMax * NUM_SLICES), 0, NUM_SLICES - 1);
    uint sphereMask = 0;
    [unroll]
    for (int i = sliceMin; i <= sliceMax; ++i)
        sphereMask |= (1u << i);
    return (sphereMask & depthMask) != 0;
}

void CullLight(uint index, float3 lightVSPos, float radius, Frustum frustum, float minZ, float maxZ, uint flatTileIndex, RWStructuredBuffer<uint> MaskBuffer, RWStructuredBuffer<uint> CulledMaskBuffer)
{
    Sphere s = { lightVSPos, radius };
    if (!SphereInsideFrustum(s, frustum, NearZ, FarZ))
        return;
    if (Enable25DCulling != 0 && !ShouldLightAffectTile(lightVSPos, radius, minZ, maxZ, tileDepthMask))
        return;
    uint bucketIdx = index / 32;
    uint bitIdx = index % 32;
    InterlockedOr(MaskBuffer[flatTileIndex * SHADER_ENTITY_TILE_BUCKET_COUNT + bucketIdx], 1 << bitIdx);
    InterlockedAdd(hitCount, 1);
    InterlockedOr(CulledMaskBuffer[bucketIdx], 1 << (index % 32));
}

void WriteHeatmap(uint2 tileCoord, uint threadFlatIndex)
{
    const float3 heatmap[] =
    {
        float3(0, 0, 0), float3(0, 0, 1), float3(0, 1, 1),
        float3(0, 1, 0), float3(1, 1, 0), float3(1, 0, 0)
    };
    const float maxHeat = 50.0f;
    float4 result = float4(0, 0, 0, 0);
    if (hitCount > 0)
    {
        float l = saturate(hitCount / maxHeat) * 5;
        float3 c1 = heatmap[floor(l)];
        float3 c2 = heatmap[ceil(l)];
        result = float4(lerp(c1, c2, frac(l)), 0.8f);
    }
    uint2 local = unflatten2D(threadFlatIndex, uint2(TILE_SIZE, TILE_SIZE));
    uint2 pixel = tileCoord * TILE_SIZE + local;
    if (all(pixel < ScreenSize))
        DebugHeatmap[pixel] = result;
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void mainCS(uint3 groupID : SV_GroupID, uint3 dispatchID : SV_DispatchThreadID, uint3 threadID : SV_GroupThreadID)
{
    uint2 tileCoord = groupID.xy;
    uint2 pixel = tileCoord * TILE_SIZE + threadID.xy;

    float minZ = NearZ;
    float maxZ = FarZ;

    if (threadID.x == 0 && threadID.y == 0)
    {
        tileDepthMask = 0;
        groupMinZ = 0x7f7fffff;
        groupMaxZ = 0x00000000;
    }
    hitCount = 0;
    GroupMemoryBarrierWithGroupSync();

    float depthSample = 1.0f;
    float linearZ = FarZ;
    if (Enable25DCulling != 0 && all(pixel < ScreenSize))
    {
        depthSample = gDepthTexture[pixel];
        if (depthSample < 1.0f)
        {
            linearZ = (NearZ * FarZ) / (FarZ - depthSample * (FarZ - NearZ));
            InterlockedMin(groupMinZ, uint(linearZ));
            InterlockedMax(groupMaxZ, uint(linearZ));
        }
    }
    GroupMemoryBarrierWithGroupSync();

    if (Enable25DCulling != 0 && groupMaxZ > groupMinZ)
    {
        minZ = float(groupMinZ);
        maxZ = float(groupMaxZ);
    }

    if (Enable25DCulling != 0 && depthSample < 1.0f)
    {
        float rangeZ = maxZ - minZ;
        if (rangeZ < 1e-1)
        {
            minZ = NearZ;
            maxZ = FarZ;
            rangeZ = maxZ - minZ;
        }
        float sliceNormZ = saturate((linearZ - minZ) / rangeZ);
        int sliceIndex = clamp((int) floor(sliceNormZ * NUM_SLICES), 0, NUM_SLICES - 1);
        InterlockedOr(tileDepthMask, (1u << sliceIndex));
    }
    GroupMemoryBarrierWithGroupSync();

    float2 tileMin = tileCoord * TileSize;
    float2 tileMax = tileMin + TileSize;
    float3 viewCorners[8];
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        float2 uv = float2((i & 1) ? tileMax.x : tileMin.x, (i & 2) ? tileMax.y : tileMin.y) / ScreenSize;
        uv.y = 1.0 - uv.y;
        float4 clipNear = float4(uv * 2.0 - 1.0, NearZ, 1.0);
        float4 clipFar = float4(uv * 2.0 - 1.0, FarZ, 1.0);
        float4 viewNear = mul(clipNear, InverseProjection);
        float4 viewFar = mul(clipFar, InverseProjection);
        viewCorners[i + 0] = viewNear.xyz / viewNear.w;
        viewCorners[i + 4] = viewFar.xyz / viewFar.w;
    }

    Frustum frustum;
    frustum.planes[0] = ComputePlane(viewCorners[0], viewCorners[2], viewCorners[6]);
    frustum.planes[1] = ComputePlane(viewCorners[3], viewCorners[1], viewCorners[7]);
    frustum.planes[2] = ComputePlane(viewCorners[1], viewCorners[0], viewCorners[5]);
    frustum.planes[3] = ComputePlane(viewCorners[2], viewCorners[3], viewCorners[6]);

    uint flatTileIndex = tileCoord.y * (ScreenSize.x / TileSize.x) + tileCoord.x;
    uint threadFlatIndex = threadID.y * TILE_SIZE + threadID.x;
    uint totalThreads = TILE_SIZE * TILE_SIZE;

    for (uint i = threadFlatIndex; i < NumPointLights; i += totalThreads)
    {
        float3 lightVSPos = mul(float4(PointLightBuffer[i].Position, 1), View).xyz;
        CullLight(i, lightVSPos, PointLightBuffer[i].Radius, frustum, minZ, maxZ, flatTileIndex, PerTilePointLightIndexMaskOut, CulledPointLightIndexMaskOUT);
    }
    for (uint j = threadFlatIndex; j < NumSpotLights; j += totalThreads)
    {
        float3 lightVSPos = mul(float4(SpotLightBuffer[j].Position, 1), View).xyz;
        CullLight(j, lightVSPos, SpotLightBuffer[j].Radius, frustum, minZ, maxZ, flatTileIndex, PerTileSpotLightIndexMaskOut, CulledSpotLightIndexMaskOUT);
    }

    GroupMemoryBarrierWithGroupSync();
    WriteHeatmap(tileCoord, threadFlatIndex);
}
