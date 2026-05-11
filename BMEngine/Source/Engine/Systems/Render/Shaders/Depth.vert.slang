#include "ShaderTypes.h"

struct LightSpaceMatrixData
{
    float4x4 Matrix;
};

[[vk::binding(0,0)]]
ConstantBuffer<LightSpaceMatrixData> lightSpaceMatrix;

DECLARE_STORAGE_BUFFER_DESCRIPTOR(StaticMeshVertex, 0, 1, BmRender_DescriptorShaderStage::Vertex, VertexDescriptor);
DECLARE_STORAGE_BUFFER_DESCRIPTOR(StaticMeshInstance, 1, 1, BmRender_DescriptorShaderStage::Vertex, InstanceDescriptor);

struct VSOutput
{
    float4 Position : SV_Position;
};

[shader("vertex")]
VSOutput main(uint vID: SV_VertexID, uint iID: SV_InstanceID)
{
    StaticMeshVertex vertex = VertexDescriptor[vID];
    StaticMeshInstance instance = InstanceDescriptor[iID];

    VSOutput output;

    output.Position = mul(lightSpaceMatrix.Matrix, mul(instance.ModelMatrix, float4(vertex.Position, 1.0)));

    return output;
}