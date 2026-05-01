#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

#ifdef __cplusplus

#include <cstdint>
#include <glm/glm.hpp>

#define SHADER_ALIGNAS alignas(16)

#define float4x4 glm::mat4
#define float4 glm::vec4
#define float3 glm::vec3
#define float2 glm::vec2
#define uint uint32_t

#else

#define SHADER_ALIGNAS

#endif

struct SHADER_ALIGNAS UboViewProjection
{
    float4x4 View;
    float4x4 Projection;
};

struct SHADER_ALIGNAS PointLight
{
    float3 Position;
    float pad1;
    float3 Color;
};

struct SHADER_ALIGNAS DirectionLight
{
    float4x4 LightSpaceMatrix;
    float3 Direction;
    float pad1;
    float3 Color;
};

struct SHADER_ALIGNAS SpotLight
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

struct SHADER_ALIGNAS LightCastersData
{
    PointLight pointlight;
    DirectionLight directionLight;
    SpotLight spotlight;
};

struct SHADER_ALIGNAS Material
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