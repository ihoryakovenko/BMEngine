#include "DeferredPass.h"

#include "Deprecated/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"
#include "VulkanCoreContext.h"
#include "RenderResources.h"

#include "Util/Settings.h"
#include "Util/Util.h"

namespace DeferredPass
{
	static VulkanHelper::RenderPipeline Pipeline;

	static VkDescriptorSetLayout DeferredInputLayout;

	static VulkanInterface::UniformImage DeferredInputDepthImage[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VulkanInterface::UniformImage DeferredInputColorImage[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkImageView DeferredInputDepthImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VkImageView DeferredInputColorImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSet DeferredInputSet[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkSampler ColorSampler;
	static VkSampler DepthSampler;

	static VulkanHelper::AttachmentData AttachmentData;

	void Init()
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();

		AttachmentData.ColorAttachmentCount = 1;
		AttachmentData.ColorAttachmentFormats[0] = VulkanInterface::GetSurfaceFormat();
		AttachmentData.DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
		AttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		DeferredInputLayout = RenderResources::GetSetLayout("MainPassOutputLayout");

		VkImageCreateInfo DeferredInputDepthUniformCreateInfo = { };
		DeferredInputDepthUniformCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		DeferredInputDepthUniformCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		DeferredInputDepthUniformCreateInfo.extent.depth = 1;
		DeferredInputDepthUniformCreateInfo.mipLevels = 1;
		DeferredInputDepthUniformCreateInfo.arrayLayers = 1;
		DeferredInputDepthUniformCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		DeferredInputDepthUniformCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DeferredInputDepthUniformCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		DeferredInputDepthUniformCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		DeferredInputDepthUniformCreateInfo.flags = 0;
		DeferredInputDepthUniformCreateInfo.extent.width = MainScreenExtent.width;
		DeferredInputDepthUniformCreateInfo.extent.height = MainScreenExtent.height;
		DeferredInputDepthUniformCreateInfo.format = DepthFormat;
		DeferredInputDepthUniformCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageCreateInfo DeferredInputColorUniformCreateInfo = DeferredInputDepthUniformCreateInfo;
		DeferredInputColorUniformCreateInfo.format = ColorFormat;
		DeferredInputColorUniformCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		ColorSampler = RenderResources::GetSampler("ColorAttachment");
		DepthSampler = RenderResources::GetSampler("DepthAttachment");

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);

			vkCreateImage(Device, &DeferredInputDepthUniformCreateInfo, nullptr, &DeferredInputDepthImage[i].Image);
			VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
				DeferredInputDepthImage[i].Image, VulkanHelper::GPULocal);
			DeferredInputDepthImage[i].Memory = AllocResult.Memory;
			vkBindImageMemory(Device, DeferredInputDepthImage[i].Image, DeferredInputDepthImage[i].Memory, 0);

			vkCreateImage(Device, &DeferredInputColorUniformCreateInfo, nullptr, &DeferredInputColorImage[i].Image);
			AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, DeferredInputColorImage[i].Image, VulkanHelper::GPULocal);
			DeferredInputColorImage[i].Memory = AllocResult.Memory;
			vkBindImageMemory(Device, DeferredInputColorImage[i].Image, DeferredInputColorImage[i].Memory, 0);

			VkImageViewCreateInfo DepthViewCreateInfo = {};
			DepthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			DepthViewCreateInfo.image = DeferredInputDepthImage[i].Image;
			DepthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			DepthViewCreateInfo.format = DepthFormat;
			DepthViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			DepthViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			DepthViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			DepthViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			DepthViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			DepthViewCreateInfo.subresourceRange.baseMipLevel = 0;
			DepthViewCreateInfo.subresourceRange.levelCount = 1;
			DepthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			DepthViewCreateInfo.subresourceRange.layerCount = 1;
			DepthViewCreateInfo.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &DepthViewCreateInfo, nullptr, &DeferredInputDepthImageInterface[i]));

			VkImageViewCreateInfo ColorViewCreateInfo = {};
			ColorViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ColorViewCreateInfo.image = DeferredInputColorImage[i].Image;
			ColorViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ColorViewCreateInfo.format = ColorFormat;
			ColorViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ColorViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ColorViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ColorViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ColorViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ColorViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ColorViewCreateInfo.subresourceRange.levelCount = 1;
			ColorViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			ColorViewCreateInfo.subresourceRange.layerCount = 1;
			ColorViewCreateInfo.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ColorViewCreateInfo, nullptr, &DeferredInputColorImageInterface[i]));

			VkDescriptorSetAllocateInfo AllocInfo = {};
			AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			AllocInfo.descriptorPool = VulkanInterface::GetDescriptorPool();
			AllocInfo.descriptorSetCount = 1;
			AllocInfo.pSetLayouts = &DeferredInputLayout;
			VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, DeferredInputSet + i));

			VkDescriptorImageInfo DeferredInputImageInfo[2];
			DeferredInputImageInfo[0].imageView = DeferredInputColorImageInterface[i];
			DeferredInputImageInfo[0].sampler = ColorSampler;
			DeferredInputImageInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			DeferredInputImageInfo[1].imageView = DeferredInputDepthImageInterface[i];
			DeferredInputImageInfo[1].sampler = DepthSampler;
			DeferredInputImageInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet WriteDescriptorSets[2] = {};
			
			WriteDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSets[0].dstSet = DeferredInputSet[i];
			WriteDescriptorSets[0].dstBinding = 0;
			WriteDescriptorSets[0].dstArrayElement = 0;
			WriteDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			WriteDescriptorSets[0].descriptorCount = 1;
			WriteDescriptorSets[0].pBufferInfo = nullptr;
			WriteDescriptorSets[0].pImageInfo = &DeferredInputImageInfo[0];

			WriteDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSets[1].dstSet = DeferredInputSet[i];
			WriteDescriptorSets[1].dstBinding = 1;
			WriteDescriptorSets[1].dstArrayElement = 0;
			WriteDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			WriteDescriptorSets[1].descriptorCount = 1;
			WriteDescriptorSets[1].pBufferInfo = nullptr;
			WriteDescriptorSets[1].pImageInfo = &DeferredInputImageInfo[1];

			vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);
		}


		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.pSetLayouts = &DeferredInputLayout;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &Pipeline.PipelineLayout));

		VulkanHelper::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = AttachmentData;

		VulkanHelper::PipelineSettings PipelineSettings;
		Util::LoadPipelineSettings(PipelineSettings, "./Resources/Settings/DeferredPipeline.yaml");
		PipelineSettings.Extent = MainScreenExtent;

		Pipeline.Pipeline = VulkanHelper::BatchPipelineCreation(Device, nullptr, 0,
			&PipelineSettings, &ResourceInfo);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkDestroyImageView(Device, DeferredInputDepthImageInterface[i], nullptr);
			vkDestroyImageView(Device, DeferredInputColorImageInterface[i], nullptr);

			vkDestroyImage(Device, DeferredInputDepthImage[i].Image, nullptr);
			vkFreeMemory(Device, DeferredInputDepthImage[i].Memory, nullptr);

			vkDestroyImage(Device, DeferredInputColorImage[i].Image, nullptr);
			vkFreeMemory(Device, DeferredInputColorImage[i].Memory, nullptr);
		}


		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);
	}

	void Draw()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);

		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
			0, 1, &DeferredInputSet[VulkanInterface::TestGetImageIndex()], 0, nullptr);

		vkCmdDraw(CmdBuffer, 3, 1, 0, 0); // 3 hardcoded vertices
	}

	void BeginPass()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };

		VkRenderingAttachmentInfo SwapchainColorAttachment = { };
		SwapchainColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		SwapchainColorAttachment.imageView = VulkanInterface::GetSwapchainImageViews()[VulkanInterface::TestGetImageIndex()];
		SwapchainColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SwapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		SwapchainColorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingInfo RenderingInfo{ };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		RenderingInfo.renderArea = RenderArea;
		RenderingInfo.layerCount = 1;
		RenderingInfo.colorAttachmentCount = 1;
		RenderingInfo.pColorAttachments = &SwapchainColorAttachment;
		RenderingInfo.pDepthAttachment = nullptr;

		VkImageMemoryBarrier2 ColorBarrier = { };
		ColorBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		ColorBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ColorBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrier.image = DeferredInputColorImage[VulkanInterface::TestGetImageIndex()].Image;
		ColorBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ColorBarrier.subresourceRange.baseMipLevel = 0;
		ColorBarrier.subresourceRange.levelCount = 1;
		ColorBarrier.subresourceRange.baseArrayLayer = 0;
		ColorBarrier.subresourceRange.layerCount = 1;
		// RELEASE: wait for all color-attachmet writes to finish
		ColorBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		ColorBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// ACQUIRE: make image ready for sampling in the fragment shader
		ColorBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		ColorBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkImageMemoryBarrier2 DepthBarrier = { };
		DepthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		DepthBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DepthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrier.image = DeferredInputDepthImage[VulkanInterface::TestGetImageIndex()].Image;
		DepthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthBarrier.subresourceRange.baseMipLevel = 0;
		DepthBarrier.subresourceRange.levelCount = 1;
		DepthBarrier.subresourceRange.baseArrayLayer = 0;
		DepthBarrier.subresourceRange.layerCount = 1;
		DepthBarrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		DepthBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		DepthBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		DepthBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;


		VkImageMemoryBarrier2 SwapchainAcquireBarrier = { };
		SwapchainAcquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainAcquireBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SwapchainAcquireBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainAcquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainAcquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainAcquireBarrier.image = VulkanInterface::GetSwapchainImages()[VulkanInterface::TestGetImageIndex()];
		SwapchainAcquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SwapchainAcquireBarrier.subresourceRange.baseMipLevel = 0;
		SwapchainAcquireBarrier.subresourceRange.levelCount = 1;
		SwapchainAcquireBarrier.subresourceRange.baseArrayLayer = 0;
		SwapchainAcquireBarrier.subresourceRange.layerCount = 1;
		// RELEASE nothing we don't depend on any earlier writes to the swap image
		SwapchainAcquireBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainAcquireBarrier.srcAccessMask = 0;
		// ACQUIRE for our upcoming color-attachment writes
		SwapchainAcquireBarrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainAcquireBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkImageMemoryBarrier2 Barriers[] = {
			ColorBarrier,
			DepthBarrier,
			SwapchainAcquireBarrier
		};

		VkDependencyInfo DepInfo = { };
		DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DepInfo.imageMemoryBarrierCount = 3;
		DepInfo.pImageMemoryBarriers = Barriers;

		vkCmdPipelineBarrier2(CmdBuffer, &DepInfo);

		vkCmdBeginRendering(CmdBuffer, &RenderingInfo);
	}

	void EndPass()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		vkCmdEndRendering(CmdBuffer);

		VkImageMemoryBarrier2 SwapchainPresentBarrier = { };
		SwapchainPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		SwapchainPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.image = VulkanInterface::GetSwapchainImages()[VulkanInterface::TestGetImageIndex()];
		SwapchainPresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SwapchainPresentBarrier.subresourceRange.baseMipLevel = 0;
		SwapchainPresentBarrier.subresourceRange.levelCount = 1;
		SwapchainPresentBarrier.subresourceRange.baseArrayLayer = 0;
		SwapchainPresentBarrier.subresourceRange.layerCount = 1;
		SwapchainPresentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
			| VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		SwapchainPresentBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
			| VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		SwapchainPresentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR;  // no further memory dep
			SwapchainPresentBarrier.dstAccessMask = 0;

		VkDependencyInfo PresentDepInfo = { };
		PresentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		PresentDepInfo.imageMemoryBarrierCount = 1;
		PresentDepInfo.pImageMemoryBarriers = &SwapchainPresentBarrier;

		vkCmdPipelineBarrier2(CmdBuffer, &PresentDepInfo);
	}





	VkImageView* TestDeferredInputColorImageInterface()
	{
		return DeferredInputColorImageInterface;
	}

	VkImageView* TestDeferredInputDepthImageInterface()
	{
		return DeferredInputDepthImageInterface;
	}

	VulkanInterface::UniformImage* TestDeferredInputColorImage()
	{
		return DeferredInputColorImage;
	}

	VulkanInterface::UniformImage* TestDeferredInputDepthImage()
	{
		return DeferredInputDepthImage;
	}

	VulkanHelper::AttachmentData* GetAttachmentData()
	{
		return &AttachmentData;
	}
}
