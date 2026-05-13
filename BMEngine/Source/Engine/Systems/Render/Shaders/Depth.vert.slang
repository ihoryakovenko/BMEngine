#include "Common.h"
#include "Depth.h"
#include "ShaderTypes.h"

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