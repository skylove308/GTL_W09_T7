// 상수 정의
#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 1024
#define SHADER_ENTITY_TILE_BUCKET_COUNT (MAX_LIGHTS_PER_TILE / 32)
#define THREAD_GROUP_SIZE 8
#define NUM_SLICES 32    // 타일 내 depth를 32개의 슬라이스로 분할 (시그래프 Harada 값과 동일)

cbuffer TileLightCullSettings : register(b0)
{
    uint2 ScreenSize; // 화면 해상도
    uint2 TileSize; // 한 타일의 크기 (예: 16x16)

    float NearZ; // 카메라 near plane
    float FarZ; // 카메라 far plane

    row_major matrix View; // View 행렬
    row_major matrix Projection; // Projection 행렬
    row_major matrix InverseProjection; // Projection^-1, 뷰스페이스 복원용

    uint NumPointLights; // 총 라이트 수
    uint NumSpotLights; // 총 라이트 수
    uint Enable25DCulling; // 1이면 2.5D 컬링 사용
}

struct FPointLightGPU
{
    float3 Position;
    float Radius;
    float3 Direction; // 사용하지 않음
    uint isPointLight;
};

struct FSpotLightGPU
{
    float3 Position;
    float Radius;
    float3 Direction;
    float AngleDeg;
};


struct Sphere
{
    float3 c; // Center point.
    float r; // Radius.
};
struct Plane
{
    float3 N; // Plane normal.
    float d; // Distance to origin.
};
struct Frustum
{
    Plane planes[4]; // left, right, top, bottom frustum planes.
};

Plane ComputePlane(float3 p0, float3 p1, float3 p2)
{
    Plane plane;

    float3 v0 = p1 - p0;
    float3 v2 = p2 - p0;

    plane.N = normalize(cross(v0, v2));

	// Compute the distance to the origin using p0.
    plane.d = dot(plane.N, p0);

    return plane;
}

// Convert clip space coordinates to view space
float4 ClipToView(float4 clip)
{
	// View space position.
    float4 view = mul(clip, InverseProjection);
	// Perspective projection.
    view = view / view.w;

    return view;
}
// Convert screen space coordinates to view space.
float4 ScreenToView(float4 screen, float2 dim_rcp)
{
	// Convert to normalized texture coordinates // Convert to clip space
    float2 texCoord = screen.xy * dim_rcp;
    float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);
    return ClipToView(clip);
}

bool SphereInsidePlane(Sphere sphere, Plane plane)
{
    return dot(plane.N, sphere.c) - plane.d < -sphere.r;
}
bool SphereInsideFrustum(Sphere sphere, Frustum frustum, float zNear, float zFar) // this can only be used in view space
{
    bool result = true;
    result = ((sphere.c.z + sphere.r < zNear || sphere.c.z - sphere.r > zFar) ? false : result);
    result = ((SphereInsidePlane(sphere, frustum.planes[0])) ? false : result);
    result = ((SphereInsidePlane(sphere, frustum.planes[1])) ? false : result);
    result = ((SphereInsidePlane(sphere, frustum.planes[2])) ? false : result);
    result = ((SphereInsidePlane(sphere, frustum.planes[3])) ? false : result);

    return result;
}

inline uint2 unflatten2D(uint idx, uint2 dim)
{
    return uint2(idx % dim.x, idx / dim.x);
}

struct Spotlight
{
    float3 position; // 빛의 위치 (view space)
    float range; // 빛의 최대 도달 거리 (cone 높이)
    float3 direction; // 정규화된 빛의 방향 벡터
    float angle; // cone 기준 outer angle (radian)
};

struct AABB
{
    float3 center; // 뷰 공간에서 AABB 중심
    float3 extents; // 반 너비 (extent) 벡터
};

// SpotLightvsAABB 함수 
bool SpotlightVsAABB(Spotlight spotlight, AABB aabb)
{
    float sphereRadius = length(aabb.extents); // AABB → Bounding Sphere 반지름^2
    float3 v = aabb.center - spotlight.position; // spotlight → AABB 중심까지 벡터
    float lenSq = dot(v, v); // v 벡터의 길이^2
    float v1Len = dot(v, spotlight.direction); // v 벡터를 spotlight 방향으로 투영한 길이

    // cone 외곽선과의 최소 거리 계산 - cos/sin (라디안)
    float cosA = cos(spotlight.angle);
    float sinA = sin(spotlight.angle);
    float perpDist = sqrt(max(lenSq - v1Len * v1Len, 0.0));
    float distanceClosestPoint = -v1Len * sinA + cosA * perpDist;

    bool angleCull = distanceClosestPoint > sphereRadius; // 제곱으로 수정
    bool frontCull = v1Len > sphereRadius + spotlight.range;
    bool backCull = v1Len < -sphereRadius;
    return !(angleCull || frontCull || backCull);
}


