#pragma once

#include <vulkan/vulkan.h>

#include "VulkanCoreTypes.h"
#include "Util/EngineTypes.h"

namespace BMR::VulkanMemoryManagementSystem
{
	struct BMRMemorySourceDevice
	{
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;
		VkCommandPool TransferCommandPool = nullptr;
		VkQueue TransferQueue = nullptr;
	};

	void Init(BMRMemorySourceDevice Device);
	void Deinit();

	VkDeviceSize CalculateBufferAlignedSize(VkDeviceSize BufferSize);
	VkDeviceSize CalculateImageAlignedSize(VkDeviceSize BufferSize);

	VkDescriptorPool AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 PoolSizeCount, u32 MaxDescriptorCount);


	void CopyDataToBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, VkDeviceSize AlignedLayerSize,
		u32 LayersCount, void* Data);
}