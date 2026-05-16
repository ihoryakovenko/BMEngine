#ifndef COMMON_H
#define COMMON_H

#include "ShaderTypes.h"

static const float PI = 3.14159265359;
static const int MAX_SHADOW_TEXTURES = 2;

START_DESCRIPTOR_SET(Shader_FrameBufferDescriptorSet)
	DECLARE_UNIFORM_BUFFER_DYNAMIC_DESCRIPTOR(Shader_FrameData, 0, 0, BmRender_DescriptorShaderStage::Vertex | BmRender_DescriptorShaderStage::Fragment, FrameBufferDescriptor) DESCRIPTOR_AND
	DECLARE_STORAGE_BUFFER_DESCRIPTOR(Shader_StaticMeshVertex, 1, 0, BmRender_DescriptorShaderStage::Vertex, VertexDescriptor) DESCRIPTOR_AND
	DECLARE_STORAGE_BUFFER_DESCRIPTOR(Shader_StaticMeshInstance, 2, 0, BmRender_DescriptorShaderStage::Vertex, InstanceDescriptor)
END_DESCRIPTOR_SET()

#endif