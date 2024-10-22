#include "VulkanMemoryManagementSystem.h"

#include <cassert>
#include <cstring>

#include "Util/Util.h"
#include "VulkanHelper.h"

namespace Core::VulkanMemoryManagementSystem
{
	static VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);

	static BrMemorySourceDevice MemorySource;
	static BrGPUBuffer StagingBuffer;

	void Init(BrMemorySourceDevice Device)
	{
		MemorySource = Device;
		StagingBuffer = CreateBuffer(MB64, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void Deinit()
	{
		DestroyBuffer(StagingBuffer);
	}

	VkDeviceSize CalculateBufferAlignedSize(VkDeviceSize BufferSize)
	{
		u32 Padding = 0;
		if (BufferSize % BUFFER_ALIGNMENT != 0)
		{
			Padding = BUFFER_ALIGNMENT - (BufferSize % BUFFER_ALIGNMENT);
		}

		return BufferSize + Padding;
	}

	VkDeviceSize CalculateImageAlignedSize(VkDeviceSize BufferSize)
	{
		u32 Padding = 0;
		if (BufferSize % IMAGE_ALIGNMENT != 0)
		{
			Padding = IMAGE_ALIGNMENT - (BufferSize % IMAGE_ALIGNMENT);
		}

		return BufferSize + Padding;
	}

	VkDescriptorPool AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 PoolSizeCount, u32 MaxDescriptorCount)
	{
		Util::Log().Info("Creating descriptor pool. Size count: {}", PoolSizeCount);

		for (u32 i = 0; i < PoolSizeCount; ++i)
		{
			Util::Log().Info("Type: {}, Count: {}", static_cast<int>(PoolSizes[i].type), PoolSizes[i].descriptorCount);
		}

		Util::Log().Info("Maximum descriptor count: {}", MaxDescriptorCount);

		VkDescriptorPoolCreateInfo PoolCreateInfo = { };
		PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolCreateInfo.maxSets = MaxDescriptorCount; // Maximum number of Descriptor Sets that can be created from pool
		PoolCreateInfo.poolSizeCount = PoolSizeCount; // Amount of Pool Sizes being passed
		PoolCreateInfo.pPoolSizes = PoolSizes; // Pool Sizes to create pool with

		VkDescriptorPool Pool;
		VkResult Result = vkCreateDescriptorPool(MemorySource.LogicalDevice, &PoolCreateInfo, nullptr, &Pool);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorPool result is {}", static_cast<int>(Result));
			assert(false);
		}

		return Pool;
	}

	void AllocateSets(VkDescriptorPool Pool, VkDescriptorSetLayout* Layouts,
		u32 DescriptorSetCount, VkDescriptorSet* OutSets)
	{
		Util::Log().Info("Allocating descriptor sets. Size count: {}", DescriptorSetCount);

		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = Pool; // Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = DescriptorSetCount; // Number of sets to allocate
		SetAllocInfo.pSetLayouts = Layouts; // Layouts to use to allocate sets (1:1 relationship)

		VkResult Result = vkAllocateDescriptorSets(MemorySource.LogicalDevice, &SetAllocInfo, OutSets);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateCommandBuffers result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	BrImageBuffer CreateImageBuffer(VkImageCreateInfo* pCreateInfo)
	{
		BrImageBuffer Buffer;
		VkResult Result = vkCreateImage(MemorySource.LogicalDevice, pCreateInfo, nullptr, &Buffer.Image);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("CreateImage result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(MemorySource.LogicalDevice, Buffer.Image, &MemoryRequirements);

		const u32 MemoryTypeIndex = GetMemoryTypeIndex(MemorySource.PhysicalDevice, MemoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Buffer.Memory = AllocateMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindImageMemory(MemorySource.LogicalDevice, Buffer.Image, Buffer.Memory, 0);

		return Buffer;
	}

	void DestroyImageBuffer(BrImageBuffer Image)
	{
		vkDestroyImage(MemorySource.LogicalDevice, Image.Image, nullptr);
		vkFreeMemory(MemorySource.LogicalDevice, Image.Memory, nullptr);
	}

	BrGPUBuffer CreateBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags Usage,
		VkMemoryPropertyFlags Properties)
	{
		Util::Log().Info("Creating buffer. Requested size: {}", BufferSize);

		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = BufferSize;
		BufferInfo.usage = Usage;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		BrGPUBuffer Buffer;
		VkResult Result = vkCreateBuffer(MemorySource.LogicalDevice, &BufferInfo, nullptr, &Buffer.Buffer);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateBuffer result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(MemorySource.LogicalDevice, Buffer.Buffer, &MemoryRequirements);

		const u32 MemoryTypeIndex = GetMemoryTypeIndex(MemorySource.PhysicalDevice, MemoryRequirements.memoryTypeBits,
			Properties);

		if (BufferSize != MemoryRequirements.size)
		{
			Util::Log().Warning("Buffer memory requirement size is {}, allocating {} more then buffer size",
				MemoryRequirements.size, MemoryRequirements.size - BufferSize);
		}

		Buffer.Memory = AllocateMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindBufferMemory(MemorySource.LogicalDevice, Buffer.Buffer, Buffer.Memory, 0);

		return Buffer;
	}

	void DestroyBuffer(BrGPUBuffer Buffer)
	{
		vkDestroyBuffer(MemorySource.LogicalDevice, Buffer.Buffer, nullptr);
		vkFreeMemory(MemorySource.LogicalDevice, Buffer.Memory, nullptr);
	}

	void CopyDataToMemory(VkDeviceMemory Memory,
		VkDeviceSize Offset, VkDeviceSize Size, const void* Data)
	{
		void* MappedMemory;
		vkMapMemory(MemorySource.LogicalDevice, Memory, Offset, Size, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, Size);
		vkUnmapMemory(MemorySource.LogicalDevice, Memory);
	}

	void CopyDataToBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data)
	{
		assert(Size <= MB64);

		void* MappedMemory;
		vkMapMemory(MemorySource.LogicalDevice, StagingBuffer.Memory, 0, Size, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, Size);
		vkUnmapMemory(MemorySource.LogicalDevice, StagingBuffer.Memory);

		VkCommandBuffer TransferCommandBuffer;

		VkCommandBufferAllocateInfo AllocInfo = { };
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = MemorySource.TransferCommandPool;
		AllocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(MemorySource.LogicalDevice, &AllocInfo, &TransferCommandBuffer);

		VkCommandBufferBeginInfo BeginInfo = { };
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// We're only using the command buffer once, so set up for one time submit
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo);

		VkBufferCopy BufferCopyRegion = { };
		BufferCopyRegion.srcOffset = 0;
		BufferCopyRegion.dstOffset = Offset;
		BufferCopyRegion.size = Size;

		vkCmdCopyBuffer(TransferCommandBuffer, StagingBuffer.Buffer, Buffer, 1, &BufferCopyRegion);
		vkEndCommandBuffer(TransferCommandBuffer);

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkQueueSubmit(MemorySource.TransferQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(MemorySource.TransferQueue);

		vkFreeCommandBuffers(MemorySource.LogicalDevice, MemorySource.TransferCommandPool, 1, &TransferCommandBuffer);
	}

	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, VkDeviceSize Size,
		u32 LayersCount, const void* Data)
	{
		assert(Size <= MB64);

		void* MappedMemory;
		vkMapMemory(MemorySource.LogicalDevice, StagingBuffer.Memory, 0, Size, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, Size);
		vkUnmapMemory(MemorySource.LogicalDevice, StagingBuffer.Memory);

		VkCommandBuffer TransferCommandBuffer;

		VkCommandBufferAllocateInfo AllocInfo = { };
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = MemorySource.TransferCommandPool;
		AllocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(MemorySource.LogicalDevice, &AllocInfo, &TransferCommandBuffer);

		VkCommandBufferBeginInfo BeginInfo = { };
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo);

		VkBufferImageCopy ImageRegion = { };
		ImageRegion.bufferOffset = 0;
		ImageRegion.bufferRowLength = 0;
		ImageRegion.bufferImageHeight = 0;
		ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageRegion.imageSubresource.mipLevel = 0;
		ImageRegion.imageSubresource.baseArrayLayer = 0; // Starting array layer (if array)
		ImageRegion.imageSubresource.layerCount = LayersCount;
		ImageRegion.imageOffset = { 0, 0, 0 };
		ImageRegion.imageExtent = { Width, Height, 1 };

		// Todo copy multiple regions at once?
		vkCmdCopyBufferToImage(TransferCommandBuffer, StagingBuffer.Buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);

		vkEndCommandBuffer(TransferCommandBuffer);

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkQueueSubmit(MemorySource.TransferQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(MemorySource.TransferQueue);

		vkFreeCommandBuffers(MemorySource.LogicalDevice, MemorySource.TransferCommandPool, 1, &TransferCommandBuffer);
	}

	VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex)
	{
		Util::Log().Info("Allocating Device memory. Buffer type: Image, Size count: {}, Index: {}",
			AllocationSize, MemoryTypeIndex);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = AllocationSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkDeviceMemory Memory;
		VkResult Result = vkAllocateMemory(MemorySource.LogicalDevice, &MemoryAllocInfo, nullptr, &Memory);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateMemory result is {}", static_cast<int>(Result));
			assert(false);
		}

		return Memory;
	}
}
