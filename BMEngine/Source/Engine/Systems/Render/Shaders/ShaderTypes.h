#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

#ifdef __cplusplus

#include <cstdint>
#include <glm/glm.hpp>

#define SHADER_BUFFER_ALIGNAS alignas(16)
#define SHADER_VERTEX_ALIGNAS alignas(4)

#define float4x4 glm::mat4
#define float4 glm::vec4
#define float3 glm::vec3
#define float2 glm::vec2
#define uint uint32_t

#else

#define SHADER_BUFFER_ALIGNAS
#define SHADER_VERTEX_ALIGNAS

#endif

struct SHADER_BUFFER_ALIGNAS PointLight
{
    float3 Position;
    float pad1;
    float3 Color;
};

struct SHADER_BUFFER_ALIGNAS DirectionLight
{
    float4x4 LightSpaceMatrix;
    float3 Direction;
    float pad1;
    float3 Color;
};

struct SHADER_BUFFER_ALIGNAS SpotLight
{
    float4x4 LightSpaceMatrix;
    float3 Position;
    float CutOff;
    float3 Direction;
    float OuterCutOff;
    float3 Color;
    float pad1;
    float2 Planes;
};

struct SHADER_BUFFER_ALIGNAS FrameBuffer
{
    float4x4 View;
    float4x4 Projection;
    PointLight pointlight;
    DirectionLight directionLight;
    SpotLight spotlight;
};

struct SHADER_BUFFER_ALIGNAS Material
{
    uint AlbedoTexIndex;
    uint SpecularTexIndex;
    float Shininess;
};

struct SHADER_VERTEX_ALIGNAS StaticMeshVertex
{
    float3 Position;
    float2 TextureCoords;
    float3 Normal;
};

struct SHADER_VERTEX_ALIGNAS StaticMeshInstance
{
    float4x4 ModelMatrix;
    uint MaterialIndex;
};

struct SHADER_VERTEX_ALIGNAS StaticMeshVertexInput
{
    [[vk::location(0)]] StaticMeshVertex Vertex;
    [[vk::location(3)]] StaticMeshInstance Instance;
};

#endif