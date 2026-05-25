
struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float2 vUV : TEXCOORD0;
};

struct PSOutput
{
    float4 Color : SV_Target0;
};

float3 InverseColor(float3 Color)
{
    return float3(1.0 - Color);
}

float3 Grayscale(float3 Color)
{
    float Average = 0.2126 * Color.r + 0.7152 * Color.g + 0.0722 * Color.b;
    return float3(Average, Average, Average);
}

[[vk::binding(0, 1)]] Sampler2D InputColor;
[[vk::binding(1, 1)]] Sampler2D InputDepth;

[shader("vertex")]
VSOutput MainVertex(uint vertexIndex: SV_VulkanVertexID)
{
    const float2 positions[3] = {
        float2(3.0, -1.0),
        float2(-1.0, -1.0),
        float2(-1.0, 3.0)
    };

    float2 pos = positions[vertexIndex];

    VSOutput output;
    output.position = float4(pos, 0.0, 1.0);
    output.uv = pos * 0.5 + 0.5;

    return output;
}

[shader("fragment")]
PSOutput MainFragment(PSInput input)
{
    PSOutput output;

    float4 SceneColor = InputColor.Sample(input.vUV);
    float depth = InputDepth.Sample(input.vUV).r;

    float Gamma = 2.2;

    int xHalf = 1600;

    if (false)
    {
        float lowerBound = 0.99;
        float upperBound = 1.0;

        float depthColorScaled = 1.0 - ((depth - lowerBound) / (upperBound - lowerBound));

        output.Color = float4(0.0, depthColorScaled, 0.0, SceneColor.a);
    }
    else
    {
        // output.Color = float4(Grayscale(SceneColor.rgb), SceneColor.a);
        // output.Color = float4(InverseColor(SceneColor.rgb), SceneColor.a);
        output.Color = SceneColor;
    }

    output.Color.rgb = pow(output.Color.rgb, float3(1.0 / Gamma));

    return output;
}