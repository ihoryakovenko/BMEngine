#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

#ifdef __cplusplus

#include <ShortTypes.h>
#include <glm/glm.hpp>
#include <glm/gtx/type_aligned.hpp>
#include <RenderInterface.h>

struct Descriptor
{
    u32 Binding;
    u32 Set;
    BmRender_DescriptorShaderStage Stage;
    BmRender_DescriptorType Type;
};

struct DescriptorSet
{
    u32 Set;
    std::vector<Descriptor> Descriptors;
};

#define DECLARE_UNIFORM_BUFFER_DYNAMIC_DESCRIPTOR(T, Binding, Set, Stage, Name) \
inline Descriptor Name = { Binding, Set, Stage, BmRender_DescriptorType::UniformBufferDynamic } \

#define DECLARE_STORAGE_BUFFER_DESCRIPTOR(T, Binding, Set, Stage, Name) \
inline Descriptor Name = { Binding, Set, Stage, BmRender_DescriptorType::StorageBuffer } \

#define DECLARE_IMAGE_SAMPLER2D_BINDLESS_DESCRIPTOR(Binding, Set, Stage, Name) \
inline Descriptor Name = { Binding, Set, Stage, BmRender_DescriptorType::CombinedImageSampler } \

#define DECLARE_IMAGE_SAMPLER2D_ARRAY_DESCRIPTOR(Binding, Set, Stage, Name) \
inline Descriptor Name = { Binding, Set, Stage, BmRender_DescriptorType::CombinedImageSampler } \

#define START_DESCRIPTOR_SET(SetIndex, Name) \
inline DescriptorSet Name = { SetIndex, {

#define END_DESCRIPTOR_SET() \
} };

typedef glm::mat4 float4x4;
typedef glm::vec4 float4;
typedef glm::vec3 float3;
typedef glm::ivec3 uint3;
typedef glm::vec2 float2;
typedef glm::ivec2 uint2;
typedef u32 uint;

typedef glm::aligned_mat4 float4x4_16;
typedef glm::aligned_vec4 float4_16;
typedef glm::aligned_vec3 float3_16;
typedef glm::aligned_vec2 float2_8;

#else

#define DECLARE_UNIFORM_BUFFER_DYNAMIC_DESCRIPTOR(T, Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] ParameterBlock<T> Name \

#define DECLARE_STORAGE_BUFFER_DESCRIPTOR(T, Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] StructuredBuffer<T> Name \

#define DECLARE_IMAGE_SAMPLER2D_BINDLESS_DESCRIPTOR(Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] Sampler2D Name[]; \

#define DECLARE_IMAGE_SAMPLER2D_ARRAY_DESCRIPTOR(Binding, Set, Stage, Name) \
[[vk::binding(Binding, Set)]] Sampler2DArray Name; \

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
    uint pad1;
    float2 TextureCoords;
    uint2 pad2;
    float3 Normal;
    uint pad3;
};

struct StaticMeshInstance
{
    float4x4 ModelMatrix;
    uint3 pad1;
    uint MaterialIndex;
    
};

struct StaticMeshVertexInput
{
    StaticMeshVertex Vertex;
    StaticMeshInstance Instance;
};

#endif