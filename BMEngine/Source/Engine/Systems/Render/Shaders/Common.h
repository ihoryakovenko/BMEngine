#ifndef COMMON_H
#define COMMON_H

#include "ShaderTypes.h"

static const float PI = 3.14159265359;

DECLARE_UNIFORM_BUFFER_DYNAMIC_DESCRIPTOR(FrameData, 0, 0, BmRender_DescriptorShaderStage::Vertex | BmRender_DescriptorShaderStage::Fragment, FrameBufferDescriptor);
DECLARE_IMAGE_SAMPLER2D_BINDLESS_DESCRIPTOR(0, 1, BmRender_DescriptorShaderStage::Fragment, AlbedoTexture);
DECLARE_STORAGE_BUFFER_DESCRIPTOR(Material, 0, 2, BmRender_DescriptorShaderStage::Vertex | BmRender_DescriptorShaderStage::Fragment, MaterialsDescriptor);
DECLARE_IMAGE_SAMPLER2D_ARRAY_DESCRIPTOR(0, 3, BmRender_DescriptorShaderStage::Fragment, ShadowMaps);
DECLARE_STORAGE_BUFFER_DESCRIPTOR(StaticMeshVertex, 0, 4, BmRender_DescriptorShaderStage::Vertex, VertexDescriptor);
DECLARE_STORAGE_BUFFER_DESCRIPTOR(StaticMeshInstance, 1, 4, BmRender_DescriptorShaderStage::Vertex, InstanceDescriptor);

#endif