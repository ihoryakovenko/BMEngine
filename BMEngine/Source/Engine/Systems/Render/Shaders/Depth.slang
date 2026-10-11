#include "Common.slang"
#include "ShaderTypes.h"
#include "Meshlet.slang"

struct MeshOutput
{
    float4 Position : SV_Position;
};

[shader("amplification")]
[numthreads(MESHLET_GROUP_SIZE, 1, 1)]
void MainTask(uint3 dispatchThreadID: SV_DispatchThreadID, uint groupThreadIndex: SV_GroupIndex)
{
    uint meshletIndex = dispatchThreadID.x;
    DispatchMeshlets(meshletIndex, groupThreadIndex, IsMeshletVisibleFromLight(meshletIndex));
}

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(MESHLET_GROUP_SIZE, 1, 1)]
void MainMesh(uint groupThreadIndex: SV_GroupIndex, uint3 groupID: SV_GroupID, in payload MeshletPayload payload,
    out vertices MeshOutput outVertices[MESHLET_MAX_VERTICES], out indices uint3 outTriangles[MESHLET_MAX_TRIANGLES])
{
    Shader_Meshlet meshlet = PushData.FrameData.MeshletBuffer[payload.MeshletIndices[groupID.x]];
    Shader_StaticMeshInstance instance = PushData.FrameData.StaticMeshInstanceBuffer[meshlet.InstanceIndex];

    SetMeshOutputCounts(meshlet.VertexCount, meshlet.TriangleCount);

    for (uint i = groupThreadIndex; i < meshlet.VertexCount; i += MESHLET_GROUP_SIZE)
    {
        Shader_StaticMeshVertex vertex = LoadMeshletVertex(meshlet, i);

        MeshOutput output;
        output.Position = mul(PushData.LightSpaceMatrixData.Matrix, mul(instance.ModelMatrix, float4(vertex.Position, 1.0)));

        outVertices[i] = output;
    }

    for (uint i = groupThreadIndex; i < meshlet.TriangleCount; i += MESHLET_GROUP_SIZE)
    {
        outTriangles[i] = LoadMeshletTriangle(meshlet, i);
    }
}
