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

	static VulkanInterface::RenderPipeline Pipelines[MainPathPipelinesCount];
	static VulkanInterface::RenderPass RenderPass;

	void Init()
	{
		


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

		const u32 SkyBoxIndex = 0;

		u32 SetLayoutCountTable[MainPathPipelinesCount];
		SetLayoutCountTable[SkyBoxIndex] = SkyBoxDescriptorLayoutCount;

		const VkDescriptorSetLayout* SetLayouts[MainPathPipelinesCount];
		SetLayouts[SkyBoxIndex] = SkyBoxDescriptorLayouts;

		u32 PushConstantRangeCountTable[MainPathPipelinesCount];
		PushConstantRangeCountTable[SkyBoxIndex] = 0;

		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			Pipelines[i].PipelineLayout = VulkanInterface::CreatePipelineLayout(SetLayouts[i],
				SetLayoutCountTable[i], nullptr, PushConstantRangeCountTable[i]);
		}

		const char* Paths[MainPathPipelinesCount][2] = {
			{ "./Resources/Shaders/SkyBox_vert.spv", "./Resources/Shaders/SkyBox_frag.spv" },
		};

		VkShaderStageFlagBits Stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };

		VulkanInterface::PipelineShaderInfo ShaderInfo[MainPathPipelinesCount];
		VkPipelineShaderStageCreateInfo ShaderInfos[MainPathPipelinesCount][2];
		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			for (u32 j = 0; j < 2; ++j)
			{
				std::vector<char> ShaderCode;
				Util::OpenAndReadFileFull(Paths[i][j], ShaderCode, "rb");

				VkPipelineShaderStageCreateInfo* Info = &ShaderInfos[i][j];
				*Info = { };
				Info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				Info->stage = Stages[j];
				Info->pName = "main";
				VulkanInterface::CreateShader((u32*)ShaderCode.data(), ShaderCode.size(), Info->module);
			}

			ShaderInfo[i].Infos = ShaderInfos[i];
			ShaderInfo[i].InfosCounter = 2;
		}


		VulkanInterface::VertexInput VertexInput[MainPathPipelinesCount];
		VertexInput[SkyBoxIndex] = SkyBoxVertexInput;

		VulkanInterface::PipelineSettings PipelineSettings[MainPathPipelinesCount];
		PipelineSettings[SkyBoxIndex] = SkyBoxPipelineSettings;

		VulkanInterface::PipelineResourceInfo PipelineResourceInfo[MainPathPipelinesCount];
		PipelineResourceInfo[SkyBoxIndex].PipelineLayout = Pipelines[SkyBoxIndex].PipelineLayout;
		PipelineResourceInfo[SkyBoxIndex].RenderPass = RenderPass.Pass;
		PipelineResourceInfo[SkyBoxIndex].SubpassIndex = 0;

		VkPipeline NewPipelines[MainPathPipelinesCount];
		CreatePipelines(ShaderInfo, VertexInput, PipelineSettings, PipelineResourceInfo, MainPathPipelinesCount, NewPipelines);

		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			Pipelines[i].Pipeline = NewPipelines[i];

			for (u32 j = 0; j < 2; ++j)
			{
				VulkanInterface::DestroyShader(ShaderInfo[i].Infos[j].module);
			}
		}
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
}
