#ifndef COMMON_SLANG
#define COMMON_SLANG

#include "ShaderTypes.h"

static const float PI = 3.14159265359;

[[vk::binding(0, 0)]] ParameterBlock<FrameBuffer> FrameData;

#endif