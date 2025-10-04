#pragma once





//////////////////////////////////////
// TODO: DEPRECATED TO REFACTOR
//////////////////////////////////////








#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

#include <mutex>

namespace VulkanInterface
{
	struct UniformBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};

	struct UniformImage
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};
}
