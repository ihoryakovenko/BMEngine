#include "LightningPass.h"

#include <vulkan/vulkan.h>

#include "RenderResources.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "Engine/Systems/Render/VulkanHelper.h"
#include "VulkanCoreContext.h"

namespace LightningPass
{
	static VulkanInterface::UniformImage ShadowMapArray[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSetLayout LightSpaceMatrixLayout;

	static VulkanInterface::UniformBuffer LightSpaceMatrixBuffer[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSet LightSpaceMatrixSet[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkImageView ShadowMapElement1ImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VkImageView ShadowMapElement2ImageInterface[VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkPushConstantRange PushConstants;

	static VulkanHelper::RenderPipeline Pipeline;

	void Init()
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();

		LightSpaceMatrixLayout = RenderResources::GetSetLayout("LightSpaceMatrixLayout");

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

			VkDescriptorSetAllocateInfo AllocInfo = {};
			AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			AllocInfo.descriptorPool = VulkanInterface::GetDescriptorPool();
			AllocInfo.descriptorSetCount = 1;
			AllocInfo.pSetLayouts = &LightSpaceMatrixLayout;
			VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device, &AllocInfo, LightSpaceMatrixSet + i));

			VkImageViewCreateInfo ViewCreateInfo1 = {};
			ViewCreateInfo1.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ViewCreateInfo1.image = ShadowMapArray[i].Image;
			ViewCreateInfo1.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ViewCreateInfo1.format = DepthFormat;
			ViewCreateInfo1.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo1.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo1.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo1.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ViewCreateInfo1.subresourceRange.baseMipLevel = 0;
			ViewCreateInfo1.subresourceRange.levelCount = 1;
			ViewCreateInfo1.subresourceRange.baseArrayLayer = 0;
			ViewCreateInfo1.subresourceRange.layerCount = 1;
			ViewCreateInfo1.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo1, nullptr, &ShadowMapElement1ImageInterface[i]));

			VkImageViewCreateInfo ViewCreateInfo2 = {};
			ViewCreateInfo2.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ViewCreateInfo2.image = ShadowMapArray[i].Image;
			ViewCreateInfo2.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ViewCreateInfo2.format = DepthFormat;
			ViewCreateInfo2.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo2.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo2.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo2.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ViewCreateInfo2.subresourceRange.baseMipLevel = 0;
			ViewCreateInfo2.subresourceRange.levelCount = 1;
			ViewCreateInfo2.subresourceRange.baseArrayLayer = 1;
			ViewCreateInfo2.subresourceRange.layerCount = 1;
			ViewCreateInfo2.pNext = nullptr;

			VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo2, nullptr, &ShadowMapElement2ImageInterface[i]));

			VkDescriptorBufferInfo LightSpaceMatrixBufferInfo;
			LightSpaceMatrixBufferInfo.buffer = LightSpaceMatrixBuffer[i].Buffer;
			LightSpaceMatrixBufferInfo.offset = 0;
			LightSpaceMatrixBufferInfo.range = LightSpaceMatrixSize;

			VkWriteDescriptorSet WriteDescriptorSet = {};
			WriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSet.dstSet = LightSpaceMatrixSet[i];
			WriteDescriptorSet.dstBinding = 0;
			WriteDescriptorSet.dstArrayElement = 0;
			WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			WriteDescriptorSet.descriptorCount = 1;
			WriteDescriptorSet.pBufferInfo = &LightSpaceMatrixBufferInfo;
			WriteDescriptorSet.pImageInfo = nullptr;

			vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);
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

		VulkanHelper::PipelineResourceInfo ResourceInfo = { };
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 0;
		ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = DepthFormat;
		ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		// Create vertex binding descriptions for depth pass (only Position + 4 model matrix components)
		VkVertexInputBindingDescription VertexBindings[2];
		VertexBindings[0].binding = 0;
		VertexBindings[0].stride = sizeof(Render::StaticMeshVertex);
		VertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VertexBindings[1].binding = 1;
		VertexBindings[1].stride = sizeof(Render::InstanceData);
		VertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		// Create vertex attribute descriptions
		VkVertexInputAttributeDescription VertexAttributes[5]; // 1 Position + 4 model matrix components
		u32 AttributeIndex = 0;

		// Position attribute
		VertexAttributes[AttributeIndex].binding = 0;
		VertexAttributes[AttributeIndex].location = AttributeIndex;
		VertexAttributes[AttributeIndex].format = VK_FORMAT_R32G32B32_SFLOAT;
		VertexAttributes[AttributeIndex].offset = offsetof(Render::StaticMeshVertex, Position);
		AttributeIndex++;

		// Instance model matrix components
		u32 Offset = 0;
		VertexAttributes[AttributeIndex].binding = 1;
		VertexAttributes[AttributeIndex].location = AttributeIndex;
		VertexAttributes[AttributeIndex].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		VertexAttributes[AttributeIndex].offset = Offset;
		AttributeIndex++;
		Offset += sizeof(glm::vec4);

		VertexAttributes[AttributeIndex].binding = 1;
		VertexAttributes[AttributeIndex].location = AttributeIndex;
		VertexAttributes[AttributeIndex].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		VertexAttributes[AttributeIndex].offset = Offset;
		AttributeIndex++;
		Offset += sizeof(glm::vec4);

		VertexAttributes[AttributeIndex].binding = 1;
		VertexAttributes[AttributeIndex].location = AttributeIndex;
		VertexAttributes[AttributeIndex].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		VertexAttributes[AttributeIndex].offset = Offset;
		AttributeIndex++;
		Offset += sizeof(glm::vec4);

		VertexAttributes[AttributeIndex].binding = 1;
		VertexAttributes[AttributeIndex].location = AttributeIndex;
		VertexAttributes[AttributeIndex].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		VertexAttributes[AttributeIndex].offset = Offset;
		AttributeIndex++;

		VkPipelineVertexInputStateCreateInfo VertexInputState = {};
		VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputState.vertexBindingDescriptionCount = 2;
		VertexInputState.pVertexBindingDescriptions = VertexBindings;
		VertexInputState.vertexAttributeDescriptionCount = AttributeIndex;
		VertexInputState.pVertexAttributeDescriptions = VertexAttributes;

		VulkanHelper::PipelineSettings PipelineSettings;
		Util::LoadPipelineSettings(PipelineSettings, "./Resources/Settings/DepthPipeline.yaml");
		PipelineSettings.Extent = DepthViewportExtent;

		Pipeline.Pipeline = RenderResources::CreateGraphicsPipeline(Device, &VertexInputState, &PipelineSettings, &ResourceInfo);
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


		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);
	}

	void Draw(const Render::DrawScene* Scene, const Render::ResourceStorage* Storage)
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		const Render::RenderState* State = Render::GetRenderState();

		const glm::mat4* LightViews[] =
		{
			&Scene->LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene->LightEntity->SpotLight.LightSpaceMatrix,
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
				Render::DrawEntity* DrawEntity = Storage->DrawEntities.Data + i;
				if (!IsDrawEntityLoaded(Storage, DrawEntity))
				{
					continue;
				}

				Render::RenderResource<Render::StaticMesh>* MeshResource = Storage->Meshes.StaticMeshes.Data + DrawEntity->StaticMeshIndex;
				Render::StaticMesh* Mesh = &MeshResource->Resource;

				const VkBuffer Buffers[] =
				{
					Storage->Meshes.VertexStageData.Buffer,
					Storage->Meshes.GPUInstances.Buffer
				};

				const u64 Offsets[] =
				{
					Mesh->VertexOffset,
					sizeof(Render::InstanceData) * DrawEntity->InstanceDataIndex
				};

				const u32 DescriptorSetGroupCount = 1;
				const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
				{
					LightSpaceMatrixSet[LightCaster],
				};

				const VkPipelineLayout PipelineLayout = Pipeline.PipelineLayout;

				vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr);

				vkCmdBindVertexBuffers(CmdBuffer, 0, 2, Buffers, Offsets);
				vkCmdBindIndexBuffer(CmdBuffer, Storage->Meshes.VertexStageData.Buffer, Mesh->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CmdBuffer, Mesh->IndicesCount, DrawEntity->Instances, 0, 0, 0);
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