#ifndef DEFERRED_H
#define DEFERRED_H

#include "Common.h"

START_DESCRIPTOR_SET(Shader_DeferredInputDescriptorSet)
DECLARE_IMAGE_SAMPLER2D_DESCRIPTOR(0, 1, BmRender_DescriptorShaderStage::Fragment, InputColor) DESCRIPTOR_AND
DECLARE_IMAGE_SAMPLER2D_DESCRIPTOR(1, 1, BmRender_DescriptorShaderStage::Fragment, InputDepth)
END_DESCRIPTOR_SET()

#endif