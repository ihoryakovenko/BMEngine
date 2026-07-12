#ifndef COMMON_H
#define COMMON_H

#include "ShaderTypes.h"

static const float PI = 3.14159265359;

[[vk::binding(0, 0)]] ConstantBuffer<Shader_FrameData> FrameBufferDescriptor;
[[vk::binding(1, 0)]] StructuredBuffer<Shader_StaticMeshVertex> VertexDescriptor;
[[vk::binding(2, 0)]] StructuredBuffer<Shader_StaticMeshInstance> InstanceDescriptor;
[[vk::binding(3, 0)]] Texture2D Textures[];
[[vk::binding(4, 0)]] SamplerState Sampler;
[[vk::binding(5, 0)]] StructuredBuffer<Shader_Material> MaterialsDescriptor;

#endif