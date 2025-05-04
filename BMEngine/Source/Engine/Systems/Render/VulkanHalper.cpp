#include "VulkanHalper.h"

#include <cassert>

namespace VulkanHelper
{
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

	VkBuffer CreateBuffer(VkDevice Device, u64 Size, BufferUsageFlag Flag)
	{
		VkBufferCreateInfo MaterialBufferInfo = { };
		MaterialBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		MaterialBufferInfo.size = Size;
		MaterialBufferInfo.usage = Flag;
		MaterialBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer Buffer;
		VkResult Result = vkCreateBuffer(Device, &MaterialBufferInfo, nullptr, &Buffer);
		if (Result != VK_SUCCESS)
		{
			assert(false);
			// TODO: 
			//HandleLog(BMRVkLogType_Error, "vkCreateBuffer result is %d", Result);
		}

		return Buffer;
	}

	VkDeviceMemory AllocateDeviceMemoryForBuffer(VkPhysicalDevice PhysicalDevice, VkDevice Device, VkBuffer Buffer,
		u64 BufferSize, MemoryPropertyFlag Properties)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device, Buffer, &MemoryRequirements);

		if (BufferSize != MemoryRequirements.size)
		{
			//assert(false);
			//HandleLog(BMRVkLogType_Warning, "Buffer memory requirement size is %d, allocating %d more then buffer size",
				//MemoryRequirements.size, MemoryRequirements.size - BufferInfo->size);
		}

		const u32 MemoryTypeIndex = VulkanHelper::GetMemoryTypeIndex(PhysicalDevice, MemoryRequirements.memoryTypeBits, Properties);

		//HandleLog(BMRVkLogType_Info, "Allocating Device memory. Buffer type: Image, Size count: %d, Index: %d",
			//AllocationSize, MemoryTypeIndex);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = BufferSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkDeviceMemory Memory;
		VkResult Result = vkAllocateMemory(Device, &MemoryAllocInfo, nullptr, &Memory);
		if (Result != VK_SUCCESS)
		{
			assert(false);
			//HandleLog(BMRVkLogType_Error, "vkAllocateMemory result is %d", Result);
		}

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
