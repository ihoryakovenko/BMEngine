#include "LightningPass.h"

#include <vulkan/vulkan.h>


#include "Util/Settings.h"
#include "Util/Util.h"

#include "Engine/Systems/Render/VulkanHelper.h"

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
		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();

		VkDescriptorType LightSpaceMatrixDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VkShaderStageFlags LightSpaceMatrixStageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		const VkDescriptorBindingFlags BindingFlags[1] = { };
		LightSpaceMatrixLayout = VulkanInterface::CreateUniformLayout(&LightSpaceMatrixDescriptorType, &LightSpaceMatrixStageFlags, BindingFlags, 1, 1);

		VulkanInterface::UniformImageInterfaceCreateInfo ShadowMapElement1InterfaceCreateInfo = { };
		ShadowMapElement1InterfaceCreateInfo.Flags = 0; // No flags
		ShadowMapElement1InterfaceCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		ShadowMapElement1InterfaceCreateInfo.Format = DepthFormat;
		ShadowMapElement1InterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapElement1InterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapElement1InterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapElement1InterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapElement1InterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		ShadowMapElement1InterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
		ShadowMapElement1InterfaceCreateInfo.SubresourceRange.levelCount = 1;
		ShadowMapElement1InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
		ShadowMapElement1InterfaceCreateInfo.SubresourceRange.layerCount = 1;

		VulkanInterface::UniformImageInterfaceCreateInfo ShadowMapElement2InterfaceCreateInfo = ShadowMapElement1InterfaceCreateInfo;
		ShadowMapElement2InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 1;

		VkImageCreateInfo ShadowMapArrayCreateInfo = { };
		ShadowMapArrayCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ShadowMapArrayCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ShadowMapArrayCreateInfo.extent.depth = 1;
		ShadowMapArrayCreateInfo.mipLevels = 1;
		ShadowMapArrayCreateInfo.arrayLayers = 1;
		ShadowMapArrayCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ShadowMapArrayCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ShadowMapArrayCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ShadowMapArrayCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ShadowMapArrayCreateInfo.flags = 0;
		ShadowMapArrayCreateInfo.format = ColorFormat;
		ShadowMapArrayCreateInfo.extent.width = DepthViewportExtent.width;
		ShadowMapArrayCreateInfo.extent.height = DepthViewportExtent.height;
		ShadowMapArrayCreateInfo.format = DepthFormat;
		ShadowMapArrayCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ShadowMapArrayCreateInfo.arrayLayers = 2;

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkCreateImage(Device, &ShadowMapArrayCreateInfo, nullptr, &ShadowMapArray[i].Image);
			VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, ShadowMapArray[i].Image, VulkanHelper::GPULocal);
			ShadowMapArray[i].Memory = AllocResult.Memory;
			VULKAN_CHECK_RESULT(vkBindImageMemory(Device, ShadowMapArray[i].Image, ShadowMapArray[i].Memory, 0));

			const VkDeviceSize LightSpaceMatrixSize = sizeof(glm::mat4);

			VkBufferCreateInfo BufferInfo = { };
			BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			BufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			BufferInfo.size = LightSpaceMatrixSize;

			LightSpaceMatrixBuffer[i].Buffer = VulkanHelper::CreateBuffer(Device, LightSpaceMatrixSize, VulkanHelper::BufferUsageFlag::UniformFlag);
			AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, LightSpaceMatrixBuffer[i].Buffer,
				VulkanHelper::MemoryPropertyFlag::HostCompatible);
			LightSpaceMatrixBuffer[i].Memory = AllocResult.Memory;
			vkBindBufferMemory(Device, LightSpaceMatrixBuffer[i].Buffer, LightSpaceMatrixBuffer[i].Memory, 0);

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

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.pSetLayouts = &LightSpaceMatrixLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstants;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &Pipeline.PipelineLayout));

		std::vector<char> ShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/Depth_vert.spv", ShaderCode, "rb");

		const u32 ShaderCount = 1;
		VulkanInterface::Shader Shaders[ShaderCount];

		Shaders[0].Stage = VK_SHADER_STAGE_VERTEX_BIT;
		Shaders[0].Code = ShaderCode.data();
		Shaders[0].CodeSize = ShaderCode.size();

		VulkanInterface::PipelineResourceInfo ResourceInfo = { };
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 0;
		ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = DepthFormat;
		ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		const u32 VertexInputCount = 1;
		VulkanInterface::BMRVertexInputBinding VertexInputBinding[VertexInputCount];
		VertexInputBinding[0].InputAttributes[0] = { "Position", VK_FORMAT_R32G32B32_SFLOAT, offsetof(Render::StaticMeshVertex, Position) };
		VertexInputBinding[0].InputAttributesCount = 1;
		VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VertexInputBinding[0].Stride = sizeof(Render::StaticMeshVertex);
		VertexInputBinding[0].VertexInputBindingName = "MeshVertex";

		Pipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, VertexInputBinding, VertexInputCount, &DepthPipelineSettings, &ResourceInfo);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkDestroyBuffer(Device, LightSpaceMatrixBuffer[i].Buffer, nullptr);
			vkFreeMemory(Device, LightSpaceMatrixBuffer[i].Memory, nullptr);

			vkDestroyImageView(Device, ShadowMapElement1ImageInterface[i], nullptr);
			vkDestroyImageView(Device, ShadowMapElement2ImageInterface[i], nullptr);

			vkDestroyImage(Device, ShadowMapArray[i].Image, nullptr);
			vkFreeMemory(Device, ShadowMapArray[i].Memory, nullptr);
		}

		vkDestroyDescriptorSetLayout(Device, LightSpaceMatrixLayout, nullptr);

		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);
	}

	void Draw()
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		const Render::RenderState* State = Render::GetRenderState();

		const glm::mat4* LightViews[] =
		{
			&Scene.LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene.LightEntity->SpotLight.LightSpaceMatrix,
		};

		VkImageView Attachments[2];
		Attachments[0] = ShadowMapElement1ImageInterface[VulkanInterface::TestGetImageIndex()];
		Attachments[1] = ShadowMapElement2ImageInterface[VulkanInterface::TestGetImageIndex()];

		for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
		{
			VulkanHelper::UpdateHostCompatibleBufferMemory(Device, LightSpaceMatrixBuffer[LightCaster].Memory, sizeof(glm::mat4), 0,
				LightViews[LightCaster]);

			VkRect2D RenderArea;
			RenderArea.extent = DepthViewportExtent;
			RenderArea.offset = { 0, 0 };

			VkRenderingAttachmentInfo DepthAttachment{ };
			DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			DepthAttachment.imageView = Attachments[LightCaster];
			DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			DepthAttachment.clearValue.depthStencil = { 1.0f, 0 };

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

			for (u32 i = 0; i < State->RenderResources.DrawEntities.Count; ++i)
			{
				Render::DrawEntity* DrawEntity = State->RenderResources.DrawEntities.Data + i;
				Render::StaticMesh* Mesh = State->RenderResources.Meshes.StaticMeshes.Data + DrawEntity->StaticMeshIndex;
				Render::Material* Material = State->RenderResources.Materials.Materials.Data + DrawEntity->MaterialIndex;
				Render::MeshTexture2D* AlbedoTexture = State->RenderResources.Textures.Textures.Data + Material->AlbedoTexIndex;
				Render::MeshTexture2D* SpecTexture = State->RenderResources.Textures.Textures.Data + Material->SpecularTexIndex;

				if (!Mesh->IsLoaded || !Material->IsLoaded || !AlbedoTexture->IsLoaded || !SpecTexture->IsLoaded)
				{
					continue;
				}

				const VkBuffer VertexBuffers[] = { State->RenderResources.Meshes.VertexStageData.Buffer };
				const VkDeviceSize Offsets[] = { Mesh->VertexOffset };

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
				vkCmdBindIndexBuffer(CmdBuffer, State->RenderResources.Meshes.VertexStageData.Buffer, Mesh->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CmdBuffer, Mesh->IndicesCount, 1, 0, 0, 0);
			}

			vkCmdEndRendering(CmdBuffer);
		}

		// TODO: move to Main pass?
		VkImageMemoryBarrier2 DepthAttachmentTransitionAfter = { };
		DepthAttachmentTransitionAfter.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

		DepthAttachmentTransitionAfter.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentTransitionAfter.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DepthAttachmentTransitionAfter.image = ShadowMapArray[VulkanInterface::TestGetImageIndex()].Image;
		DepthAttachmentTransitionAfter.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthAttachmentTransitionAfter.subresourceRange.baseMipLevel = 0;
		DepthAttachmentTransitionAfter.subresourceRange.levelCount = 1;
		DepthAttachmentTransitionAfter.subresourceRange.baseArrayLayer = 0;
		DepthAttachmentTransitionAfter.subresourceRange.layerCount = 2;
		// RELEASE: all depth writes have finished  
		DepthAttachmentTransitionAfter.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		DepthAttachmentTransitionAfter.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		// ACQUIRE: allow sampling in the fragment shader  
		DepthAttachmentTransitionAfter.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		DepthAttachmentTransitionAfter.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		VkDependencyInfo DependencyInfoAfter = { };
		DependencyInfoAfter.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DependencyInfoAfter.imageMemoryBarrierCount = 1;
		DependencyInfoAfter.pImageMemoryBarriers = &DepthAttachmentTransitionAfter;

		vkCmdPipelineBarrier2(CmdBuffer, &DependencyInfoAfter);
	}

	VulkanInterface::UniformImage* GetShadowMapArray()
	{
		return ShadowMapArray;
	}
}