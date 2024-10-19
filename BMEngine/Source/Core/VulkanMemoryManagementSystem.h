#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>

#include "VulkanCoreTypes.h"

#include "Util/EngineTypes.h"

namespace Core
{
	struct MemorySourceDevice
	{
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;
		VkCommandPool TransferCommandPool = nullptr;
		VkQueue TransferQueue = nullptr;
	};

	class VulkanMemoryManagementSystem
	{
	public:
		static VulkanMemoryManagementSystem* Get();

		void Init(MemorySourceDevice Device);
		void Deinit();

		VkDeviceSize CalculateBufferAlignedSize(VkDeviceSize BufferSize);
		VkDeviceSize CalculateImageAlignedSize(VkDeviceSize BufferSize);

		VkDescriptorPool AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 PoolSizeCount, u32 MaxDescriptorCount);
		void AllocateSets(VkDescriptorPool Pool, VkDescriptorSetLayout* Layouts,
			u32 DescriptorSetCount, VkDescriptorSet* OutSets);

		ImageBuffer CreateImageBuffer(VkImageCreateInfo* pCreateInfo);
		void DestroyImageBuffer(ImageBuffer Image);

		GPUBuffer CreateBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags Usage,
			VkMemoryPropertyFlags Properties);
		void DestroyBuffer(GPUBuffer Buffer);

		void CopyDataToMemory(VkDeviceMemory Memory, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
		void CopyDataToBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
		void CopyDataToImage(VkImage Image, u32 Width, u32 Height, VkDeviceSize Size, u32 LayersCount, const void* Data);

	private:
		VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);

	private:
		static inline MemorySourceDevice MemorySource;
		static inline GPUBuffer StagingBuffer;
	};
}