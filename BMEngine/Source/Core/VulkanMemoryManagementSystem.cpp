#include "VulkanMemoryManagementSystem.h"

#include <cassert>

#include "Util/Util.h"
#include "VulkanHelper.h"

namespace Core
{
	static const u32 Mb64 = 1024 * 1024 * 64;

	VulkanMemoryManagementSystem* VulkanMemoryManagementSystem::Get()
	{
		static VulkanMemoryManagementSystem Instance;
		return &Instance;
	}

	void VulkanMemoryManagementSystem::Init(MemorySourceDevice Device)
	{
		MemorySource = Device;

		for (u32 i = 0; i < BufferType::Count; ++i)
		{
			BuffersMemory[i] = nullptr;
			MemoryOffsets[i] = 0;
			Buffers[i] = nullptr;

			// Shit
			// https://stackoverflow.com/questions/55445653/deriving-the-vkmemoryrequirements
			VkBufferCreateInfo BufferInfo = { };
			BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			BufferInfo.size = 1;
			BufferInfo.usage = BuffersUsage[i];
			BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkBuffer InitialBuffer = CreateBufferInternal(1, BuffersUsage[i]);

			VkMemoryRequirements MemRequirements;
			vkGetBufferMemoryRequirements(MemorySource.LogicalDevice, InitialBuffer, &MemRequirements);

			BuffersAlignment[i] = MemRequirements.alignment;
			BuffersMemoryTypeIndex[i] = GetMemoryTypeIndex(MemorySource.PhysicalDevice, MemRequirements.memoryTypeBits, BuffersProperties[i]);
		
			DestroyBuffer(InitialBuffer);
		}

		CreateStagingBuffer(Mb64);
	}

	void VulkanMemoryManagementSystem::Deinit()
	{
		vkDestroyDescriptorPool(MemorySource.LogicalDevice, Pool, nullptr);
		vkFreeMemory(MemorySource.LogicalDevice, ImageMemory, nullptr);
		
		for (u32 i = 0; i < BufferType::Count; ++i)
		{
			if (BuffersMemory[i] != nullptr)
			{
				vkFreeMemory(MemorySource.LogicalDevice, BuffersMemory[i], nullptr);
			}

			if (Buffers[i] != nullptr)
			{
				vkDestroyBuffer(MemorySource.LogicalDevice, Buffers[i], nullptr);
			}
		}

		DestroyBuffer(StagingBuffer);
		vkFreeMemory(MemorySource.LogicalDevice, StagingBufferMemory, nullptr);
	}

	VkDeviceSize VulkanMemoryManagementSystem::CalculateAlignedSize(BufferType::BufferType Type, VkDeviceSize BufferSize)
	{
		u32 Padding = 0;
		if (BufferSize % BuffersAlignment[BufferType::Vertex] != 0)
		{
			Padding = BuffersAlignment[BufferType::Vertex] -
				(BufferSize % BuffersAlignment[BufferType::Vertex]);
		}

		return BufferSize + Padding;
	}

	void VulkanMemoryManagementSystem::AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 PoolSizeCount, u32 MaxDescriptorCount)
	{
		assert(Pool == nullptr);
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

		VkResult Result = vkCreateDescriptorPool(MemorySource.LogicalDevice, &PoolCreateInfo, nullptr, &Pool);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorPool result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void VulkanMemoryManagementSystem::AllocateSets(VkDescriptorSetLayout* Layouts, u32 DescriptorSetCount, VkDescriptorSet* OutSets)
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

	void VulkanMemoryManagementSystem::AllocateImageMemory(VkDeviceSize AllocationSize,
		u32 MemoryTypeIndex)
	{
		assert(ImageMemory == nullptr);
		AllocateMemory(AllocationSize, MemoryTypeIndex, &ImageMemory);
	}

	void VulkanMemoryManagementSystem::CreateImage(VkImageCreateInfo* pCreateInfo, VkImage* Image)
	{
		VkResult Result = vkCreateImage(MemorySource.LogicalDevice, pCreateInfo, nullptr, Image);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("CreateImage result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void VulkanMemoryManagementSystem::DestroyImage(VkImage Image)
	{
		vkDestroyImage(MemorySource.LogicalDevice, Image, nullptr);
	}

	void VulkanMemoryManagementSystem::BindImageToMemory(VkImage Image, VkDeviceSize Offset)
	{
		vkBindImageMemory(MemorySource.LogicalDevice, Image, ImageMemory, Offset);
	}

	void VulkanMemoryManagementSystem::AllocateBufferMemory(BufferType::BufferType Type, VkDeviceSize Size)
	{
		assert(Size % BuffersAlignment[Type] == 0);
		AllocateMemory(Size, BuffersMemoryTypeIndex[Type], &BuffersMemory[Type]);
	}

	VkBuffer VulkanMemoryManagementSystem::CreateBuffer(BufferType::BufferType Type, VkDeviceSize BufferSize)
	{
		Util::Log().Info("Creating buffer, Type: {}. Requested size: {}", static_cast<int>(Type), BufferSize);

		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = BufferSize;
		BufferInfo.usage = BuffersUsage[Type];
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer Buffer = CreateBufferInternal(BufferSize, BuffersUsage[Type]);

		VkMemoryRequirements MemRequirements;
		vkGetBufferMemoryRequirements(MemorySource.LogicalDevice, Buffer, &MemRequirements);

		if (BufferSize != MemRequirements.size)
		{
			Util::Log().Warning("Buffer memory requirement size is {}, allocating {} more then buffer size",
				MemRequirements.size, MemRequirements.size - BufferSize);
		}

		vkBindBufferMemory(MemorySource.LogicalDevice, Buffer, BuffersMemory[Type], MemoryOffsets[Type]);
		MemoryOffsets[Type] += MemRequirements.size;

		return Buffer;
	}

	void VulkanMemoryManagementSystem::DestroyBuffer(VkBuffer Buffer)
	{
		vkDestroyBuffer(MemorySource.LogicalDevice, Buffer, nullptr);
	}

	void VulkanMemoryManagementSystem::CopyDataToMemory(BufferType::BufferType Type, VkDeviceSize Offset, VkDeviceSize Size, const void* Data)
	{
		void* MappedMemory;
		vkMapMemory(MemorySource.LogicalDevice, BuffersMemory[Type], Offset, Size, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, Size);
		vkUnmapMemory(MemorySource.LogicalDevice, BuffersMemory[Type]);
	}

	void VulkanMemoryManagementSystem::CopyDataToBuffer(BufferType::BufferType Type, VkDeviceSize Size,
		const void* Data)
	{
		assert(Size <= Mb64);

		void* MappedMemory;
		vkMapMemory(MemorySource.LogicalDevice, StagingBufferMemory, 0, Size, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, Size);
		vkUnmapMemory(MemorySource.LogicalDevice, StagingBufferMemory);

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
		BufferCopyRegion.dstOffset = BuffersOffset[Type];
		BufferCopyRegion.size = Size;

		vkCmdCopyBuffer(TransferCommandBuffer, StagingBuffer, Buffers[Type], 1, &BufferCopyRegion);
		vkEndCommandBuffer(TransferCommandBuffer);

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkQueueSubmit(MemorySource.TransferQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(MemorySource.TransferQueue);

		vkFreeCommandBuffers(MemorySource.LogicalDevice, MemorySource.TransferCommandPool, 1, &TransferCommandBuffer);

		BuffersOffset[Type] += Size;
	}

	VkBuffer VulkanMemoryManagementSystem::CreateBufferInternal(VkDeviceSize BufferSize, VkBufferUsageFlags BufferUsage)
	{
		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = BufferSize;
		BufferInfo.usage = BufferUsage;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer Buffer;
		VkResult Result = vkCreateBuffer(MemorySource.LogicalDevice, &BufferInfo, nullptr, &Buffer);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateBuffer result is {}", static_cast<int>(Result));
			assert(false);
		}

		return Buffer;
	}

	void VulkanMemoryManagementSystem::CreateStagingBuffer(VkDeviceSize Size)
	{
		const VkMemoryPropertyFlags Properties =  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		StagingBuffer = CreateBufferInternal(Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		VkMemoryRequirements MemRequirements;
		vkGetBufferMemoryRequirements(MemorySource.LogicalDevice, StagingBuffer, &MemRequirements);

		const u32 Index = GetMemoryTypeIndex(MemorySource.PhysicalDevice, MemRequirements.memoryTypeBits, Properties);
		AllocateMemory(Size, Index, &StagingBufferMemory);

		vkBindBufferMemory(MemorySource.LogicalDevice, StagingBuffer, StagingBufferMemory, 0);
	}

	void VulkanMemoryManagementSystem::AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex,
		VkDeviceMemory* Memory)
	{
		assert(*Memory == nullptr);
		Util::Log().Info("Allocating Device memory. Buffer type: Image, Size count: {}, Index: {}",
			AllocationSize, MemoryTypeIndex);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = AllocationSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkResult Result = vkAllocateMemory(MemorySource.LogicalDevice, &MemoryAllocInfo, nullptr, Memory);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateMemory result is {}", static_cast<int>(Result));
			assert(false);
		}
	}
}
