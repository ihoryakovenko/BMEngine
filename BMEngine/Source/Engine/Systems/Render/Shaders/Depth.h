#ifndef DEPTH_H
#define DEPTH_H

#include "Common.h"

struct LightSpaceMatrixData
{
	float4x4 Matrix;
};

START_DESCRIPTOR_SET(Shader_LightSpaceMatrixDescriptorSet)
DECLARE_UNIFORM_BUFFER_DESCRIPTOR(LightSpaceMatrixData, 0, 1, BmRender_DescriptorShaderStage::Vertex, lightSpaceMatrix)
END_DESCRIPTOR_SET()

#endif