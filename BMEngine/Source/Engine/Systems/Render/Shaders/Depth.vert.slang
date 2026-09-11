#include "Common.slang"
#include "ShaderTypes.h"

struct VSOutput
{
    float4 Position : SV_Position;
};

[shader("vertex")]
VSOutput main(uint vID: SV_VertexID, uint iID: SV_InstanceID)
{
    Shader_StaticMeshVertex vertex = PushData.FrameData.StaticMeshVertexBuffer[vID];
    Shader_StaticMeshInstance instance = PushData.FrameData.StaticMeshInstanceBuffer[iID];

    VSOutput output;

    output.Position = mul(PushData.LightSpaceMatrixData.Matrix, mul(instance.ModelMatrix, float4(vertex.Position, 1.0)));

    return output;
}