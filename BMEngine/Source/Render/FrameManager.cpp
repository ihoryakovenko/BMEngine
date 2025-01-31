#include "FrameManager.h"

#include <vulkan/vulkan.h>

#include "VulkanInterface/VulkanInterface.h"

namespace FrameManager
{
	static const u64 BufferSingleFrameSize = 1024 * 1024 * 10;
	static u64 BufferMultiFrameSize;

	static const VkDescriptorType BufferType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	static VulkanInterface::UniformBuffer Buffer;
	static u64 BufferAlignment;
	static u64 NextUniformMemoryHandle;

	static VkDescriptorSetLayout VpLayout;
	static UniformMemoryHnadle VpHandle;
	static VkDescriptorSet VpSet;

	static VkPushConstantRange PushConstants;

	void Init()
	{
		BufferMultiFrameSize = BufferSingleFrameSize * VulkanInterface::GetImageCount();
		Buffer = VulkanInterface::CreateUniformBuffer(BufferMultiFrameSize);
		BufferAlignment = VulkanInterface::GetDeviceProperties()->limits.minUniformBufferOffsetAlignment;

		const VkDeviceSize VpBufferSize = sizeof(ViewProjectionBuffer);
		const VkShaderStageFlags VpStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VpHandle = ReserveUniformMemory(VpBufferSize);
		VpLayout = VulkanInterface::CreateUniformLayout(&BufferType, &VpStageFlags, 1);

		VpSet = CreateAndBindSet(VpHandle, VpBufferSize, VpLayout);
	}

	void DeInit()
	{
		VulkanInterface::DestroyUniformLayout(VpLayout);
		VulkanInterface::DestroyUniformBuffer(Buffer);
	}

	void UpdateViewProjection(const ViewProjectionBuffer* Data)
	{
		UpdateUniformMemory(VpHandle, Data, sizeof(ViewProjectionBuffer));
	}

	UniformMemoryHnadle ReserveUniformMemory(u64 Size)
	{
		const UniformMemoryHnadle Handle = NextUniformMemoryHandle;
		NextUniformMemoryHandle += Size * VulkanInterface::GetImageCount();
		return Handle;
	}

	void UpdateUniformMemory(UniformMemoryHnadle Handle, const void* Data, u64 Size)
	{
		VulkanInterface::UpdateUniformBuffer(Buffer, Size,
			Handle + (Size * VulkanInterface::TestGetImageIndex()), Data);
	}

	VkDescriptorSet CreateAndBindSet(UniformMemoryHnadle Handle, u64 Size, VkDescriptorSetLayout Layout)
	{
		VkDescriptorSet NewSet;
		VulkanInterface::CreateUniformSets(&Layout, 1, &NewSet);

		VulkanInterface::UniformSetAttachmentInfo VpBufferAttachmentInfo;
		VpBufferAttachmentInfo.BufferInfo.buffer = Buffer.Buffer;
		VpBufferAttachmentInfo.BufferInfo.offset = Handle;
		VpBufferAttachmentInfo.BufferInfo.range = Size;
		VpBufferAttachmentInfo.Type = BufferType;

		VulkanInterface::AttachUniformsToSet(NewSet, &VpBufferAttachmentInfo, 1);

		return NewSet;
	}

	VkDescriptorSetLayout CreateCompatibleLayout(u32 Flags)
	{
		return VulkanInterface::CreateUniformLayout(&BufferType, &Flags, 1);
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
