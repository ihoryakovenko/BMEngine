#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

#ifdef __cplusplus

#include <glm/glm.hpp>

typedef glm::mat4 float4x4;
typedef glm::vec4 float4;
typedef glm::ivec4 uint4;
typedef glm::vec3 float3;
typedef glm::ivec3 uint3;
typedef glm::vec2 float2;
typedef glm::ivec2 uint2;
typedef u32 uint;

#define GPU_POINTER_TYPE(T) u64

#else

#define GPU_POINTER_TYPE(T) T*

#endif

static const int MAX_SHADOW_TEXTURES = 2;

static const uint MESHLET_MAX_VERTICES = 64;
static const uint MESHLET_MAX_TRIANGLES = 124;
static const uint MESHLET_GROUP_SIZE = 32;

struct Shader_PointLight
{
    float3 Position;
    uint pad1;
    float3 Color;
    uint pad2;
};

struct Shader_DirectionLight
{
    float4x4 LightSpaceMatrix;
    float3 Direction;
    uint pad1;
    float3 Color;
    uint pad2;
};

struct Shader_SpotLight
{
    float4x4 LightSpaceMatrix;
    float3 Position;
    float CutOff;
    float3 Direction;
    float OuterCutOff;
    float3 Color;
    uint pad1;
    float2 Planes;
    uint2 pad2;
};

struct Shader_Material
{
    uint AlbedoTexIndex;
    uint SpecularTexIndex;
    float Shininess;
};

struct Shader_StaticMeshVertex
{
    float3 Position;
    uint pad1;
    float2 TextureCoords;
    uint2 pad2;
    float3 Normal;
    uint pad3;
};

struct Shader_StaticMeshInstance
{
    float4x4 ModelMatrix;
    uint3 pad1;
    uint MaterialIndex;
    
};

struct Shader_Meshlet
{
    float3 BoundsCenter;
    float BoundsRadius;
    float3 ConeAxis;
    float ConeCutoff;
    uint VertexOffset;
    uint TriangleOffset;
    uint VertexCount;
    uint TriangleCount;
    uint InstanceIndex;
    uint pad1;
    uint pad2;
    uint pad3;
};

struct Shader_LightSpaceMatrixData
{
    float4x4 Matrix;
    float4x4 CullMatrix;
};

struct Shader_FrameData
{
    float4x4 View;
    float4x4 Projection;
    Shader_PointLight pointlight;
    Shader_DirectionLight directionLight;
    Shader_SpotLight spotlight;
    GPU_POINTER_TYPE(Shader_StaticMeshVertex) StaticMeshVertexBuffer;
    GPU_POINTER_TYPE(Shader_StaticMeshInstance) StaticMeshInstanceBuffer;
    GPU_POINTER_TYPE(Shader_Material) MaterialBuffer;
    GPU_POINTER_TYPE(Shader_Meshlet) MeshletBuffer;
    GPU_POINTER_TYPE(uint) MeshletVertexBuffer;
    GPU_POINTER_TYPE(uint) MeshletTriangleBuffer;
    float4x4 CullViewProjection;
    float3 CullCameraPosition;
    uint MeshletCount;
};

struct Shader_PushData
{
    GPU_POINTER_TYPE(Shader_FrameData) FrameData;
    GPU_POINTER_TYPE(Shader_LightSpaceMatrixData) LightSpaceMatrixData;
};

#endif