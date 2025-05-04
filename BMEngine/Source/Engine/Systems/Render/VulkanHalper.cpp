#include "VulkanHalper.h"

#include <cassert>

namespace VulkanHelper
{
	static const u32 BUFFER_ALIGNMENT = 64;

	u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties)
	{
		VkPhysicalDeviceMemoryProperties MemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

		for (u32 MemoryTypeIndex = 0; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++)
		{
			if ((AllowedTypes & (1 << MemoryTypeIndex))	// Index of memory type must match corresponding bit in allowedTypes
				&& (MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & Properties) == Properties) // Desired property bit flags are part of memory type's property flags
			{
				return MemoryTypeIndex;
			}
		}

		assert(false);
		return 0;
	}

	u64 CalculateBufferAlignedSize(VkDevice Device, VkBuffer Buffer, u64 BufferSize)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device, Buffer, &MemoryRequirements);

		u32 Padding = 0;
		if (BufferSize % MemoryRequirements.alignment != 0)
		{
			Padding = MemoryRequirements.alignment - (BufferSize % MemoryRequirements.alignment);
		}

		return BufferSize + Padding;
	}

	u64 CalculateImageAlignedSize(VkDevice Device, VkImage Image, u64 ImageSize)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device, Image, &MemoryRequirements);

		u32 Padding = 0;
		if (ImageSize % MemoryRequirements.alignment != 0)
		{
			Padding = IMAGE_ALIGNMENT - (ImageSize % MemoryRequirements.alignment);
		}

		return ImageSize + Padding;
	}

	VkBuffer CreateBuffer(VkDevice Device, u64 Size, BufferUsageFlag Flag)
	{
		VkBufferCreateInfo MaterialBufferInfo = { };
		MaterialBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		MaterialBufferInfo.size = Size;
		MaterialBufferInfo.usage = Flag;
		MaterialBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer Buffer;
		VkResult Result = vkCreateBuffer(Device, &MaterialBufferInfo, nullptr, &Buffer);
		assert(Result == VK_SUCCESS);

		return Buffer;
	}

	VkDeviceMemory AllocateDeviceMemoryForBuffer(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer,
		u64 BufferSize, MemoryPropertyFlag Properties)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device, Buffer, &MemoryRequirements);

		const u32 MemoryTypeIndex = VulkanHelper::GetMemoryTypeIndex(PhysicalDevice, MemoryRequirements.memoryTypeBits, Properties);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = BufferSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkDeviceMemory Memory;
		VkResult Result = vkAllocateMemory(Device, &MemoryAllocInfo, nullptr, &Memory);
		assert(Result == VK_SUCCESS);

		return Memory;
	}

	VkDeviceMemory VulkanHelper::AllocateAndBindDeviceMemoryForBuffer(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer,
		u64 BufferSize, MemoryPropertyFlag Properties)
	{
		VkDeviceMemory Memory = AllocateDeviceMemoryForBuffer(PhysicalDevice, Device, Buffer, BufferSize, Properties);
		vkBindBufferMemory(Device, Buffer, Memory, 0);
		return Memory;
	}
}
