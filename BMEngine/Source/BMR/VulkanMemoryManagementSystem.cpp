#include "VulkanMemoryManagementSystem.h"

#include <cassert>
#include <cstring>

#include "VulkanHelper.h"

#include "BMRInterface.h"

namespace BMR::VulkanMemoryManagementSystem
{
	static VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);

	static BMRMemorySourceDevice MemorySource;
	static BMRUniformBuffer StagingBuffer;

	void Init(BMRMemorySourceDevice Device)
	{
		MemorySource = Device;
		StagingBuffer = CreateUniformBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, MB128);
	}

	void Deinit()
	{
		DestroyUniformBuffer(StagingBuffer);
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
		HandleLog(BMRLogType::LogType_Info, "Creating descriptor pool. Size count: %d", PoolSizeCount);

		for (u32 i = 0; i < PoolSizeCount; ++i)
		{
			HandleLog(BMRLogType::LogType_Info, "Type: %d, Count: %d", PoolSizes[i].type, PoolSizes[i].descriptorCount);
		}

		HandleLog(BMRLogType::LogType_Info, "Maximum descriptor count: %d", MaxDescriptorCount);

		VkDescriptorPoolCreateInfo PoolCreateInfo = { };
		PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolCreateInfo.maxSets = MaxDescriptorCount; // Maximum number of Descriptor Sets that can be created from pool
		PoolCreateInfo.poolSizeCount = PoolSizeCount; // Amount of Pool Sizes being passed
		PoolCreateInfo.pPoolSizes = PoolSizes; // Pool Sizes to create pool with

		VkDescriptorPool Pool;
		VkResult Result = vkCreateDescriptorPool(MemorySource.LogicalDevice, &PoolCreateInfo, nullptr, &Pool);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateDescriptorPool result is %d", Result);
		}

		return Pool;
	}

	void AllocateSets(VkDescriptorPool Pool, VkDescriptorSetLayout* Layouts,
		u32 DescriptorSetCount, VkDescriptorSet* OutSets)
	{
		HandleLog(BMRLogType::LogType_Info, "Allocating descriptor sets. Size count: %d", DescriptorSetCount);

		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = Pool; // Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = DescriptorSetCount; // Number of sets to allocate
		SetAllocInfo.pSetLayouts = Layouts; // Layouts to use to allocate sets (1:1 relationship)

		VkResult Result = vkAllocateDescriptorSets(MemorySource.LogicalDevice, &SetAllocInfo, OutSets);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkAllocateDescriptorSets result is %d", Result);
		}
	}

	BMRUniformImage CreateImageBuffer(VkImageCreateInfo* pCreateInfo)
	{
		BMRUniformImage Buffer;
		VkResult Result = vkCreateImage(MemorySource.LogicalDevice, pCreateInfo, nullptr, &Buffer.Image);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "CreateImage result is %d", Result);
		}

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(MemorySource.LogicalDevice, Buffer.Image, &MemoryRequirements);

		const u32 MemoryTypeIndex = GetMemoryTypeIndex(MemorySource.PhysicalDevice, MemoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Buffer.Memory = AllocateMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindImageMemory(MemorySource.LogicalDevice, Buffer.Image, Buffer.Memory, 0);

		return Buffer;
	}

	void DestroyImageBuffer(BMRUniformImage Image)
	{
		vkDestroyImage(MemorySource.LogicalDevice, Image.Image, nullptr);
		vkFreeMemory(MemorySource.LogicalDevice, Image.Memory, nullptr);
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

	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, VkDeviceSize AlignedLayerSize,
		u32 LayersCount, void* Data)
	{
		const VkDeviceSize TotalAlignedSize = AlignedLayerSize * LayersCount;
		assert(TotalAlignedSize <= MB128);

		const VkDeviceSize ActualLayerSize = Width * Height * Format; // Should be TotalAlignedSize in ideal (assert?)
		if (ActualLayerSize != AlignedLayerSize)
		{
			HandleLog(BMRLogType::LogType_Warning, "Image memory requirement size for layer is %d, actual size is %d",
				AlignedLayerSize, ActualLayerSize);
		}

		VkDeviceSize CopyOffset = 0;

		void* MappedMemory;
		vkMapMemory(MemorySource.LogicalDevice, StagingBuffer.Memory, 0, TotalAlignedSize, 0, &MappedMemory);

		for (u32 i = 0; i < LayersCount; ++i)
		{
			std::memcpy(static_cast<u8*>(MappedMemory) + CopyOffset, reinterpret_cast<u8**>(Data)[i], ActualLayerSize);
			CopyOffset += AlignedLayerSize;
		}

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
		HandleLog(BMRLogType::LogType_Info, "Allocating Device memory. Buffer type: Image, Size count: %d, Index: %d",
			AllocationSize, MemoryTypeIndex);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = AllocationSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkDeviceMemory Memory;
		VkResult Result = vkAllocateMemory(MemorySource.LogicalDevice, &MemoryAllocInfo, nullptr, &Memory);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkAllocateMemory result is %d", Result);
		}

		return Memory;
	}
}
