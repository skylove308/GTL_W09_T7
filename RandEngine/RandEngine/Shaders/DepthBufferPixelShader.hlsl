
#define CAMERA_NEAR 0.1f
#define CAMERA_FAR  100.0f

Texture2D SceneDepthTexture : register(t99);
SamplerState DepthSampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float DepthRaw = SceneDepthTexture.Sample(DepthSampler, Input.UV).r;

    float DepthNDC = DepthRaw * 2.0 - 1.0;
    
    float DepthLinearized = (2.0 * CAMERA_NEAR * CAMERA_FAR) / (CAMERA_FAR + CAMERA_NEAR - DepthNDC * (CAMERA_FAR - CAMERA_NEAR));

    float DepthNormalized = saturate((DepthLinearized - CAMERA_NEAR) / (CAMERA_FAR - CAMERA_NEAR));

    return float4(DepthNormalized, DepthNormalized, DepthNormalized, 1.0);
}
