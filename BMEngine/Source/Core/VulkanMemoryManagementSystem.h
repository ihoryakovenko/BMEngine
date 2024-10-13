#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>

#include "Util/EngineTypes.h"

namespace Core
{
	namespace BufferType
	{
		enum BufferType
		{
			Vertex = 0,
			Index,
			Uniform,

			Count,
			None = -1
		};
	};

	struct MemorySourceDevice
	{
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;
	};

	class VulkanMemoryManagementSystem
	{
	public:
		static VulkanMemoryManagementSystem* Get();

		void Init(MemorySourceDevice Device);
		void Deinit();

		void AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 PoolSizeCount, u32 MaxDescriptorCount);
		void AllocateSets(VkDescriptorSetLayout* Layouts, u32 DescriptorSetCount, VkDescriptorSet* OutSets);

		void AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex, VkDeviceMemory* Memory);

		void AllocateImageMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);
		void CreateImage(VkImageCreateInfo* pCreateInfo, VkImage* Image);
		void DestroyImage(VkImage Image);
		void BindImageToMemory(VkImage Image, VkDeviceSize Offset);

		void AllocateBufferMemory(BufferType::BufferType Type, VkDeviceSize Size);
		VkBuffer CreateBuffer(BufferType::BufferType Type, VkDeviceSize BufferSize);
		void DestroyBuffer(VkBuffer Buffer);

		template <typename T>
		void CopyDataToMemory(BufferType::BufferType Type, VkDeviceSize Offset, VkDeviceSize Size, T* Data)
		{
			void* MappedMemory;
			vkMapMemory(MemorySource.LogicalDevice, BuffersMemory[Type], Offset, Size, 0, &MappedMemory);
			std::memcpy(MappedMemory, Data, Size);
			vkUnmapMemory(MemorySource.LogicalDevice, BuffersMemory[Type]);
		}

	public:
		u32 BuffersAlignment[BufferType::Count];
		u32 BuffersMemoryTypeIndex[BufferType::Count];

	private:
		MemorySourceDevice MemorySource;

		VkDescriptorPool Pool = nullptr;

		VkDeviceMemory ImageMemory = nullptr;

		VkDeviceMemory BuffersMemory[BufferType::Count];
		u32 BuffersOffset[BufferType::Count];

		VkBufferUsageFlags BuffersUsage[BufferType::Count] =
		{
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
		};

		VkMemoryPropertyFlags BuffersProperties[BufferType::Count] =
		{
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};
	};
}