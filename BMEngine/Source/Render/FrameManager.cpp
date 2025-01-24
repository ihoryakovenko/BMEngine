#include "FrameManager.h"

#include <vulkan/vulkan.h>

#include "VulkanInterface/VulkanInterface.h"

namespace FrameManager
{
	static const u64 BufferSingleFrameSize = 1024 * 1024 * 10;
	static u64 BufferMultiFrameSize;

	static VulkanInterface::UniformBuffer Buffer;
	static u64 Offset;

	static VkDescriptorSetLayout VpLayout;
	static u64 VpOffset;
	static VkDescriptorSet VpSet;

	void Init()
	{
		BufferMultiFrameSize = BufferSingleFrameSize * VulkanInterface::GetImageCount();

		const VkDeviceSize VpBufferSize = sizeof(ViewProjectionBuffer);
		Buffer = VulkanInterface::CreateUniformBuffer(BufferMultiFrameSize);

		const VkDescriptorType VpDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		const VkShaderStageFlags VpStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		VpLayout = VulkanInterface::CreateUniformLayout(&VpDescriptorType, &VpStageFlags, 1);

		VulkanInterface::CreateUniformSets(&VpLayout, 1, &VpSet);

		VulkanInterface::UniformSetAttachmentInfo VpBufferAttachmentInfo;
		VpBufferAttachmentInfo.BufferInfo.buffer = Buffer.Buffer;
		VpBufferAttachmentInfo.BufferInfo.offset = 0;
		VpBufferAttachmentInfo.BufferInfo.range = VpBufferSize;
		VpBufferAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

		VulkanInterface::AttachUniformsToSet(VpSet, &VpBufferAttachmentInfo, 1);
	}

	void DeInit()
	{
		VulkanInterface::DestroyUniformLayout(VpLayout);
		VulkanInterface::DestroyUniformBuffer(Buffer);
	}

	void UpdateViewProjection(const ViewProjectionBuffer* Data)
	{
		UpdateUniformMemory(VpOffset, Data, sizeof(ViewProjectionBuffer));
	}

	u64 ReserveUniformMemory(u64 Size)
	{
		const u64 OldOffset = Offset;
		Offset += Size * VulkanInterface::GetImageCount();
		return OldOffset;
	}

	void UpdateUniformMemory(u64 Offset, const void* Data, u64 Size)
	{
		VulkanInterface::UpdateUniformBuffer(Buffer, Size,
			Offset + (Size * VulkanInterface::TestGetImageIndex()), Data);
	}

	VkDescriptorSetLayout GetViewProjectionLayout()
	{
		return VpLayout;
	}

	VkDescriptorSet GetViewProjectionSet()
	{
		return VpSet;
	}
}
