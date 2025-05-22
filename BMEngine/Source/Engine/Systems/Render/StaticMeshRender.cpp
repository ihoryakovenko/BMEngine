#include "StaticMeshRender.h"

#include "Render/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/Render.h"
#include "Render/FrameManager.h"
#include "Engine/Systems/Render/RenderResources.h"
#include "Engine/Systems/Render/LightningPass.h"
#include "Engine/Systems/Render/MainPass.h"

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "Engine/Systems/ResourceManager.h"
#include "Engine/Scene.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace StaticMeshRender
{
	static const char ModelsBaseDir[] = "./Resources/Models/";

	static VulkanInterface::RenderPipeline Pipeline;
	static VkSampler ShadowMapArraySampler;

	static VkDescriptorSetLayout StaticMeshLightLayout;
	static VkDescriptorSetLayout ShadowMapArrayLayout;

	static FrameManager::UniformMemoryHnadle EntityLightBufferHandle;
	
	static VkImageView ShadowMapArrayImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkPushConstantRange PushConstants;

	static VkDescriptorSet StaticMeshLightSet;
	static VkDescriptorSet ShadowMapArraySet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	struct PushConstantsData
	{
		glm::mat4 Model;
		s32 matIndex;
	};

	void Init()
	{
		VkSamplerCreateInfo ShadowMapSamplerCreateInfo = { };
		ShadowMapSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		ShadowMapSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
		ShadowMapSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
		ShadowMapSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;	// How to handle texture wrap in U (x) direction
		ShadowMapSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;	// How to handle texture wrap in V (y) direction
		ShadowMapSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;	// How to handle texture wrap in W (z) direction
		ShadowMapSamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
		ShadowMapSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
		ShadowMapSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
		ShadowMapSamplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
		ShadowMapSamplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
		ShadowMapSamplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
		ShadowMapSamplerCreateInfo.anisotropyEnable = VK_TRUE;
		ShadowMapSamplerCreateInfo.maxAnisotropy = 1; // Todo: support in config

		ShadowMapArraySampler = VulkanInterface::CreateSampler(&ShadowMapSamplerCreateInfo);


		const VkDeviceSize LightBufferSize = sizeof(Render::LightBuffer);
		StaticMeshLightLayout = FrameManager::CreateCompatibleLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		EntityLightBufferHandle = FrameManager::ReserveUniformMemory(LightBufferSize);
		StaticMeshLightSet = FrameManager::CreateAndBindSet(EntityLightBufferHandle, LightBufferSize, StaticMeshLightLayout);

		const VkDescriptorType ShadowMapArrayDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags ShadowMapArrayFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		const VkDescriptorBindingFlags ShadowMapArrayBindingFlags[1] = { };
		ShadowMapArrayLayout = VulkanInterface::CreateUniformLayout(&ShadowMapArrayDescriptorType, &ShadowMapArrayFlags, ShadowMapArrayBindingFlags, 1, 1);

		VulkanInterface::UniformImageInterfaceCreateInfo ShadowMapArrayInterfaceCreateInfo = { };
		ShadowMapArrayInterfaceCreateInfo.Flags = 0; // No flags
		ShadowMapArrayInterfaceCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		ShadowMapArrayInterfaceCreateInfo.Format = DepthFormat;
		ShadowMapArrayInterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapArrayInterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapArrayInterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapArrayInterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.levelCount = 1;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
		ShadowMapArrayInterfaceCreateInfo.SubresourceRange.layerCount = 2;

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			ShadowMapArrayImageInterface[i] = VulkanInterface::CreateImageInterface(&ShadowMapArrayInterfaceCreateInfo,
				LightningPass::GetShadowMapArray()[i].Image);

			VulkanInterface::UniformSetAttachmentInfo ShadowMapArrayAttachmentInfo;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageView = ShadowMapArrayImageInterface[i];
			ShadowMapArrayAttachmentInfo.ImageInfo.sampler = ShadowMapArraySampler;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapArrayAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			VulkanInterface::CreateUniformSets(&ShadowMapArrayLayout, 1, ShadowMapArraySet + i);
			VulkanInterface::AttachUniformsToSet(ShadowMapArraySet[i], &ShadowMapArrayAttachmentInfo, 1);
		}

		VkDescriptorSetLayout StaticMeshDescriptorLayouts[] =
		{
			FrameManager::GetViewProjectionLayout(),
			RenderResources::GetBindlesTexturesLayout(),
			StaticMeshLightLayout,
			RenderResources::TmpGetMaterialLayout(),
			ShadowMapArrayLayout
		};

		const u32 StaticMeshDescriptorLayoutCount = sizeof(StaticMeshDescriptorLayouts) / sizeof(StaticMeshDescriptorLayouts[0]);

		PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants.offset = 0;
		// Todo: check constant and model size?
		PushConstants.size = sizeof(PushConstantsData);

		Pipeline.PipelineLayout = VulkanHelper::CreatePipelineLayout(VulkanInterface::GetDevice(), StaticMeshDescriptorLayouts, StaticMeshDescriptorLayoutCount, &PushConstants, 1);

		const u32 ShaderCount = 2;
		VulkanInterface::Shader Shaders[ShaderCount];

		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/vert.spv", VertexShaderCode, "rb");
		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/frag.spv", FragmentShaderCode, "rb");

		Shaders[0].Stage = VK_SHADER_STAGE_VERTEX_BIT;
		Shaders[0].Code = VertexShaderCode.data();
		Shaders[0].CodeSize = VertexShaderCode.size();

		Shaders[1].Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		Shaders[1].Code = FragmentShaderCode.data();
		Shaders[1].CodeSize = FragmentShaderCode.size();

		VulkanInterface::BMRVertexInputBinding VertexInputBinding[1];
		VertexInputBinding[0].InputAttributes[0] = { "Position", VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMeshVertex, Position) };
		VertexInputBinding[0].InputAttributes[1] = { "TextureCoords", VK_FORMAT_R32G32_SFLOAT, offsetof(StaticMeshVertex, TextureCoords) };
		VertexInputBinding[0].InputAttributes[2] = { "Normal", VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMeshVertex, Normal) };
		VertexInputBinding[0].InputAttributesCount = 3;
		VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VertexInputBinding[0].Stride = sizeof(StaticMeshVertex);
		VertexInputBinding[0].VertexInputBindingName = "EntityVertex";

		VulkanInterface::PipelineSettings PipelineSettings;
		Util::LoadPipelineSettings(PipelineSettings, "./Resources/Settings/StaticMeshRender.ini");
		PipelineSettings.Extent = MainScreenExtent;

		VulkanInterface::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		Pipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, VertexInputBinding, 1,
			&PipelineSettings, &ResourceInfo);
	}

	void DeInit()
	{
		VkDevice Device = VulkanInterface::GetDevice();

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkDestroyImageView(Device, ShadowMapArrayImageInterface[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(Device, StaticMeshLightLayout, nullptr);
		vkDestroyDescriptorSetLayout(Device, ShadowMapArrayLayout, nullptr);

		vkDestroyPipeline(Device, Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, Pipeline.PipelineLayout, nullptr);

		vkDestroySampler(Device, ShadowMapArraySampler, nullptr);
	}

	void Draw()
	{
		FrameManager::UpdateUniformMemory(EntityLightBufferHandle, Scene.LightEntity, sizeof(Render::LightBuffer));

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);
		
		const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet();
		const u32 DynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
			0, 1, &VpSet, 1, &DynamicOffset);

		VkDescriptorSet BindlesTexturesSet = RenderResources::GetBindlesTexturesSet();
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
			1, 1, &BindlesTexturesSet, 0, nullptr);

		const u32 LightDynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(Render::LightBuffer);

		u32 EntitiesCount;
		RenderResources::DrawEntity* Entities = RenderResources::GetEntities(&EntitiesCount);
		const Render::RenderState* State = Render::GetRenderState();

		for (u32 i = 0; i < EntitiesCount; ++i)
		{
			RenderResources::DrawEntity* DrawEntity = Entities + i;
			Render::StaticMesh* Mesh = State->StaticMeshLinearStorage.StaticMeshes.Data + DrawEntity->StaticMeshIndex;

			const VkDescriptorSet DescriptorSetGroup[] =
			{
				StaticMeshLightSet,
				RenderResources::TmpGetMaterialSet(),
				ShadowMapArraySet[VulkanInterface::TestGetImageIndex()],
			};
			const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

			PushConstantsData Constants;
			Constants.Model = DrawEntity->Model;
			Constants.matIndex = DrawEntity->MaterialIndex;

			const VkShaderStageFlags Flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			vkCmdPushConstants(CmdBuffer, Pipeline.PipelineLayout, Flags, 0, sizeof(PushConstantsData), &Constants);

			vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
				2, DescriptorSetGroupCount, DescriptorSetGroup, 1, &LightDynamicOffset);

			vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &State->StaticMeshLinearStorage.VertexBuffer.Buffer, &Mesh->VertexOffset);
			vkCmdBindIndexBuffer(CmdBuffer, State->StaticMeshLinearStorage.IndexBuffer.Buffer, Mesh->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CmdBuffer, Mesh->IndicesCount, 1, 0, 0, 0);
		}
	}
}
