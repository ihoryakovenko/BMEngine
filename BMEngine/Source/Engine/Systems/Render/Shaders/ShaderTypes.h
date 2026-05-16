#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

#ifdef __cplusplus

#include <array>

#include <ShortTypes.h>
#include <glm/glm.hpp>
#include <glm/gtx/type_aligned.hpp>
#include <RenderInterface.h>

struct Shader_Descriptor
{
    u32 Binding;
    u32 Set;
    BmRender_DescriptorShaderStage Stage;
    BmRender_DescriptorType Type;
    bool IsBindless;
};

template<u32 N>
struct Shader_DescriptorSet
{
    std::array<Shader_Descriptor, N> Descriptors;
    u32 Set;
};

template <typename... Args>
constexpr auto Shader_CreateDescriptorSet(Args&&... args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        throw "Empty descriptor set";
    }

    const std::array<Shader_Descriptor, sizeof...(Args)> Descriptors{ std::forward<Args>(args)... };
    const u32 FirstSet = Descriptors[0].Set;

    for (u32 i = 0; i < Descriptors.size(); ++i)
    {
        if (Descriptors[i].Set != FirstSet)
        {
            throw "All descriptors in a set must have the same Set index";
        }

        for (u32 j = i + 1; j < Descriptors.size(); ++j)
        {
            if (Descriptors[i].Binding == Descriptors[j].Binding)
            {
                throw "All descriptors in a set must have the same Set index";
            }
        }
    }

    return Shader_DescriptorSet<sizeof...(Args)>{ Descriptors, FirstSet };
}

#define DECLARE_UNIFORM_BUFFER_DYNAMIC_DESCRIPTOR(T, Binding, Set, Stage, Name) Shader_Descriptor{ Binding, Set, Stage, BmRender_DescriptorType::UniformBufferDynamic, false }
#define DECLARE_UNIFORM_BUFFER_DESCRIPTOR(T, Binding, Set, Stage, Name) Shader_Descriptor{ Binding, Set, Stage, BmRender_DescriptorType::UniformBuffer, false }
#define DECLARE_STORAGE_BUFFER_DESCRIPTOR(T, Binding, Set, Stage, Name) Shader_Descriptor{ Binding, Set, Stage, BmRender_DescriptorType::StorageBuffer, false }
#define DECLARE_IMAGE_SAMPLER2D_BINDLESS_DESCRIPTOR(Binding, Set, Stage, Name) Shader_Descriptor{ Binding, Set, Stage, BmRender_DescriptorType::CombinedImageSampler, true }
#define DECLARE_IMAGE_SAMPLER2D_DESCRIPTOR(Binding, Set, Stage, Name) Shader_Descriptor{ Binding, Set, Stage, BmRender_DescriptorType::CombinedImageSampler, false }
#define DECLARE_IMAGE_SAMPLER2D_ARRAY_DESCRIPTOR(Binding, Set, Stage, Name) Shader_Descriptor{ Binding, Set, Stage, BmRender_DescriptorType::CombinedImageSampler, false }

#define START_DESCRIPTOR_SET(Name) inline constexpr auto Name = Shader_CreateDescriptorSet(
#define END_DESCRIPTOR_SET() );
#define DESCRIPTOR_AND ,

#define NOINTERPOLATION

typedef glm::mat4 float4x4;
typedef glm::vec4 float4;
typedef glm::vec3 float3;
typedef glm::ivec3 uint3;
typedef glm::vec2 float2;
typedef glm::ivec2 uint2;
typedef u32 uint;

#else

#define DECLARE_UNIFORM_BUFFER_DYNAMIC_DESCRIPTOR(T, Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] ConstantBuffer<T> Name;

#define DECLARE_UNIFORM_BUFFER_DESCRIPTOR(T, Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] ConstantBuffer<T> Name;

#define DECLARE_STORAGE_BUFFER_DESCRIPTOR(T, Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] StructuredBuffer<T> Name;

#define DECLARE_IMAGE_SAMPLER2D_BINDLESS_DESCRIPTOR(Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] Sampler2D Name[];

#define DECLARE_IMAGE_SAMPLER2D_ARRAY_DESCRIPTOR(Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] Sampler2DArray Name; \

#define DECLARE_IMAGE_SAMPLER2D_DESCRIPTOR(Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] Sampler2D Name; \

#define DESCRIPTOR_AND ;

#define START_DESCRIPTOR_SET(Name)
#define END_DESCRIPTOR_SET()

#define NOINTERPOLATION nointerpolation

#endif

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

struct Shader_FrameData
{
    float4x4 View;
    float4x4 Projection;
    Shader_PointLight pointlight;
    Shader_DirectionLight directionLight;
    Shader_SpotLight spotlight;
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

#endif