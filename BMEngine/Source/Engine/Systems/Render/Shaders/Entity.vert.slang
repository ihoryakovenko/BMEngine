#include "Common.h"
#include "ShaderTypes.h"
#include "Entity.h"

struct VertexOutput
{
    ShaderLayout layout;

    float4 Position : SV_Position;
};

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
VertexOutput main(uint vID: SV_VertexID, uint iID: SV_StartInstanceLocation)
{
    StaticMeshVertex vertex = VertexDescriptor[vID];
    StaticMeshInstance instance = InstanceDescriptor[iID];

    VertexOutput output;
    output.layout.FragmentTexture = vertex.TextureCoords;

    float4 worldPos = mul(instance.ModelMatrix, float4(vertex.Position, 1.0));
    output.layout.WorldFragPos = worldPos.xyz;

    float3x3 normalMatrix = (float3x3)transpose(inverse((float3x3)instance.ModelMatrix));

    output.layout.FragmentNormal = normalize(mul(normalMatrix, vertex.Normal));
    output.layout.FragmentMaterialIndex = instance.MaterialIndex;
    output.Position = mul(FrameBufferDescriptor.Projection, mul(FrameBufferDescriptor.View, worldPos));

    return output;
}