#pragma once

#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

namespace VulkanHelper
{
	enum BufferUsageFlag
	{
		Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	};

	enum MemoryPropertyFlag
	{
		GPULocal = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		HostCompatible = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties);

	VkDeviceMemory AllocateDeviceMemoryForBuffer(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer,
		u64 BufferSize, MemoryPropertyFlag Properties);
	VkDeviceMemory AllocateAndBindDeviceMemoryForBuffer(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer,
		u64 BufferSize, MemoryPropertyFlag Properties);

	VkBuffer CreateBuffer(VkDevice Device, u64 Size, BufferUsageFlag Flag);
}