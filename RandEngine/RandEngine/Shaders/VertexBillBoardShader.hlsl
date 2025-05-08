
#include "ShaderRegisters.hlsl"

struct VS_Input 
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
};

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

PS_Input main(VS_Input Input)
{
    PS_Input Output;
    Output.Position = float4(Input.Position, 1.0);
    Output.Position = mul(Output.Position, WorldMatrix);
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);
    
    Output.UV = Input.UV;
    
    return Output;
}
