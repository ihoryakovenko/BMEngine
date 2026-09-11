#ifndef COMMON_H
#define COMMON_H

#include "ShaderTypes.h"

static const float PI = 3.14159265359;

[[vk::binding(0, 0)]] SamplerState Sampler;
[[vk::binding(1, 0)]] Texture2D Textures[];

[[vk::push_constant]]
Shader_PushData PushData;

#endif