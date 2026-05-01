struct VSInput
{
    [[vk::location(0)]] float3 Position;

    // mat4 occupies locations 1,2,3,4
    [[vk::location(1)]] float4x4 ModelMatrix;
};

struct LightSpaceMatrixData
{
    float4x4 Matrix;
};

[[vk::binding(0,0)]]
ConstantBuffer<LightSpaceMatrixData> lightSpaceMatrix;

struct VSOutput
{
    float4 Position : SV_Position;
};

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;

    output.Position = mul(lightSpaceMatrix.Matrix, mul(input.ModelMatrix, float4(input.Position, 1.0)));

    return output;
}