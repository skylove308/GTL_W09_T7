#define NUM_FACES 6 // 1개 삼각형 당 6개의 Depth용 버텍스 필요


cbuffer DepthCubeMapConstants : register(b0)
{
    row_major matrix World;
    row_major matrix ViewProj[NUM_FACES];
}

struct VS_OUTPUT_CubeMap
{
    float4 position : POSITION;
};

struct GS_OUTPUT
{
    float4 pos : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};


[maxvertexcount(3 * NUM_FACES)]
void mainGS(
    triangle VS_OUTPUT_CubeMap input[3],
    inout TriangleStream<GS_OUTPUT> TriStream
)
{
    for (uint face = 0; face < NUM_FACES; ++face)
    {
        for (int i = 0; i < 3; ++i)
        {
            GS_OUTPUT output;
            float4 worldPos = mul(input[i].position, World);
            output.pos = mul(worldPos, ViewProj[face]);
            output.RTIndex = face;
            TriStream.Append(output);
        }
        TriStream.RestartStrip();
    }
}


// float4 mainPS(GS_OUTPUT Input) : SV_TARGET
// {
//     // clip.z/clip.w 를 깊이로 저장
//     float depth = Input.pos.z / Input.pos.w;
//     return float4(depth, depth, depth, 1.0f);
// }
