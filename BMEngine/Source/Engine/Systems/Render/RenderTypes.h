#pragma once

#include <vulkan/vulkan.h>
//#include "Engine/Systems/HandleManager.h"

struct SamplerData
{
	VkSampler VulkanSampler;
};

//using BmRHI_Sampler = HandleManager::Handle<SamplerData>;
using BmRHI_Sampler = int;