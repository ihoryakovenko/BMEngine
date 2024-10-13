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

	namespace ImageType
	{
		enum ImageType
		{

		};
	};

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

		void CopyDataToMemory(BufferType::BufferType Type, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
		void CopyDataToBuffer(VkBuffer SrcBuffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);

	private:
		VkBuffer CreateBufferInternal(VkDeviceSize BufferSize, VkBufferUsageFlags BufferUsage);
		void CreateStagingBuffer(VkDeviceSize Size);

	public:
		static inline u32 BuffersAlignment[BufferType::Count];
		static inline u32 BuffersMemoryTypeIndex[BufferType::Count];

	private:
		static inline MemorySourceDevice MemorySource;

		static inline VkBuffer StagingBuffer = nullptr;
		static inline VkDeviceMemory StagingBufferMemory = nullptr;

		static inline VkDescriptorPool Pool = nullptr;

		static inline VkDeviceMemory ImageMemory = nullptr;

		static inline VkDeviceMemory BuffersMemory[BufferType::Count];
		static inline u32 BuffersOffset[BufferType::Count];

		static inline VkBufferUsageFlags BuffersUsage[BufferType::Count] =
		{
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
		};

		static inline VkMemoryPropertyFlags BuffersProperties[BufferType::Count] =
		{
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};
	};
}