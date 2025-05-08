// Depth Only Vertex Shader

#include "ShaderRegisters.hlsl"

cbuffer FShadowConstantBuffer : register(b11)
{
    row_major matrix ShadowViewProj;
};

float4 mainVS(VS_INPUT_StaticMesh Input) : SV_POSITION
{
    float4 pos = mul(float4(Input.Position, 1.0f), WorldMatrix);
    pos = mul(pos, ShadowViewProj);
    return pos;
}
