#include "VulkanMemoryManagementSystem.h"

#include <cassert>

#include "Util/Util.h"
#include "VulkanHelper.h"

namespace Core
{
	VulkanMemoryManagementSystem* VulkanMemoryManagementSystem::Get()
	{
		static VulkanMemoryManagementSystem Instance;
		return &Instance;
	}

	void VulkanMemoryManagementSystem::Init(MemorySourceDevice Device)
	{
		MemorySource = Device;

		for (uint32_t i = 0; i < BufferType::Count; ++i)
		{
			BuffersMemory[i] = nullptr;
		}
	}

	void VulkanMemoryManagementSystem::Deinit()
	{
		vkDestroyDescriptorPool(MemorySource.LogicalDevice, Pool, nullptr);
		vkFreeMemory(MemorySource.LogicalDevice, ImageMemory, nullptr);
		
		for (uint32_t i = 0; i < BufferType::Count; ++i)
		{
			if (BuffersMemory[i] != nullptr)
			{
				vkFreeMemory(MemorySource.LogicalDevice, BuffersMemory[i], nullptr);
			}
		}
	}

	void VulkanMemoryManagementSystem::AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, uint32_t PoolSizeCount, uint32_t MaxDescriptorCount)
	{
		assert(Pool == nullptr);
		Util::Log().Info("Creating descriptor pool. Size count: {}", PoolSizeCount);

		for (uint32_t i = 0; i < PoolSizeCount; ++i)
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

	void VulkanMemoryManagementSystem::AllocateSets(VkDescriptorSetLayout* Layouts, uint32_t DescriptorSetCount, VkDescriptorSet* OutSets)
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
		uint32_t MemoryTypeIndex)
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

	void VulkanMemoryManagementSystem::AllocateBufferMemory(BufferType::BufferType Type, VkDeviceSize AllocationSize,
		uint32_t MemoryTypeIndex)
	{
		assert(Type < BufferType::Count);
		assert(BuffersMemory[Type] == nullptr);
		AllocateMemory(AllocationSize, MemoryTypeIndex, &BuffersMemory[Type]);
	}

	VkBuffer VulkanMemoryManagementSystem::CreateBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags BufferUsage,
		VkMemoryPropertyFlags BufferProperties)
	{
		VkBufferCreateInfo bufferInfo = { };
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = BufferSize;
		bufferInfo.usage = BufferUsage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer Buffer;
		VkResult Result = vkCreateBuffer(MemorySource.LogicalDevice, &bufferInfo, nullptr, &Buffer);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateBuffer result is {}", static_cast<int>(Result));
			assert(false);
		}

		return Buffer;
	}

	void VulkanMemoryManagementSystem::DestroyBuffer(VkBuffer Buffer)
	{
		vkDestroyBuffer(MemorySource.LogicalDevice, Buffer, nullptr);
	}

	void VulkanMemoryManagementSystem::BindBufferToMemory(VkBuffer Buffer, BufferType::BufferType Type, VkDeviceSize Offset)
	{
		vkBindBufferMemory(MemorySource.LogicalDevice, Buffer, BuffersMemory[Type], Offset);
	}

	void VulkanMemoryManagementSystem::CreateUniformBuffers(VkDeviceSize* BufferSizes, VkBuffer* OutBuffers,
		u32 BuffersCount)
	{
		const VkMemoryPropertyFlags BufferProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		VkDeviceSize TotalSize = 0;
		for (u32 i = 0; i < BuffersCount; ++i)
		{
			OutBuffers[i] = CreateBuffer(BufferSizes[i], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, BufferProperties);
			TotalSize += BufferSizes[i];
		}

		VkMemoryRequirements MemRequirements;
		vkGetBufferMemoryRequirements(MemorySource.LogicalDevice, OutBuffers[0], &MemRequirements);

		const u32 Index = GetMemoryTypeIndex(MemorySource.PhysicalDevice, MemRequirements.memoryTypeBits, BufferProperties);

		assert(BuffersMemory[BufferType::Uniform] == nullptr);
		AllocateMemory(TotalSize, Index, &(BuffersMemory[BufferType::Uniform]));

		VkDeviceSize Offset = 0;
		for (u32 i = 0; i < BuffersCount; ++i)
		{
			vkBindBufferMemory(MemorySource.LogicalDevice, OutBuffers[i], BuffersMemory[BufferType::Uniform], Offset);
			Offset += BufferSizes[i];
		}
	}

	void VulkanMemoryManagementSystem::AllocateMemory(VkDeviceSize AllocationSize, uint32_t MemoryTypeIndex,
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
