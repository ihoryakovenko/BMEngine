#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>

#include "VulkanCoreTypes.h"

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
			Texture_2D,

			Count,
			None = -1
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

		VkDeviceSize CalculateAlignedSize(BufferType::BufferType Type, VkDeviceSize BufferSize);

		void AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 PoolSizeCount, u32 MaxDescriptorCount);
		void AllocateSets(VkDescriptorSetLayout* Layouts, u32 DescriptorSetCount, VkDescriptorSet* OutSets);

		void AllocateImageMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);
		void CreateImage(VkImageCreateInfo* pCreateInfo, VkImage* Image);
		void DestroyImage(VkImage Image);
		void BindImageToMemory(VkImage Image, VkDeviceSize Offset);

		void AllocateBufferMemory(BufferType::BufferType Type, VkDeviceSize Size);
		VkBuffer CreateBuffer(BufferType::BufferType Type, VkDeviceSize BufferSize);
		void DestroyBuffer(VkBuffer Buffer);

		void CopyDataToMemory(BufferType::BufferType Type, VkDeviceSize Offset, VkDeviceSize Size, const void* Data);
		void CopyDataToBuffer(BufferType::BufferType Type, VkDeviceSize Size, const void* Data);
		void CopyDataToImage(VkImage Image, u32 Width, u32 Height, VkDeviceSize Size, u32 LayersCount, const void* Data);

	private:
		VkBuffer CreateBufferInternal(VkDeviceSize BufferSize, VkBufferUsageFlags BufferUsage);
		void CreateStagingBuffer(VkDeviceSize Size);
		void AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex, VkDeviceMemory* Memory);

	public:
		static inline u32 BuffersAlignment[BufferType::Count];
		static inline u32 BuffersMemoryTypeIndex[BufferType::Count];
		static inline VkBuffer Buffers[BufferType::Count];
		static inline u32 BuffersOffset[BufferType::Count];

		static inline u32 ImagesMemoryAlignment[MAX_IMAGES];
		static inline u32 ImagesMemoryTypeIndex[MAX_IMAGES];
		static inline VkImage Images[MAX_IMAGES];

	private:
		static inline MemorySourceDevice MemorySource;

		static inline VkBuffer StagingBuffer = nullptr;
		static inline VkDeviceMemory StagingBufferMemory = nullptr;

		static inline VkDescriptorPool Pool = nullptr;

		static inline VkDeviceMemory ImagesMemory[MAX_IMAGES];
		static inline u32 ImagesMemoryCount = 0;

		static inline VkDeviceMemory BuffersMemory[BufferType::Count];
		static inline u32 MemoryOffsets[BufferType::Count];

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