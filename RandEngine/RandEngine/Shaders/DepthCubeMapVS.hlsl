// Depth Only Vertex Shader
#define NUM_FACES 6 // 1개 삼각형 당 6개의 Depth용 버텍스 필요

struct VS_INPUT_StaticMesh
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    uint MaterialIndex : MATERIAL_INDEX;
};

struct VS_OUTPUT_CubeMap
{
    float4 position : POSITION;
};

cbuffer DepthCubeMapConstants : register(b0)
{
    row_major matrix World;
    row_major matrix ViewProj[NUM_FACES];
}

VS_OUTPUT_CubeMap mainVS(VS_INPUT_StaticMesh Input)
{
    VS_OUTPUT_CubeMap output;
    //output.position = mul(float4(Input.Position, 1.0f), World);
    output.position = float4(Input.Position, 1.0f);
    return output;
}
