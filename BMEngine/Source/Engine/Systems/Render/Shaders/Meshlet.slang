#ifndef MESHLET_H
#define MESHLET_H

#include "Common.slang"
#include "ShaderTypes.h"

struct MeshletPayload
{
    uint MeshletIndices[MESHLET_GROUP_SIZE];
};

groupshared MeshletPayload Payload;
groupshared uint VisibleMeshletMask;

void DispatchMeshlets(uint meshletIndex, uint groupThreadIndex, bool visible)
{
    if (groupThreadIndex == 0)
    {
        VisibleMeshletMask = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    if (visible)
    {
        InterlockedOr(VisibleMeshletMask, 1u << groupThreadIndex);
    }

    GroupMemoryBarrierWithGroupSync();

    if (visible)
    {
        Payload.MeshletIndices[countbits(VisibleMeshletMask & ((1u << groupThreadIndex) - 1))] = meshletIndex;
    }

    GroupMemoryBarrierWithGroupSync();

    DispatchMesh(countbits(VisibleMeshletMask), 1, 1, Payload);
}

Shader_StaticMeshVertex LoadMeshletVertex(Shader_Meshlet meshlet, uint vertexIndex)
{
    uint vID = PushData.FrameData.MeshletVertexBuffer[meshlet.VertexOffset + vertexIndex];
    return PushData.FrameData.StaticMeshVertexBuffer[vID];
}

uint3 LoadMeshletTriangle(Shader_Meshlet meshlet, uint triangleIndex)
{
    uint packedTriangle = PushData.FrameData.MeshletTriangleBuffer[meshlet.TriangleOffset + triangleIndex];
    return uint3(packedTriangle & 0xFF, (packedTriangle >> 8) & 0xFF, (packedTriangle >> 16) & 0xFF);
}

#endif
