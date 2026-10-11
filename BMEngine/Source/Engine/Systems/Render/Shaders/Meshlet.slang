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

float4 GetWorldBoundingSphere(Shader_Meshlet meshlet, float4x4 modelMatrix)
{
    float3 center = mul(modelMatrix, float4(meshlet.BoundsCenter, 1.0)).xyz;

    float3x3 axes = transpose((float3x3)modelMatrix);
    float maxScaleSquared = max(max(dot(axes[0], axes[0]), dot(axes[1], axes[1])), dot(axes[2], axes[2]));

    return float4(center, meshlet.BoundsRadius * sqrt(maxScaleSquared));
}

bool IsSphereInFrustum(float4 sphere, float4x4 viewProjection)
{
    float4 planes[6] =
    {
        viewProjection[3] + viewProjection[0],
        viewProjection[3] - viewProjection[0],
        viewProjection[3] + viewProjection[1],
        viewProjection[3] - viewProjection[1],
        viewProjection[2],
        viewProjection[3] - viewProjection[2],
    };

    for (uint i = 0; i < 6; ++i)
    {
        if (dot(planes[i].xyz, sphere.xyz) + planes[i].w < -sphere.w * length(planes[i].xyz))
        {
            return false;
        }
    }

    return true;
}

bool IsMeshletBackFacing(Shader_Meshlet meshlet, float4 sphere, float4x4 modelMatrix, float3 cameraPosition)
{
    float3 coneAxis = normalize(mul((float3x3)modelMatrix, meshlet.ConeAxis));
    float3 toCenter = sphere.xyz - cameraPosition;

    return dot(toCenter, coneAxis) >= meshlet.ConeCutoff * length(toCenter) + sphere.w;
}

bool IsMeshletVisibleFromCamera(uint meshletIndex)
{
    if (meshletIndex >= PushData.FrameData.MeshletCount)
    {
        return false;
    }

    Shader_Meshlet meshlet = PushData.FrameData.MeshletBuffer[meshletIndex];
    float4x4 modelMatrix = PushData.FrameData.StaticMeshInstanceBuffer[meshlet.InstanceIndex].ModelMatrix;
    float4 sphere = GetWorldBoundingSphere(meshlet, modelMatrix);

    return IsSphereInFrustum(sphere, PushData.FrameData.CullViewProjection) &&
        !IsMeshletBackFacing(meshlet, sphere, modelMatrix, PushData.FrameData.CullCameraPosition);
}

bool IsMeshletVisibleFromLight(uint meshletIndex)
{
    if (meshletIndex >= PushData.FrameData.MeshletCount)
    {
        return false;
    }

    Shader_Meshlet meshlet = PushData.FrameData.MeshletBuffer[meshletIndex];
    float4x4 modelMatrix = PushData.FrameData.StaticMeshInstanceBuffer[meshlet.InstanceIndex].ModelMatrix;

    return IsSphereInFrustum(GetWorldBoundingSphere(meshlet, modelMatrix), PushData.LightSpaceMatrixData.Matrix);
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
