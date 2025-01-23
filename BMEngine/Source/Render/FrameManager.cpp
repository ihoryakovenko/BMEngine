#include "FrameManager.h"

#include <vulkan/vulkan.h>

#include "VulkanInterface/VulkanInterface.h"

namespace FrameManager
{
	static VkDescriptorSetLayout VpLayout = nullptr;
	static VulkanInterface::UniformBuffer VpBuffer[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT] = { };
	static VkDescriptorSet VpSet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT] = { };

	void Init()
	{
		VkDescriptorType VpDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VkShaderStageFlags VpStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		VpLayout = VulkanInterface::CreateUniformLayout(&VpDescriptorType, &VpStageFlags, 1);

		const VkDeviceSize VpBufferSize = sizeof(ViewProjectionBuffer);

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VpBuffer[i] = VulkanInterface::CreateUniformBuffer(VpBufferSize);
			VulkanInterface::CreateUniformSets(&VpLayout, 1, VpSet + i);

			VulkanInterface::UniformSetAttachmentInfo VpBufferAttachmentInfo;
			VpBufferAttachmentInfo.BufferInfo.buffer = VpBuffer[i].Buffer;
			VpBufferAttachmentInfo.BufferInfo.offset = 0;
			VpBufferAttachmentInfo.BufferInfo.range = VpBufferSize;
			VpBufferAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			VulkanInterface::AttachUniformsToSet(VpSet[i], &VpBufferAttachmentInfo, 1);
		}
	}

	void Update(const ViewProjectionBuffer* Data)
	{
		VulkanInterface::UpdateUniformBuffer(VpBuffer[VulkanInterface::TestGetImageIndex()], sizeof(ViewProjectionBuffer), 0,
			Data);
	}

	void DeInit()
	{
		VulkanInterface::DestroyUniformLayout(VpLayout);

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VulkanInterface::DestroyUniformBuffer(VpBuffer[i]);
		}
	}

	VkDescriptorSetLayout GetViewProjectionLayout()
	{
		return VpLayout;
	}

	VkDescriptorSet* GetViewProjectionSet()
	{
		return VpSet;
	}
}
