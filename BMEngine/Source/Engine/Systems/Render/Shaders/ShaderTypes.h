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

#else

#endif

static const int MAX_SHADOW_TEXTURES = 2;

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

struct Shader_StaticMeshVertexInput
{
    Shader_StaticMeshVertex Vertex;
    Shader_StaticMeshInstance Instance;
};

struct Shader_LightSpaceMatrixData
{
    float4x4 Matrix;
};

struct Shader_FrameData
{
    float4x4 View;
    float4x4 Projection;
    Shader_PointLight pointlight;
    Shader_DirectionLight directionLight;
    Shader_SpotLight spotlight;
    Shader_StaticMeshVertex* StaticMeshVertexBuffer;
    Shader_StaticMeshInstance* StaticMeshInstanceBuffer;
    Shader_Material* MaterialBuffer;
    uint pad;
};

struct Shader_PushData
{
    Shader_FrameData* FrameData;
    Shader_LightSpaceMatrixData* LightSpaceMatrixData;
};

#endif