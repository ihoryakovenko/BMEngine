#ifndef ENTITY_H
#define ENTITY_H

#include "ShaderTypes.h"
#include "Common.h"

START_DESCRIPTOR_SET(Shader_ShadowMapsDescriptorSet)
DECLARE_IMAGE_SAMPLER2D_ARRAY_DESCRIPTOR(0, 1, BmRender_DescriptorShaderStage::Fragment, ShadowMaps)
END_DESCRIPTOR_SET()

#endif 