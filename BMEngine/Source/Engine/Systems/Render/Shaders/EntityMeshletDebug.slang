#include "Common.slang"
#include "ShaderTypes.h"
#include "Meshlet.slang"

struct MeshletDebugLayout
{
    nointerpolation float3 Color;
};

struct FragmentInput
{
    MeshletDebugLayout layout;
};

struct FragmentOutput
{
    float4 OutColor;
};

struct MeshOutput
{
    MeshletDebugLayout layout;
    float4 Position : SV_Position;
};

float3 GetMeshletColor(uint meshletIndex)
{
    uint hash = meshletIndex * 747796405u + 2891336453u;
    hash = ((hash >> ((hash >> 28u) + 4u)) ^ hash) * 277803737u;
    hash = (hash >> 22u) ^ hash;

    return float3(hash & 0xFF, (hash >> 8) & 0xFF, (hash >> 16) & 0xFF) / 255.0;
}

[shader("amplification")]
[numthreads(MESHLET_GROUP_SIZE, 1, 1)]
void MainTask(uint3 dispatchThreadID: SV_DispatchThreadID, uint groupThreadIndex: SV_GroupIndex)
{
    uint meshletIndex = dispatchThreadID.x;
    DispatchMeshlets(meshletIndex, groupThreadIndex, IsMeshletVisibleFromCamera(meshletIndex));
}

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(MESHLET_GROUP_SIZE, 1, 1)]
void MainMesh(uint groupThreadIndex: SV_GroupIndex, uint3 groupID: SV_GroupID, in payload MeshletPayload payload,
    out vertices MeshOutput outVertices[MESHLET_MAX_VERTICES], out indices uint3 outTriangles[MESHLET_MAX_TRIANGLES])
{
    uint meshletIndex = payload.MeshletIndices[groupID.x];
    Shader_Meshlet meshlet = PushData.FrameData.MeshletBuffer[meshletIndex];
    Shader_StaticMeshInstance instance = PushData.FrameData.StaticMeshInstanceBuffer[meshlet.InstanceIndex];

    SetMeshOutputCounts(meshlet.VertexCount, meshlet.TriangleCount);

    float3 color = GetMeshletColor(meshletIndex);

    for (uint i = groupThreadIndex; i < meshlet.VertexCount; i += MESHLET_GROUP_SIZE)
    {
        Shader_StaticMeshVertex vertex = LoadMeshletVertex(meshlet, i);

        float4 worldPos = mul(instance.ModelMatrix, float4(vertex.Position, 1.0));

        MeshOutput output;
        output.layout.Color = color;
        output.Position = mul(PushData.FrameData.Projection, mul(PushData.FrameData.View, worldPos));

        outVertices[i] = output;
    }

    for (uint i = groupThreadIndex; i < meshlet.TriangleCount; i += MESHLET_GROUP_SIZE)
    {
        outTriangles[i] = LoadMeshletTriangle(meshlet, i);
    }
}

[shader("fragment")]
FragmentOutput MainFragment(FragmentInput input)
{
    FragmentOutput output;
    output.OutColor = float4(input.layout.Color, 1.0);
    return output;
}
