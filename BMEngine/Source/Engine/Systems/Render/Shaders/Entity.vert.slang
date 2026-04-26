#include "Common.slang"

struct VertexInput
{
    [[vk::location(0)]] float3 Position;
    [[vk::location(1)]] float2 TextureCoords;
    [[vk::location(2)]] float3 Normal;

    // mat4 occupies locations 3,4,5,6
    [[vk::location(3)]] float4x4 ModelMatrix;
    [[vk::location(7)]] uint MaterialIndex;
};

struct VertexOutput
{
    [[vk::location(0)]] float2 FragmentTexture;
    [[vk::location(1)]] float3 FragmentNormal;
    [[vk::location(2)]] float3 WorldFragPos;
    [[vk::location(3)]] nointerpolation uint FragmentMaterialIndex;

    float4 Position : SV_Position;
};

struct PushConstants
{
    uint FrameIndex;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> Constants;

[[vk::binding(0,0)]]
ConstantBuffer<UboViewProjection> ViewProjection;

float3x3 inverse(float3x3 m)
{
    float3 a = m[0];
    float3 b = m[1];
    float3 c = m[2];

    float3 r0 = cross(b, c);
    float3 r1 = cross(c, a);
    float3 r2 = cross(a, b);

    float invDet = 1.0 / dot(r2, c);
    return float3x3(r0 * invDet, r1 * invDet, r2 * invDet);
}

[shader("vertex")]
VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.FragmentTexture = input.TextureCoords;

    float4 worldPos = mul(input.ModelMatrix, float4(input.Position, 1.0));
    output.WorldFragPos = worldPos.xyz;

    float3x3 normalMatrix = (float3x3)transpose(inverse((float3x3)input.ModelMatrix));

    output.FragmentNormal = normalize(mul(normalMatrix, input.Normal));
    output.FragmentMaterialIndex = input.MaterialIndex;
    output.Position = mul(ViewProjection.Projection, mul(ViewProjection.View, worldPos));

    return output;
}