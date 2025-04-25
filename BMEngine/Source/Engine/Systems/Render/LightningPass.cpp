#include "RenderResourceManger.h"
#include "LightningPass.h"

#include <vulkan/vulkan.h>

#include "RenderResourceManger.h"

#include "Util/Settings.h"
#include "Util/Util.h"

// todo: delete
#include "Engine/Scene.h"

namespace LightningPass
{
	static VulkanInterface::UniformImage ShadowMapArray[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSetLayout LightSpaceMatrixLayout;

	static VulkanInterface::UniformBuffer LightSpaceMatrixBuffer[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSet LightSpaceMatrixSet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkImageView ShadowMapElement1ImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VkImageView ShadowMapElement2ImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkPushConstantRange PushConstants;

	static VulkanInterface::RenderPipeline Pipeline;

	void Init()
	{
		VkDescriptorType LightSpaceMatrixDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VkShaderStageFlags LightSpaceMatrixStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		LightSpaceMatrixLayout = VulkanInterface::CreateUniformLayout(&LightSpaceMatrixDescriptorType, &LightSpaceMatrixStageFlags, 1);

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			ShadowMapArray[i] = VulkanInterface::CreateUniformImage(&ShadowMapArrayCreateInfo);

			const VkDeviceSize LightSpaceMatrixSize = sizeof(glm::mat4);

			VpBufferInfo.size = LightSpaceMatrixSize;
			LightSpaceMatrixBuffer[i] = VulkanInterface::CreateUniformBufferInternal(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			VulkanInterface::CreateUniformSets(&LightSpaceMatrixLayout, 1, LightSpaceMatrixSet + i);

			ShadowMapElement1ImageInterface[i] = VulkanInterface::CreateImageInterface(&ShadowMapElement1InterfaceCreateInfo, ShadowMapArray[i].Image);
			ShadowMapElement2ImageInterface[i] = VulkanInterface::CreateImageInterface(&ShadowMapElement2InterfaceCreateInfo, ShadowMapArray[i].Image);

			VulkanInterface::UniformSetAttachmentInfo LightSpaceMatrixAttachmentInfo;
			LightSpaceMatrixAttachmentInfo.BufferInfo.buffer = LightSpaceMatrixBuffer[i].Buffer;
			LightSpaceMatrixAttachmentInfo.BufferInfo.offset = 0;
			LightSpaceMatrixAttachmentInfo.BufferInfo.range = LightSpaceMatrixSize;
			LightSpaceMatrixAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			VulkanInterface::AttachUniformsToSet(LightSpaceMatrixSet[i], &LightSpaceMatrixAttachmentInfo, 1);
		}

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(glm::mat4);

		Pipeline.PipelineLayout = VulkanInterface::CreatePipelineLayout(&LightSpaceMatrixLayout, 1,
			&PushConstants, 1);

		std::vector<char> ShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/Depth_vert.spv", ShaderCode, "rb");

		VkPipelineShaderStageCreateInfo Info = { };
		Info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		Info.pName = "main";
		VulkanInterface::CreateShader((u32*)ShaderCode.data(), ShaderCode.size(), Info.module);

		VulkanInterface::PipelineShaderInfo ShaderInfo;
		ShaderInfo.Infos = &Info;
		ShaderInfo.InfosCounter = 1;

		VulkanInterface::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.RenderPass = nullptr;
		ResourceInfo.SubpassIndex = 0;

		ResourceInfo.ColorAttachmentCount = 0;
		ResourceInfo.ColorAttachmentFormats = nullptr;
		ResourceInfo.DepthAttachmentFormat = DepthFormat;
		ResourceInfo.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		VulkanInterface::CreatePipelines(&ShaderInfo, &DepthVertexInput, &DepthPipelineSettings, &ResourceInfo, 1, &Pipeline.Pipeline);
		VulkanInterface::DestroyShader(Info.module);
	}

	void DeInit()
	{
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VulkanInterface::DestroyUniformBuffer(LightSpaceMatrixBuffer[i]);

			VulkanInterface::DestroyImageInterface(ShadowMapElement1ImageInterface[i]);
			VulkanInterface::DestroyImageInterface(ShadowMapElement2ImageInterface[i]);

			VulkanInterface::DestroyUniformImage(ShadowMapArray[i]);
		}

		VulkanInterface::DestroyUniformLayout(LightSpaceMatrixLayout);

		VulkanInterface::DestroyPipelineLayout(Pipeline.PipelineLayout);
		VulkanInterface::DestroyPipeline(Pipeline.Pipeline);
	}

	void Draw()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		const glm::mat4* LightViews[] =
		{
			&Scene.LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene.LightEntity->SpotLight.LightSpaceMatrix,
		};

		for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
		{
			VulkanInterface::UpdateUniformBuffer(LightSpaceMatrixBuffer[LightCaster], sizeof(glm::mat4), 0,
				LightViews[LightCaster]);

			VkRect2D RenderArea;
			RenderArea.extent = DepthViewportExtent;
			RenderArea.offset = { 0, 0 };

			VkRenderingAttachmentInfo DepthAttachment{ };
			DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			DepthAttachment.imageView = ShadowMapElement1ImageInterface[VulkanInterface::TestGetImageIndex()];
			DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			DepthAttachment.clearValue.depthStencil.depth = 1.0f;

			VkRenderingInfo RenderingInfo{ };
			RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			RenderingInfo.renderArea = RenderArea;
			RenderingInfo.layerCount = 1;
			RenderingInfo.colorAttachmentCount = 0;
			RenderingInfo.pColorAttachments = nullptr;
			RenderingInfo.pDepthAttachment = &DepthAttachment;

			VkImageMemoryBarrier2 DepthAttachmentTransitionBefore = { };
			DepthAttachmentTransitionBefore.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			DepthAttachmentTransitionBefore.srcStageMask = VK_PIPELINE_STAGE_2_NONE; // because we're coming from UNDEFINED
			DepthAttachmentTransitionBefore.srcAccessMask = 0;
			DepthAttachmentTransitionBefore.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			DepthAttachmentTransitionBefore.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			DepthAttachmentTransitionBefore.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			DepthAttachmentTransitionBefore.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			DepthAttachmentTransitionBefore.image = ShadowMapArray[VulkanInterface::TestGetImageIndex()].Image;
			DepthAttachmentTransitionBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			DepthAttachmentTransitionBefore.subresourceRange.baseMipLevel = 0;
			DepthAttachmentTransitionBefore.subresourceRange.levelCount = 1;
			DepthAttachmentTransitionBefore.subresourceRange.baseArrayLayer = LightCaster;
			DepthAttachmentTransitionBefore.subresourceRange.layerCount = 1;

			VkDependencyInfo DependencyInfoBefore = { };
			DependencyInfoBefore.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			DependencyInfoBefore.imageMemoryBarrierCount = 1;
			DependencyInfoBefore.pImageMemoryBarriers = &DepthAttachmentTransitionBefore,

			vkCmdPipelineBarrier2(CmdBuffer, &DependencyInfoBefore);

			vkCmdBeginRendering(CmdBuffer, &RenderingInfo);

			vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);

			for (u32 i = 0; i < RenderResourceManager::GetEntitiesCount(); ++i)
			{
				RenderResourceManager::DrawEntity* DrawEntity = RenderResourceManager::GetEntities() + i;

				const VkBuffer VertexBuffers[] = { RenderResourceManager::GetVertexBuffer().Buffer };
				const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

				const u32 DescriptorSetGroupCount = 1;
				const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
				{
					LightSpaceMatrixSet[LightCaster],
				};

				const VkPipelineLayout PipelineLayout = Pipeline.PipelineLayout;

				vkCmdPushConstants(CmdBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &DrawEntity->Model);

				vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr);

				vkCmdBindVertexBuffers(CmdBuffer, 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(CmdBuffer, RenderResourceManager::GetIndexBuffer().Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CmdBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
			}

			vkCmdEndRendering(CmdBuffer);

			// TODO: move to Main pass?
			VkImageMemoryBarrier2 DepthAttachmentTransitionAfter = { };
			DepthAttachmentTransitionAfter.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			DepthAttachmentTransitionAfter.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			DepthAttachmentTransitionAfter.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			DepthAttachmentTransitionAfter.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			DepthAttachmentTransitionAfter.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			DepthAttachmentTransitionAfter.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			DepthAttachmentTransitionAfter.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthAttachmentTransitionAfter.image = ShadowMapArray[VulkanInterface::TestGetImageIndex()].Image;
			DepthAttachmentTransitionAfter.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			DepthAttachmentTransitionAfter.subresourceRange.baseMipLevel = 0;
			DepthAttachmentTransitionAfter.subresourceRange.levelCount = 1;
			DepthAttachmentTransitionAfter.subresourceRange.baseArrayLayer = LightCaster;
			DepthAttachmentTransitionAfter.subresourceRange.layerCount = 1;

			VkDependencyInfo DependencyInfoAfter = { };
			DependencyInfoAfter.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			DependencyInfoAfter.imageMemoryBarrierCount = 1;
			DependencyInfoAfter.pImageMemoryBarriers = &DepthAttachmentTransitionAfter;

			vkCmdPipelineBarrier2(CmdBuffer, &DependencyInfoAfter);
		}
	}

	VulkanInterface::UniformImage* GetShadowMapArray()
	{
		return ShadowMapArray;
	}
}