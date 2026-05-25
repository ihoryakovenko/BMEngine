#ifndef COMMON_H
#define COMMON_H

#include "ShaderTypes.h"

static const float PI = 3.14159265359;

[[vk::binding(0, 0)]] ConstantBuffer<Shader_FrameData> FrameBufferDescriptor;
[[vk::binding(1, 0)]] StructuredBuffer<Shader_StaticMeshVertex> VertexDescriptor;
[[vk::binding(2, 0)]] StructuredBuffer<Shader_StaticMeshInstance> InstanceDescriptor;
[[vk::binding(3, 0)]] Sampler2D AlbedoTexture[];
[[vk::binding(4, 0)]] StructuredBuffer<Shader_Material> MaterialsDescriptor;

#endif