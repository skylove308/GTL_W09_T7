#include "ShaderRegisters.hlsl"

#ifdef LIGHTING_MODEL_GOURAUD
SamplerState DiffuseSampler : register(s0);

Texture2D DiffuseTexture : register(t0);

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

#include "Light.hlsl"
#endif

#define MAX_BONE_INFLUENCES 4
#define MAX_BONES 120

struct VS_INPUT_SkeletalMesh
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    uint MaterialIndex : MATERIAL_INDEX;
    uint4    BoneIndices    : BONEINDICES;
    float4   BoneWeights    : BONEWEIGHTS;
};
cbuffer FBonesConstants : register(b2)
{
    row_major float4x4 BoneMatrices[MAX_BONES];
};

PS_INPUT_StaticMesh mainVS(VS_INPUT_SkeletalMesh Input) 
{
    PS_INPUT_StaticMesh Output;

    // 스키닝 전
    float4 skinnedPos     = float4(0,0,0,0);
    float3 skinnedNormal  = float3(0,0,0);
    float3 skinnedTangent = float3(0,0,0);

    // 1) 가중치 합 계산
    float weightSum = dot(Input.BoneWeights, float4(1,1,1,1));

    // 2) (선택) 가중치 정규화 벡터
    float4 normWeights = (weightSum > 0) 
        ? Input.BoneWeights / weightSum 
        : Input.BoneWeights;  // weightSum==0 이면 그대로

    // 3) 스키닝 루프: normWeights 사용
    [unroll]
    for (int i = 0; i < MAX_BONE_INFLUENCES; ++i)
    {
        uint idx = Input.BoneIndices[i];
        float w  = normWeights[i];  // 정규화된 가중치 사용

        if (w > 0 && idx < MAX_BONES)
        {
            float4x4 m = BoneMatrices[idx];
            skinnedPos     += mul(float4(Input.Position, 1.0), m) * w;
            skinnedNormal  += mul(Input.Normal,   (float3x3)m) * w;
            skinnedTangent += mul(Input.Tangent.xyz, (float3x3)m) * w;
        }
    }

    // 스키닝 결과 정규화
    skinnedNormal = normalize(skinnedNormal);
    skinnedTangent = normalize(skinnedTangent - skinnedNormal * dot(skinnedNormal, skinnedTangent));
    // -----------------------

    // 월드 변환
    float4 worldPos = mul(skinnedPos, WorldMatrix);
    Output.WorldPosition = worldPos.xyz;

    // 뷰-프로젝션
    Output.Position = mul(worldPos, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);

    // 노멀, 탄젠트, UV, 색상
    Output.WorldNormal  = mul(skinnedNormal, (float3x3)InverseTransposedWorld);
    Output.WorldTangent = float4(skinnedTangent, Input.Tangent.w);
    Output.UV           = Input.UV;
    Output.MaterialIndex = Input.MaterialIndex;

#ifdef LIGHTING_MODEL_GOURAUD
    float3 DiffuseColor = Input.Color;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        DiffuseColor = DiffuseTexture.SampleLevel(DiffuseSampler, Input.UV, 0).rgb;
    }
    float3 Diffuse = Lighting(Output.WorldPosition, Output.WorldNormal, ViewWorldLocation, DiffuseColor, Material.SpecularColor, Material.Shininess);
    Output.Color = float4(Diffuse.rgb, 1.0);
#else
    Output.Color = Input.Color;
#endif

    return Output;
}
