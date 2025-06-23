#include "FrameManager.h"






//////////////////////////////////////
// TODO: DEPRECATED TO REFACTOR
//////////////////////////////////////









#include <vulkan/vulkan.h>

#include "VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/Util.h"

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
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkDevice Device = VulkanInterface::GetDevice();

		BufferMultiFrameSize = BufferSingleFrameSize * VulkanInterface::GetImageCount();

		Buffer.Buffer = VulkanHelper::CreateBuffer(Device, BufferMultiFrameSize, VulkanHelper::BufferUsageFlag::UniformFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, Buffer.Buffer,
			VulkanHelper::MemoryPropertyFlag::HostCompatible);
		Buffer.Memory = AllocResult.Memory;
		BufferAlignment = AllocResult.Alignment;
		vkBindBufferMemory(Device, Buffer.Buffer, Buffer.Memory, 0);

		const VkDeviceSize VpBufferSize = sizeof(ViewProjectionBuffer);
		const VkShaderStageFlags VpStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VpHandle = ReserveUniformMemory(VpBufferSize);
		
		VkDescriptorSetLayoutBinding LayoutBinding = { };
		LayoutBinding.binding = 0;
		LayoutBinding.descriptorType = BufferType;
		LayoutBinding.descriptorCount = 1;
		LayoutBinding.stageFlags = VpStageFlags;
		LayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = 1;
		LayoutCreateInfo.pBindings = &LayoutBinding;
		LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		LayoutCreateInfo.pNext = nullptr;

		VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(VulkanInterface::GetDevice(), &LayoutCreateInfo, nullptr, &VpLayout));

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
		VkDescriptorSetAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.descriptorPool = VulkanInterface::GetDescriptorPool();
		AllocInfo.descriptorSetCount = 1;
		AllocInfo.pSetLayouts = &Layout;
		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(VulkanInterface::GetDevice(), &AllocInfo, &NewSet));

		VkDescriptorBufferInfo VpBufferInfo;
		VpBufferInfo.buffer = Buffer.Buffer;
		VpBufferInfo.offset = Handle;
		VpBufferInfo.range = Size;

		VkWriteDescriptorSet WriteDescriptorSet = {};
		WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSet.dstSet = NewSet;
		WriteDescriptorSet.dstBinding = 0;
		WriteDescriptorSet.dstArrayElement = 0;
		WriteDescriptorSet.descriptorType = BufferType;
		WriteDescriptorSet.descriptorCount = 1;
		WriteDescriptorSet.pBufferInfo = &VpBufferInfo;
		WriteDescriptorSet.pImageInfo = nullptr;

		vkUpdateDescriptorSets(VulkanInterface::GetDevice(), 1, &WriteDescriptorSet, 0, nullptr);

		return NewSet;
	}

	VkDescriptorSetLayout CreateCompatibleLayout(u32 Flags)
	{
		VkDescriptorSetLayoutBinding LayoutBinding = { };
		LayoutBinding.binding = 0;
		LayoutBinding.descriptorType = BufferType;
		LayoutBinding.descriptorCount = 1;
		LayoutBinding.stageFlags = Flags;
		LayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = 1;
		LayoutCreateInfo.pBindings = &LayoutBinding;
		LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		LayoutCreateInfo.pNext = nullptr;

		VkDescriptorSetLayout Layout;
		VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(VulkanInterface::GetDevice(), &LayoutCreateInfo, nullptr, &Layout));
		return Layout;
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
