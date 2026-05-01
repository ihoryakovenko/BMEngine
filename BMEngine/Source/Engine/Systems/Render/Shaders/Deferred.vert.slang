struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

[shader("vertex")]
VSOutput main(uint vertexIndex: SV_VulkanVertexID)
{
    const float2 positions[3] = {
        float2( 3.0, -1.0),
        float2(-1.0, -1.0),
        float2(-1.0,  3.0)
    };

    float2 pos = positions[vertexIndex];

    VSOutput output;
    output.position = float4(pos, 0.0, 1.0);
    output.uv       = pos * 0.5 + 0.5;

    return output;
}