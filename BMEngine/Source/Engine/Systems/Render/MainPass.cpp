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
	static VulkanInterface::RenderPass RenderPass;

	static VulkanInterface::AttachmentData AttachmentData;

	void Init()
	{
		AttachmentData.ColorAttachmentCount = 2;
		AttachmentData.ColorAttachmentFormats[0] = ColorFormat;
		AttachmentData.ColorAttachmentFormats[1] = DepthFormat;
		AttachmentData.DepthAttachmentFormat = DepthFormat;


		const VkDescriptorType Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags Flags = VK_SHADER_STAGE_FRAGMENT_BIT;
		SkyBoxLayout = VulkanInterface::CreateUniformLayout(&Type, &Flags, 1);





		VulkanInterface::RenderTarget RenderTarget;
		for (u32 ImageIndex = 0; ImageIndex < VulkanInterface::GetImageCount(); ImageIndex++)
		{
			VulkanInterface::AttachmentView* AttachmentView = RenderTarget.AttachmentViews + ImageIndex;

			AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(MainRenderPassSettings.AttachmentDescriptionsCount);
			AttachmentView->ImageViews[0] = VulkanInterface::GetSwapchainImageViews()[ImageIndex];
			AttachmentView->ImageViews[1] = DeferredPass::TestDeferredInputColorImageInterface()[ImageIndex];
			AttachmentView->ImageViews[2] = DeferredPass::TestDeferredInputDepthImageInterface()[ImageIndex];

		}

		VulkanInterface::CreateRenderPass(&MainRenderPassSettings, &RenderTarget, MainScreenExtent, 1, VulkanInterface::GetImageCount(), &RenderPass);

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
		ResourceInfo.RenderPass = MainPass::TestGetRenderPass();
		ResourceInfo.SubpassIndex = 0;

		SkyBoxPipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, VertexInputBinding, 1, &SkyBoxPipelineSettings, &ResourceInfo);
	}

	void DeInit()
	{
	}

	void BeginPass()
	{
		//const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet()[ImageIndex];
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };

		VkRenderPassBeginInfo DepthRenderPassBeginInfo = { };
		DepthRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		DepthRenderPassBeginInfo.renderPass = RenderPass.Pass;
		DepthRenderPassBeginInfo.renderArea = RenderArea;
		DepthRenderPassBeginInfo.pClearValues = RenderPass.ClearValues;
		DepthRenderPassBeginInfo.clearValueCount = RenderPass.ClearValuesCount;
		DepthRenderPassBeginInfo.framebuffer = RenderPass.RenderTargets[0].FrameBuffers[VulkanInterface::TestGetImageIndex()];

		vkCmdBeginRenderPass(CmdBuffer, &DepthRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void EndPass()
	{
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

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		vkCmdEndRenderPass(CmdBuffer);
	}

	VkRenderPass TestGetRenderPass()
	{
		return RenderPass.Pass;
	}

	VulkanInterface::AttachmentData* GetAttachmentData()
	{
		return &AttachmentData;
	}
}
