#include "MainPass.h"

#include "Deprecated/VulkanInterface/VulkanInterface.h"
#include "Deprecated/FrameManager.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/Util.h"
#include "Util/Settings.h"

#include <glm/glm.hpp>

#include "DeferredPass.h"

namespace MainPass
{
	static VkDescriptorSetLayout SkyBoxLayout;

	static VkDescriptorSet SkyBoxSet;

	static VulkanHelper::RenderPipeline SkyBoxPipeline;

	static VulkanHelper::AttachmentData AttachmentData;

	void Init()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		AttachmentData.ColorAttachmentCount = 1;
		AttachmentData.ColorAttachmentFormats[0] = ColorFormat;
		AttachmentData.DepthAttachmentFormat = DepthFormat;


		const VkDescriptorType Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags Flags = VK_SHADER_STAGE_FRAGMENT_BIT;
		
		VkDescriptorSetLayoutBinding LayoutBinding = { };
		LayoutBinding.binding = 0;
		LayoutBinding.descriptorType = Type;
		LayoutBinding.descriptorCount = 1;
		LayoutBinding.stageFlags = Flags;
		LayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = 1;
		LayoutCreateInfo.pBindings = &LayoutBinding;
		LayoutCreateInfo.flags = 0;
		LayoutCreateInfo.pNext = nullptr;

		VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(VulkanInterface::GetDevice(), &LayoutCreateInfo, nullptr, &SkyBoxLayout));

		const VkDescriptorSetLayout VpLayout = FrameManager::GetViewProjectionLayout();

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] =
		{
			VpLayout,
			SkyBoxLayout,
		};

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = SkyBoxDescriptorLayoutCount;
		PipelineLayoutCreateInfo.pSetLayouts = SkyBoxDescriptorLayouts;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &SkyBoxPipeline.PipelineLayout));

		VulkanHelper::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = SkyBoxPipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = AttachmentData;

		Yaml::Node Root;
		Yaml::Parse(Root, "./Resources/Settings/SkyBoxPipeline.yaml");

		SkyBoxPipeline.Pipeline = RenderResources::CreateGraphicsPipeline(Device, Root, MainScreenExtent, SkyBoxPipeline.PipelineLayout, &ResourceInfo);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		vkDestroyDescriptorSetLayout(VulkanInterface::GetDevice(), SkyBoxLayout, nullptr);
		vkDestroyPipeline(Device, SkyBoxPipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, SkyBoxPipeline.PipelineLayout, nullptr);
	}

	void BeginPass()
	{
		//const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet()[ImageIndex];
		VkRenderingAttachmentInfo ColorAttachment = { };
		ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		ColorAttachment.imageView = DeferredPass::TestDeferredInputColorImageInterface()[VulkanInterface::TestGetImageIndex()];
		ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingAttachmentInfo DepthAttachment = { };
		DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		DepthAttachment.imageView = DeferredPass::TestDeferredInputDepthImageInterface()[VulkanInterface::TestGetImageIndex()];
		DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthAttachment.clearValue.depthStencil.depth = 1.0f;

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };

		VkRenderingInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		RenderingInfo.flags = 0;
		RenderingInfo.renderArea = RenderArea;
		RenderingInfo.layerCount = 1;
		RenderingInfo.viewMask = 0;
		RenderingInfo.colorAttachmentCount = 1;
		RenderingInfo.pColorAttachments = &ColorAttachment;
		RenderingInfo.pDepthAttachment = &DepthAttachment;
		RenderingInfo.pStencilAttachment = nullptr;

		VkImageMemoryBarrier2 ColorBarrierBefore = { };
		ColorBarrierBefore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		ColorBarrierBefore.pNext = nullptr;
		ColorBarrierBefore.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ColorBarrierBefore.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorBarrierBefore.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrierBefore.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrierBefore.image = DeferredPass::TestDeferredInputColorImage()[VulkanInterface::TestGetImageIndex()].Image;
		ColorBarrierBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ColorBarrierBefore.subresourceRange.baseMipLevel = 0;
		ColorBarrierBefore.subresourceRange.levelCount = 1;
		ColorBarrierBefore.subresourceRange.baseArrayLayer = 0;
		ColorBarrierBefore.subresourceRange.layerCount = 1;
		ColorBarrierBefore.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;        // whatever stage last read it
		ColorBarrierBefore.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;                   // reading it as a sampled image
		ColorBarrierBefore.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT; // the clear/write stage
		ColorBarrierBefore.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;         // the clear/write access

		VkImageMemoryBarrier2 DepthBarrierBefore = { };
		DepthBarrierBefore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		DepthBarrierBefore.pNext = nullptr;
		DepthBarrierBefore.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		DepthBarrierBefore.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthBarrierBefore.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthBarrierBefore.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrierBefore.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrierBefore.image = DeferredPass::TestDeferredInputDepthImage()[VulkanInterface::TestGetImageIndex()].Image;
		DepthBarrierBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthBarrierBefore.subresourceRange.baseMipLevel = 0;
		DepthBarrierBefore.subresourceRange.levelCount = 1;
		DepthBarrierBefore.subresourceRange.baseArrayLayer = 0;
		DepthBarrierBefore.subresourceRange.layerCount = 1;
		// RELEASE nothing (it was UNDEFINED)
		DepthBarrierBefore.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		DepthBarrierBefore.srcAccessMask = VK_PIPELINE_STAGE_NONE;
		// ACQUIRE for depth-writes
		DepthBarrierBefore.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
			| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

		VkImageMemoryBarrier2 BarriersBefore[] = { ColorBarrierBefore, DepthBarrierBefore };

		VkDependencyInfo DepInfoBefore = { };
		DepInfoBefore.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DepInfoBefore.imageMemoryBarrierCount = 2;
		DepInfoBefore.pImageMemoryBarriers = BarriersBefore;

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		vkCmdPipelineBarrier2(CmdBuffer, &DepInfoBefore);

		vkCmdBeginRendering(CmdBuffer, &RenderingInfo);

		//
		//if (Scene->DrawSkyBox)
		//{
		//	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipelines[2].Pipeline);

		//	const u32 SkyBoxDescriptorSetGroupCount = 2;
		//	const VkDescriptorSet SkyBoxDescriptorSetGroup[SkyBoxDescriptorSetGroupCount] = {
		//		VpSet,
		//		Scene->SkyBox.TextureSet,
		//	};

		//	const VkPipelineLayout PipelineLayout = Pipelines[2].PipelineLayout;

		//	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
		//		0, SkyBoxDescriptorSetGroupCount, SkyBoxDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

		//	vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &PassSharedResources.VertexBuffer.Buffer, &Scene->SkyBox.VertexOffset);
		//	vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, Scene->SkyBox.IndexOffset, VK_INDEX_TYPE_UINT32);
		//	vkCmdDrawIndexed(CommandBuffer, Scene->SkyBox.IndicesCount, 1, 0, 0, 0);
		//}

		//MainRenderPass::OnDraw();
	}

	void EndPass()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		vkCmdEndRendering(CmdBuffer);
	}

	VulkanHelper::AttachmentData* GetAttachmentData()
	{
		return &AttachmentData;
	}
}
