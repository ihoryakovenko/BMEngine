#include "FrameManager.h"

#include <vulkan/vulkan.h>

#include "VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"

namespace FrameManager
{
	static const u64 BufferSingleFrameSize = 1024 * 1024 * 10;
	static u64 BufferMultiFrameSize;

	static const VkDescriptorType BufferType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	static const VkDescriptorBindingFlags BindingFlags[1];
	static VulkanInterface::UniformBuffer Buffer;
	static u64 BufferAlignment;
	static u64 NextUniformMemoryHandle;

	static VkDescriptorSetLayout VpLayout;
	static UniformMemoryHnadle VpHandle;
	static VkDescriptorSet VpSet;

	static VkPushConstantRange PushConstants;

	void Init()
	{
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkDevice Device = VulkanInterface::GetDevice();

		BufferMultiFrameSize = BufferSingleFrameSize * VulkanInterface::GetImageCount();

		u64 Size;

		Buffer.Buffer = VulkanHelper::CreateBuffer(Device, BufferMultiFrameSize, VulkanHelper::BufferUsageFlag::UniformFlag);
		Buffer.Memory = VulkanHelper::AllocateDeviceMemoryForBuffer(PhysicalDevice, Device, Buffer.Buffer, VulkanHelper::MemoryPropertyFlag::HostCompatible,
			&Size, &BufferAlignment);
		vkBindBufferMemory(Device, Buffer.Buffer, Buffer.Memory, 0);

		const VkDeviceSize VpBufferSize = sizeof(ViewProjectionBuffer);
		const VkShaderStageFlags VpStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VpHandle = ReserveUniformMemory(VpBufferSize);
		const VkDescriptorBindingFlags BindingFlags[1] = { };
		VpLayout = VulkanInterface::CreateUniformLayout(&BufferType, &VpStageFlags, BindingFlags, 1, 1);

		VpSet = CreateAndBindSet(VpHandle, VpBufferSize, VpLayout);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		vkDestroyDescriptorSetLayout(Device, VpLayout, nullptr);

		vkDestroyBuffer(Device, Buffer.Buffer, nullptr);
		vkFreeMemory(Device, Buffer.Memory, nullptr);
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
		VulkanHelper::UpdateHostCompatibleBufferMemory(VulkanInterface::GetDevice(), Buffer.Memory, Size,
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
		return VulkanInterface::CreateUniformLayout(&BufferType, &Flags, BindingFlags, 1, 1);
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
