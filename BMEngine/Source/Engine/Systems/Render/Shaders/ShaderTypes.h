#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

#ifdef __cplusplus

#include <ShortTypes.h>
#include <glm/glm.hpp>
#include <glm/gtx/type_aligned.hpp>

#define INLINE_GLOBAL inline

typedef glm::mat4 float4x4;
typedef glm::vec4 float4;
typedef glm::vec3 float3;
typedef glm::vec2 float2;
typedef u32 uint;

typedef glm::aligned_mat4 float4x4_16;
typedef glm::aligned_vec4 float4_16;
typedef glm::aligned_vec3 float3_16;
typedef glm::aligned_vec2 float2_8;

template<typename T>
struct ParameterBlock
{
    static_assert(sizeof(T) % 16 == 0, "T size must be a multiple of 16 for alignment.");
    static_assert(alignof(T) >= 16, "T must be aligned to at least 16 bytes.");

    T Data;
    BmRender_DescriptorSet Set;
    static constexpr u64 DataSize = (sizeof(T) + 15) & ~u64(15);
};

template<typename T>
struct StructuredBuffer
{
    static_assert(sizeof(T) % 4 == 0, "T size must be a multiple of 4 for alignment.");
    static_assert(alignof(T) >= 4, "T must be aligned to at least 4 bytes.");

    T Data;
    BmRender_DescriptorSet Set;
    static constexpr u64 DataSize = (sizeof(T) + 15) & ~u64(15);
};

#else

#define INLINE_GLOBAL

typedef float4x4 float4x4_16;
typedef float4 float4_16;
typedef float3 float3_16;
typedef float2 float2_8;

#endif

struct PointLight
{
    float3_16 Position;
    float3_16 Color;
};

struct DirectionLight
{
    float4x4_16 LightSpaceMatrix;
    float3_16 Direction;
    float3_16 Color;
};

struct SpotLight
{
    float4x4_16 LightSpaceMatrix;
    float3_16 Position;
    float CutOff;
    float3_16 Direction;
    float OuterCutOff;
    float3_16 Color;
    float2_8 Planes;
};

struct FrameData
{
    float4x4_16 View;
    float4x4_16 Projection;
    PointLight pointlight;
    DirectionLight directionLight;
    SpotLight spotlight;
};

struct Material
{
    uint AlbedoTexIndex;
    uint SpecularTexIndex;
    float Shininess;
};

struct StaticMeshVertex
{
    float3 Position;
    float2 TextureCoords;
    float3 Normal;
};

struct StaticMeshInstance
{
    float4x4 ModelMatrix;
    uint MaterialIndex;
};

struct StaticMeshVertexInput
{
    [[vk::location(0)]] StaticMeshVertex Vertex;
    [[vk::location(3)]] StaticMeshInstance Instance;
};

#endif