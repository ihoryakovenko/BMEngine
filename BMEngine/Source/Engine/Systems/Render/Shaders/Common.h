#ifndef COMMON_H
#define COMMON_H

#include "ShaderTypes.h"

typedef ParameterBlock<FrameData> FrameBufferBlock;
typedef StructuredBuffer<Material> MaterialsStructuredBuffer;

static const float PI = 3.14159265359;

[[vk::binding(0, 0)]] INLINE_GLOBAL FrameBufferBlock FrameBufferDescriptor;

[[vk::binding(0, 2)]] INLINE_GLOBAL MaterialsStructuredBuffer MaterialsDescriptor;

#endif