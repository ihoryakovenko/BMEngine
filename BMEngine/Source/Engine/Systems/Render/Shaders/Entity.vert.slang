#include "Common.slang"
#include "ShaderTypes.h"

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

[[vk::push_constant]] ConstantBuffer<PushConstants> Constants;

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
VertexOutput main(StaticMeshVertexInput input)
{
    StaticMeshVertex vertex = input.Vertex;
    StaticMeshInstance instance = input.Instance;

    VertexOutput output;
    output.FragmentTexture = vertex.TextureCoords;

    float4 worldPos = mul(instance.ModelMatrix, float4(vertex.Position, 1.0));
    output.WorldFragPos = worldPos.xyz;

    float3x3 normalMatrix = (float3x3)transpose(inverse((float3x3)instance.ModelMatrix));

    output.FragmentNormal = normalize(mul(normalMatrix, vertex.Normal));
    output.FragmentMaterialIndex = instance.MaterialIndex;
    output.Position = mul(ViewProjection.Projection, mul(ViewProjection.View, worldPos));

    return output;
}