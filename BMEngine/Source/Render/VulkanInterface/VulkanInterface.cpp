#include "VulkanInterface.h"






//////////////////////////////////////
// TODO: DEPRECATED TO REFACTOR
//////////////////////////////////////









#include <cassert>
#include <cstring>
#include <stdio.h>
#include <stdarg.h>

#include <glm/glm.hpp>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/Util.h"


#include "Engine/Systems/Render/Render.h"

namespace VulkanInterface
{
	u32 GetImageCount()
	{
		return Render::GetRenderState()->CoreContext.ImagesCount;
	}

	VkImageView* GetSwapchainImageViews()
	{
		return Render::GetRenderState()->CoreContext.ImageViews;
	}

	VkImage* GetSwapchainImages()
	{
		return Render::GetRenderState()->CoreContext.Images;
	}

	u32 TestGetImageIndex()
	{
		return Render::GetRenderState()->RenderDrawState.CurrentImageIndex;
	}

	void WaitDevice()
	{
		vkDeviceWaitIdle(Render::GetRenderState()->CoreContext.LogicalDevice);
	}

	VkDescriptorSetLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages,
		const VkDescriptorBindingFlags* BindingFlags, u32 BindingCount, u32 DescriptorCount)
	{
		auto LayoutBindings = Memory::MemoryManagementSystem::FrameAlloc<VkDescriptorSetLayoutBinding>(BindingCount);
		for (u32 BindingIndex = 0; BindingIndex < BindingCount; ++BindingIndex)
		{
			VkDescriptorSetLayoutBinding* LayoutBinding = LayoutBindings + BindingIndex;
			*LayoutBinding = { };
			LayoutBinding->binding = BindingIndex;
			LayoutBinding->descriptorType = Types[BindingIndex];
			LayoutBinding->descriptorCount = DescriptorCount;
			LayoutBinding->stageFlags = Stages[BindingIndex];
			LayoutBinding->pImmutableSamplers = nullptr; // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout	
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo BindingFlagsInfo = { };
		BindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		BindingFlagsInfo.bindingCount = BindingCount;
		BindingFlagsInfo.pBindingFlags = BindingFlags;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = BindingCount;
		LayoutCreateInfo.pBindings = LayoutBindings;
		LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

		VkDescriptorSetLayout Layout;
		VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Render::GetRenderState()->CoreContext.LogicalDevice, &LayoutCreateInfo, nullptr, &Layout));
		return Layout;
	}

	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32 Count, VkDescriptorSet* OutSets)
	{
		Util::RenderLog(Util::BMRVkLogType_Info, "Allocating descriptor sets. Size count: %d", Count);

		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = GetDescriptorPool();
		SetAllocInfo.descriptorSetCount = Count;
		SetAllocInfo.pSetLayouts = Layouts;

		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Render::GetRenderState()->CoreContext.LogicalDevice, &SetAllocInfo, (VkDescriptorSet*)OutSets));
	}

	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32* DescriptorCounts, u32 Count, VkDescriptorSet* OutSets)
	{
		Util::RenderLog(Util::BMRVkLogType_Info, "Allocating descriptor sets. Size count: %d", Count);

		VkDescriptorSetVariableDescriptorCountAllocateInfo CountInfo = { };
		CountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		CountInfo.descriptorSetCount = Count;
		CountInfo.pDescriptorCounts = DescriptorCounts;

		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = GetDescriptorPool(); // Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = Count; // Number of sets to allocate
		SetAllocInfo.pSetLayouts = Layouts; // Layouts to use to allocate sets (1:1 relationship)
		SetAllocInfo.pNext = &CountInfo;

		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Render::GetRenderState()->CoreContext.LogicalDevice, &SetAllocInfo, (VkDescriptorSet*)OutSets));
	}

	VkImageView CreateImageInterface(const UniformImageInterfaceCreateInfo* InterfaceCreateInfo, VkImage Image)
	{
		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.image = Image;
		ViewCreateInfo.viewType = InterfaceCreateInfo->ViewType;
		ViewCreateInfo.format = InterfaceCreateInfo->Format;
		ViewCreateInfo.components = InterfaceCreateInfo->Components;
		ViewCreateInfo.subresourceRange = InterfaceCreateInfo->SubresourceRange;
		ViewCreateInfo.pNext = InterfaceCreateInfo->pNext;

		VkImageView ImageView;
		VULKAN_CHECK_RESULT(vkCreateImageView(Render::GetRenderState()->CoreContext.LogicalDevice, &ViewCreateInfo, nullptr, &ImageView));
		return ImageView;
	}

	void AttachUniformsToSet(VkDescriptorSet Set, const UniformSetAttachmentInfo* Infos, u32 BufferCount)
	{
		auto SetWrites = Memory::MemoryManagementSystem::FrameAlloc<VkWriteDescriptorSet>(BufferCount);
		for (u32 BufferIndex = 0; BufferIndex < BufferCount; ++BufferIndex)
		{
			const UniformSetAttachmentInfo* Info = Infos + BufferIndex;

			VkWriteDescriptorSet* SetWrite = SetWrites + BufferIndex;
			*SetWrite = { };
			SetWrite->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			SetWrite->dstSet = Set;
			SetWrite->dstBinding = BufferIndex;
			SetWrite->dstArrayElement = 0;
			SetWrite->descriptorType = Info->Type;
			SetWrite->descriptorCount = 1;
			SetWrite->pBufferInfo = &Info->BufferInfo;
			SetWrite->pImageInfo = &Info->ImageInfo;
		}

		vkUpdateDescriptorSets(Render::GetRenderState()->CoreContext.LogicalDevice, BufferCount, SetWrites, 0, nullptr);
	}


	VkDevice GetDevice()
	{
		return Render::GetRenderState()->CoreContext.LogicalDevice;
	}

	VkPhysicalDevice GetPhysicalDevice()
	{
		return Render::GetRenderState()->CoreContext.PhysicalDevice;
	}

	VkCommandBuffer GetCommandBuffer()
	{
		return Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
	}

	VkQueue GetTransferQueue()
	{
		//return TransferQueue;
		return Render::GetRenderState()->CoreContext.GraphicsQueue;
	}

	VkQueue GetGraphicsQueue()
	{
		return Render::GetRenderState()->CoreContext.GraphicsQueue;
	}

	VkFormat GetSurfaceFormat()
	{
		return Render::GetRenderState()->CoreContext.SurfaceFormat.format;
	}

	u32 GetQueueGraphicsFamilyIndex()
	{
		return Render::GetRenderState()->CoreContext.Indices.GraphicsFamily;
	}

	VkDescriptorPool GetDescriptorPool()
	{
		return Render::GetRenderState()->RenderDrawState.MainPool;
	}

	VkInstance GetInstance()
	{
		return Render::GetRenderState()->CoreContext.VulkanInstance;
	}

	VkCommandPool GetTransferCommandPool()
	{
		return Render::GetRenderState()->RenderDrawState.GraphicsCommandPool;
	}

	VkSwapchainKHR GetSwapchain()
	{
		return Render::GetRenderState()->CoreContext.VulkanSwapchain;
	}
}
