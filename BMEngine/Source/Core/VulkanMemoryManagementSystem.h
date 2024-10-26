#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
	void AllocateSets(VkDescriptorPool Pool, VkDescriptorSetLayout* Layouts,
		u32 DescriptorSetCount, VkDescriptorSet* OutSets);

	BMRImageBuffer CreateImageBuffer(VkImageCreateInfo* pCreateInfo);
	void DestroyImageBuffer(BMRImageBuffer Image);

	BMRGPUBuffer CreateBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags Usage,
		VkMemoryPropertyFlags Properties);
	void DestroyBuffer(BMRGPUBuffer Buffer);

	void CopyDataToMemory(VkDeviceMemory Memory, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
	void CopyDataToBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, VkDeviceSize Size, u32 LayersCount, const void* Data);
}