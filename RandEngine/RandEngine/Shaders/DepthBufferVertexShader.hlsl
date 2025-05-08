

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

VS_OUTPUT mainVS(uint VertexID : SV_VertexID)
{
    VS_OUTPUT Output;

    float2 QuadPositions[6] = {
        float2(-1,  1),  // Top Left
        float2( 1,  1),  // Top Right
        float2(-1, -1),  // Bottom Left
        float2( 1,  1),  // Top Right
        float2( 1, -1),  // Bottom Right
        float2(-1, -1)   // Bottom Left
    };

    float2 UVs[6] = {
        float2(0, 0), float2(1, 0), float2(0, 1),
        float2(1, 0), float2(1, 1), float2(0, 1)
    };
    
    Output.Position = float4(QuadPositions[VertexID], 0, 1);
    Output.UV = UVs[VertexID];

    return Output;
}
