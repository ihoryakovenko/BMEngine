#ifndef ENTITY_H
#define ENTITY_H

#include "ShaderTypes.h"
#include "Common.h"

START_DESCRIPTOR_SET(Shader_AlbedoTextureDescriptorSet)
DECLARE_IMAGE_SAMPLER2D_BINDLESS_DESCRIPTOR(0, 1, BmRender_DescriptorShaderStage::Fragment, AlbedoTexture)
END_DESCRIPTOR_SET()

START_DESCRIPTOR_SET(Shader_MaterialsDescriptorSet)
DECLARE_STORAGE_BUFFER_DESCRIPTOR(Shader_Material, 0, 2, BmRender_DescriptorShaderStage::Vertex | BmRender_DescriptorShaderStage::Fragment, MaterialsDescriptor)
END_DESCRIPTOR_SET()

START_DESCRIPTOR_SET(Shader_ShadowMapsDescriptorSet)
DECLARE_IMAGE_SAMPLER2D_ARRAY_DESCRIPTOR(0, 3, BmRender_DescriptorShaderStage::Fragment, ShadowMaps)
END_DESCRIPTOR_SET()

#endif 