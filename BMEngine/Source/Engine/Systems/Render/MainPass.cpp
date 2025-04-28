#include "MainPass.h"

#include "Render/VulkanInterface/VulkanInterface.h"
#include "Render/FrameManager.h"

#include "Memory/MemoryManagmentSystem.h"

#include "Util/Util.h"
#include "Util/Settings.h"

#include <glm/glm.hpp>

#include "DeferredPass.h"

namespace MainPass
{
	static VkDescriptorSetLayout SkyBoxLayout;

	static VulkanInterface::RenderPipeline SkyBoxPipeline;

	static VulkanInterface::AttachmentData AttachmentData;

	void Init()
	{
		AttachmentData.ColorAttachmentCount = 1;
		AttachmentData.ColorAttachmentFormats[0] = ColorFormat;
		AttachmentData.DepthAttachmentFormat = DepthFormat;


		const VkDescriptorType Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags Flags = VK_SHADER_STAGE_FRAGMENT_BIT;
		SkyBoxLayout = VulkanInterface::CreateUniformLayout(&Type, &Flags, 1);





		//VulkanInterface::RenderTarget RenderTarget;
		//for (u32 ImageIndex = 0; ImageIndex < VulkanInterface::GetImageCount(); ImageIndex++)
		//{
		//	VulkanInterface::AttachmentView* AttachmentView = RenderTarget.AttachmentViews + ImageIndex;

		//	AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(MainRenderPassSettings.AttachmentDescriptionsCount);
		//	AttachmentView->ImageViews[0] = VulkanInterface::GetSwapchainImageViews()[ImageIndex];
		//	AttachmentView->ImageViews[1] = DeferredPass::TestDeferredInputColorImageInterface()[ImageIndex];
		//	AttachmentView->ImageViews[2] = DeferredPass::TestDeferredInputDepthImageInterface()[ImageIndex];

		//}


		const VkDescriptorSetLayout VpLayout = FrameManager::GetViewProjectionLayout();

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] =
		{
			VpLayout,
			SkyBoxLayout,
		};


		SkyBoxPipeline.PipelineLayout = VulkanInterface::CreatePipelineLayout(SkyBoxDescriptorLayouts, SkyBoxDescriptorLayoutCount, nullptr, 0);

		const u32 ShaderCount = 2;
		VulkanInterface::Shader Shaders[ShaderCount];

		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/SkyBox_vert.spv", VertexShaderCode, "rb");
		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/SkyBox_frag.spv", FragmentShaderCode, "rb");

		Shaders[0].Stage = VK_SHADER_STAGE_VERTEX_BIT;
		Shaders[0].Code = VertexShaderCode.data();
		Shaders[0].CodeSize = VertexShaderCode.size();

		Shaders[1].Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		Shaders[1].Code = FragmentShaderCode.data();
		Shaders[1].CodeSize = FragmentShaderCode.size();

		VulkanInterface::BMRVertexInputBinding VertexInputBinding[1];
		VertexInputBinding[0].InputAttributes[0] = { "Position", VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkyBoxVertex, Position) };
		VertexInputBinding[0].InputAttributesCount = 1;
		VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VertexInputBinding[0].Stride = sizeof(SkyBoxVertex);
		VertexInputBinding[0].VertexInputBindingName = "EntityVertex";

		VulkanInterface::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = SkyBoxPipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = AttachmentData;

		SkyBoxPipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, VertexInputBinding, 1, &SkyBoxPipelineSettings, &ResourceInfo);
	}

	void DeInit()
	{
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
		ColorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingAttachmentInfo DepthAttachment = { };
		DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		DepthAttachment.imageView = DeferredPass::TestDeferredInputDepthImageInterface()[VulkanInterface::TestGetImageIndex()];
		DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthAttachment.clearValue.depthStencil = { 1.0f, 0 }; // Clear depth to 1.0f

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
		RenderingInfo.pDepthAttachment = &DepthAttachment; // Optional, if using depth
		RenderingInfo.pStencilAttachment = nullptr; // If not using a stencil attachment

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
		// RELEASE: finish all color-writes
		ColorBarrierBefore.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		ColorBarrierBefore.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// ACQUIRE: allow shader-reads in frag stage
		ColorBarrierBefore.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		ColorBarrierBefore.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

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

	VulkanInterface::AttachmentData* GetAttachmentData()
	{
		return &AttachmentData;
	}
}
