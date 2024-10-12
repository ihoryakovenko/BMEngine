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

		void AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, uint32_t PoolSizeCount, uint32_t MaxDescriptorCount);
		void AllocateSets(VkDescriptorSetLayout* Layouts, uint32_t DescriptorSetCount, VkDescriptorSet* OutSets);

		void AllocateMemory(VkDeviceSize AllocationSize, uint32_t MemoryTypeIndex, VkDeviceMemory* Memory);

		void AllocateImageMemory(VkDeviceSize AllocationSize, uint32_t MemoryTypeIndex);
		void CreateImage(VkImageCreateInfo* pCreateInfo, VkImage* Image);
		void DestroyImage(VkImage Image);
		void BindImageToMemory(VkImage Image, VkDeviceSize Offset);

		VkBuffer CreateBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags BufferUsage, VkMemoryPropertyFlags BufferProperties);
		void DestroyBuffer(VkBuffer Buffer);
		void BindBufferToMemory(VkBuffer Buffer, BufferType::BufferType Type, VkDeviceSize Offset);

		template <typename T>
		void CopyDataToMemory(BufferType::BufferType Type, VkDeviceSize Offset, VkDeviceSize Size, T* Data)
		{
			void* MappedMemory;
			vkMapMemory(MemorySource.LogicalDevice, BuffersMemory[Type], Offset, Size, 0, &MappedMemory);
			std::memcpy(MappedMemory, Data, Size);
			vkUnmapMemory(MemorySource.LogicalDevice, BuffersMemory[Type]);
		}

		void CreateUniformBuffers(VkDeviceSize* BufferSizes, VkBuffer* OutBuffers, u32 BuffersCount);

	private:
		void AllocateBufferMemory(BufferType::BufferType Type, VkDeviceSize AllocationSize, uint32_t MemoryTypeIndex);

	private:
		MemorySourceDevice MemorySource;

		VkDescriptorPool Pool = nullptr;
		VkDeviceMemory BuffersMemory[BufferType::Count];
		VkDeviceMemory ImageMemory = nullptr;
	};
}