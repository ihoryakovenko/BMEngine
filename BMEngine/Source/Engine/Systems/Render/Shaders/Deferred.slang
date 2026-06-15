
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
[[vk::binding(2, 1)]] RWTexture2D<float4> OutputTexture;

[numthreads(8, 8, 1)]
[shader("compute")]
void main(uint2 dispatchThreadID: SV_DispatchThreadID)
{
    float4 color = InputColor.Load(int3(dispatchThreadID, 0));
    float depth = InputDepth.Load(int3(dispatchThreadID, 0)).r;

	float Gamma = 2.2;
    int xHalf = 1600;

    if (false)
    {
        //float lowerBound = 0.99;
        //float upperBound = 1.0;

        //float depthColorScaled = 1.0 - ((depth - lowerBound) / (upperBound - lowerBound));

        //color = float4(0.0, depthColorScaled, 0.0, color.a);
    }
    else
    {
        // output.Color = float4(Grayscale(SceneColor.rgb), SceneColor.a);
        // output.Color = float4(InverseColor(SceneColor.rgb), SceneColor.a);
        //output.Color = SceneColor;
    }

    color.rgb = pow(color.rgb, float3(1.0 / Gamma));

    OutputTexture[dispatchThreadID] = color;
}
