#include "ShaderTypes.h"

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
VSOutput main(StaticMeshVertexInput input)
{
    StaticMeshVertex vertex = input.Vertex;
    StaticMeshInstance instance = input.Instance;

    VSOutput output;

    output.Position = mul(lightSpaceMatrix.Matrix, mul(instance.ModelMatrix, float4(vertex.Position, 1.0)));

    return output;
}