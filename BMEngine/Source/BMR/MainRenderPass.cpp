#include "MainRenderPass.h"

#include "VulkanHelper.h"
#include "VulkanMemoryManagementSystem.h"
#include "VulkanResourceManagementSystem.h"

#include "Memory/MemoryManagmentSystem.h"

#include "Util/Settings.h"


#include "BMRInterface.h"

namespace BMR
{
	namespace SubpassIndex
	{
		enum
		{
			MainSubpass = 0,
			DeferredSubpas, // Fullscreen effects

			Count
		};
	}

	static const u32 TmpDepthSubpassIndex = 0;

	void BMRMainRenderPass::ClearResources(VkDevice LogicalDevice, u32 ImagesCount)
	{
		for (u32 i = 0; i < ImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, DepthBufferViews[i], nullptr);
			VulkanMemoryManagementSystem::DestroyImageBuffer(DepthBuffers[i]);

			vkDestroyImageView(LogicalDevice, ColorBufferViews[i], nullptr);
			VulkanMemoryManagementSystem::DestroyImageBuffer(ColorBuffers[i]);

			//VulkanMemoryManagementSystem::DestroyBuffer(VpUniformBuffers[i]); TODO:Destroy set
			DestroyUniformBuffer(LightBuffers[i]);
			DestroyUniformBuffer(DepthLightSpaceMatrixBuffers[i]);

			vkDestroyImageView(LogicalDevice, ShadowArrayViews[i], nullptr);
			vkDestroyImageView(LogicalDevice, ShadowDepthBufferViews2[i], nullptr);
			vkDestroyImageView(LogicalDevice, ShadowDepthBufferViews1[i], nullptr);
			VulkanMemoryManagementSystem::DestroyImageBuffer(ShadowDepthBuffers[i]);
		}

		for (u32 i = 0; i < DescriptorLayoutHandles::Count; ++i)
		{
			vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorLayouts[i], nullptr);
		}

		DestroyUniformBuffer(MaterialBuffer);

		for (u32 i = 0; i < RenderPassHandles::Count; ++i)
		{
			vkDestroyRenderPass(LogicalDevice, RenderPasses[i], nullptr);
		}

		for (u32 i = 0; i < FrameBuffersHandles::Count; ++i)
		{
			for (u32 j = 0; j < ImagesCount; ++j)
			{
				vkDestroyFramebuffer(LogicalDevice, Framebuffers[i][j], nullptr);
			}
		}
	}

	void BMRMainRenderPass::CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat, VkSurfaceFormatKHR SurfaceFormat)
	{
		RenderPasses[RenderPassHandles::Main] = CreateRenderPass(&MainRenderPassSettings, SurfaceFormat);
		RenderPasses[RenderPassHandles::Depth] = CreateRenderPass(&DepthRenderPassSettings, SurfaceFormat);
	}

	void BMRMainRenderPass::CreateFrameBuffer(VkDevice LogicalDevice, VkExtent2D FrameBufferSizes, u32 ImagesCount,
		VkImageView SwapchainImageViews[MAX_SWAPCHAIN_IMAGES_COUNT])
	{
		VkRenderPass RenderPassTable[FrameBuffersHandles::Count];
		RenderPassTable[FrameBuffersHandles::Tex1] = RenderPasses[RenderPassHandles::Depth];
		RenderPassTable[FrameBuffersHandles::Tex2] = RenderPasses[RenderPassHandles::Depth];
		RenderPassTable[FrameBuffersHandles::Main] = RenderPasses[RenderPassHandles::Main];

		VkExtent2D ExtentTable[FrameBuffersHandles::Count];
		ExtentTable[FrameBuffersHandles::Tex1] = DepthViewportExtent;
		ExtentTable[FrameBuffersHandles::Tex2] = DepthViewportExtent;
		ExtentTable[FrameBuffersHandles::Main] = FrameBufferSizes;

		VkFramebufferCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

		for (u32 i = 0; i < FrameBuffersHandles::Count; ++i)
		{
			for (u32 j = 0; j < ImagesCount; j++)
			{
				const u32 MainPassAttachmentsCount = 3;
				VkImageView MainPassAttachments[MainPassAttachmentsCount] = {
					SwapchainImageViews[j],
					// Do not forget about position in array AttachmentDescriptions
					ColorBufferViews[j],
					DepthBufferViews[j]
				};

				const u32 DepthAttachmentsCount = 1;
				VkImageView DepthAttachments[DepthAttachmentsCount] = {
					ShadowDepthBufferViews1[j]
				};

				VkImageView DepthAttachments2[DepthAttachmentsCount] = {
					ShadowDepthBufferViews2[j]
				};

				u32 AttachmentCountTable[FrameBuffersHandles::Count];
				AttachmentCountTable[FrameBuffersHandles::Tex1] = DepthAttachmentsCount;
				AttachmentCountTable[FrameBuffersHandles::Tex2] = DepthAttachmentsCount;
				AttachmentCountTable[FrameBuffersHandles::Main] = MainPassAttachmentsCount;

				const VkImageView* AttachmentsTable[FrameBuffersHandles::Count];
				AttachmentsTable[FrameBuffersHandles::Tex1] = DepthAttachments;
				AttachmentsTable[FrameBuffersHandles::Tex2] = DepthAttachments2;
				AttachmentsTable[FrameBuffersHandles::Main] = MainPassAttachments;

				CreateInfo.renderPass = RenderPassTable[i];
				CreateInfo.attachmentCount = AttachmentCountTable[i];
				CreateInfo.pAttachments = AttachmentsTable[i]; // List of attachments (1:1 with Render Pass)
				CreateInfo.width = ExtentTable[i].width;
				CreateInfo.height = ExtentTable[i].height;
				CreateInfo.layers = 1;

				VkResult Result = vkCreateFramebuffer(LogicalDevice, &CreateInfo, nullptr, &(Framebuffers[i][j]));
				if (Result != VK_SUCCESS)
				{
					HandleLog(BMRLogType::LogType_Error, "vkCreateFramebuffer result is %d", Result);
				}
			}
		}
	}

	void BMRMainRenderPass::SetupPushConstants()
	{
		PushConstants[PushConstantHandles::Model].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants[PushConstantHandles::Model].offset = 0;
		// Todo: check constant and model size?
		PushConstants[PushConstantHandles::Model].size = sizeof(BMRModel);
	}

	void BMRMainRenderPass::CreateDescriptorLayouts(VkDevice LogicalDevice)
	{
		const u32 VpLayoutBindingBindingCount = 1;
		VkDescriptorSetLayoutBinding VpLayoutBinding = { };
		VpLayoutBinding.binding = 0;
		VpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VpLayoutBinding.descriptorCount = 1;
		VpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		VpLayoutBinding.pImmutableSamplers = nullptr; // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

		const u32 EntitySamplerLayoutBindingCount = 2;
		VkDescriptorSetLayoutBinding EntitySamplerLayoutBinding[EntitySamplerLayoutBindingCount];
		EntitySamplerLayoutBinding[0].binding = 0;
		EntitySamplerLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		EntitySamplerLayoutBinding[0].descriptorCount = 1;
		EntitySamplerLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		EntitySamplerLayoutBinding[0].pImmutableSamplers = nullptr;
		EntitySamplerLayoutBinding[1] = EntitySamplerLayoutBinding[0];
		EntitySamplerLayoutBinding[1].binding = 1;

		VkDescriptorSetLayoutBinding TerrainSamplerLayoutBinding = { };
		TerrainSamplerLayoutBinding.binding = 0;
		TerrainSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		TerrainSamplerLayoutBinding.descriptorCount = 1;
		TerrainSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		TerrainSamplerLayoutBinding.pImmutableSamplers = nullptr;

		const u32 LightBindingsCount = 1;
		VkDescriptorSetLayoutBinding LightBindings[LightBindingsCount];
		LightBindings[0] = { };
		LightBindings[0].binding = 0;
		LightBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		LightBindings[0].descriptorCount = 1;
		LightBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		LightBindings[0].pImmutableSamplers = nullptr;

		const u32 MaterialBindingsCount = 1;
		VkDescriptorSetLayoutBinding MaterialLayoutBinding = { };
		MaterialLayoutBinding.binding = 0;
		MaterialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		MaterialLayoutBinding.descriptorCount = 1;
		MaterialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		MaterialLayoutBinding.pImmutableSamplers = nullptr;

		const u32 DeferredAttachmentBindingsCount = 2;
		VkDescriptorSetLayoutBinding DeferredInputBindings[DeferredAttachmentBindingsCount];
		DeferredInputBindings[0].binding = 0;
		DeferredInputBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		DeferredInputBindings[0].descriptorCount = 1;
		DeferredInputBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		DeferredInputBindings[1] = DeferredInputBindings[0];
		DeferredInputBindings[1].binding = 1;

		VkDescriptorSetLayoutBinding SkyBoxSamplerLayoutBinding = { };
		SkyBoxSamplerLayoutBinding.binding = 0;
		SkyBoxSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		SkyBoxSamplerLayoutBinding.descriptorCount = 1;
		SkyBoxSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		SkyBoxSamplerLayoutBinding.pImmutableSamplers = nullptr;

		const u32 DepthVpLayoutBindingBindingCount = 1;
		VkDescriptorSetLayoutBinding DepthVpLayoutBinding = { };
		DepthVpLayoutBinding.binding = 0;
		DepthVpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		DepthVpLayoutBinding.descriptorCount = 1;
		DepthVpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		DepthVpLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding ShadowMapAttachmentSamplerLayoutBinding = { };
		ShadowMapAttachmentSamplerLayoutBinding.binding = 0;
		ShadowMapAttachmentSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ShadowMapAttachmentSamplerLayoutBinding.descriptorCount = 1;
		ShadowMapAttachmentSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		ShadowMapAttachmentSamplerLayoutBinding.pImmutableSamplers = nullptr;

		u32 BindingCountTable[DescriptorLayoutHandles::Count];
		BindingCountTable[DescriptorLayoutHandles::TerrainSampler] = 1;
		BindingCountTable[DescriptorLayoutHandles::EntitySampler] = EntitySamplerLayoutBindingCount;
		BindingCountTable[DescriptorLayoutHandles::EntityLigh] = LightBindingsCount;
		BindingCountTable[DescriptorLayoutHandles::EntityMaterial] = MaterialBindingsCount;
		BindingCountTable[DescriptorLayoutHandles::DeferredInputAttachments] = DeferredAttachmentBindingsCount;
		BindingCountTable[DescriptorLayoutHandles::SkyBoxSampler] = 1;
		BindingCountTable[DescriptorLayoutHandles::DepthLightSpaceMatrix] = DepthVpLayoutBindingBindingCount;
		BindingCountTable[DescriptorLayoutHandles::EntityShadowMapSampler] = 1;

		const VkDescriptorSetLayoutBinding* BindingsTable[DescriptorLayoutHandles::Count];
		BindingsTable[DescriptorLayoutHandles::TerrainSampler] = &TerrainSamplerLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::EntitySampler] = EntitySamplerLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::EntityLigh] = LightBindings;
		BindingsTable[DescriptorLayoutHandles::EntityMaterial] = &MaterialLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::DeferredInputAttachments] = DeferredInputBindings;
		BindingsTable[DescriptorLayoutHandles::SkyBoxSampler] = &SkyBoxSamplerLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::DepthLightSpaceMatrix] = &DepthVpLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::EntityShadowMapSampler] = &ShadowMapAttachmentSamplerLayoutBinding;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfos[DescriptorLayoutHandles::Count];
		for (u32 i = 0; i < DescriptorLayoutHandles::Count; ++i)
		{
			LayoutCreateInfos[i] = { };
			LayoutCreateInfos[i].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfos[i].bindingCount = BindingCountTable[i];
			LayoutCreateInfos[i].pBindings = BindingsTable[i];

			const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &(LayoutCreateInfos[i]), nullptr, &(DescriptorLayouts[i]));
			if (Result != VK_SUCCESS)
			{
				HandleLog(BMRLogType::LogType_Error, "vkCreateDescriptorSetLayout result is %d", Result);
			}
		}

		VkDescriptorType bt[] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
		u32 st[] = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT };


		TestVpLayout = CreateUniformLayout(bt, st, 1);
	}

	void BMRMainRenderPass::CreatePipelineLayouts(VkDevice LogicalDevice)
	{
		const u32 TerrainDescriptorLayoutsCount = 2;
		VkDescriptorSetLayout TerrainDescriptorLayouts[TerrainDescriptorLayoutsCount] = {
			(VkDescriptorSetLayout)TestVpLayout,
			DescriptorLayouts[DescriptorLayoutHandles::TerrainSampler]
		};

		const u32 EntityDescriptorLayoutCount = 5;
		VkDescriptorSetLayout EntityDescriptorLayouts[EntityDescriptorLayoutCount] = {
			(VkDescriptorSetLayout)TestVpLayout,
			DescriptorLayouts[DescriptorLayoutHandles::EntitySampler],
			DescriptorLayouts[DescriptorLayoutHandles::EntityLigh],
			DescriptorLayouts[DescriptorLayoutHandles::EntityMaterial],
			DescriptorLayouts[DescriptorLayoutHandles::EntityShadowMapSampler],
		};

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] = {
			(VkDescriptorSetLayout)TestVpLayout,
			DescriptorLayouts[DescriptorLayoutHandles::SkyBoxSampler],
		};

		u32 SetLayoutCountTable[BMRPipelineHandles::PipelineHandlesCount];
		SetLayoutCountTable[BMRPipelineHandles::Entity] = EntityDescriptorLayoutCount;
		SetLayoutCountTable[BMRPipelineHandles::Terrain] = TerrainDescriptorLayoutsCount;
		SetLayoutCountTable[BMRPipelineHandles::Deferred] = 1;
		SetLayoutCountTable[BMRPipelineHandles::SkyBox] = SkyBoxDescriptorLayoutCount;
		SetLayoutCountTable[BMRPipelineHandles::Depth] = 1;

		const VkDescriptorSetLayout* SetLayouts[BMRPipelineHandles::PipelineHandlesCount];
		SetLayouts[BMRPipelineHandles::Entity] = EntityDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Terrain] = TerrainDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Deferred] = DescriptorLayouts + DescriptorLayoutHandles::DeferredInputAttachments;
		SetLayouts[BMRPipelineHandles::SkyBox] = SkyBoxDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Depth] = DescriptorLayouts + DescriptorLayoutHandles::DepthLightSpaceMatrix;

		u32 PushConstantRangeCountTable[BMRPipelineHandles::PipelineHandlesCount];
		PushConstantRangeCountTable[BMRPipelineHandles::Entity] = 1;
		PushConstantRangeCountTable[BMRPipelineHandles::Terrain] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::Deferred] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::SkyBox] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::Depth] = 1;


		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			Pipelines[i].PipelineLayout = VulkanResourceManagementSystem::CreatePipelineLayout(SetLayouts[i],
				SetLayoutCountTable[i], &PushConstants[PushConstantHandles::Model], PushConstantRangeCountTable[i]);
		}
	}

	void BMRMainRenderPass::CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent,
		BMRPipelineShaderInputDepr ShaderInputs[BMRShaderNames::ShaderNamesCount])
	{
		const VkShaderStageFlagBits NamesToStagesTable[] =
		{
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		BMRSPipelineShaderInfo ShaderInfos[BMRPipelineHandles::PipelineHandlesCount];
		for (u32 i = 0; i < BMRShaderNames::ShaderNamesCount; ++i)
		{
			const BMRPipelineShaderInputDepr& Input = ShaderInputs[i];
			BMRSPipelineShaderInfo& Info = ShaderInfos[Input.Handle];

			Info.Infos[Info.InfosCounter] = { };
			Info.Infos[Info.InfosCounter].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Info.Infos[Info.InfosCounter].stage = NamesToStagesTable[Input.Stage];
			Info.Infos[Info.InfosCounter].pName = Input.EntryPoint;
			CreateShader(LogicalDevice, Input.Code, Input.CodeSize, Info.Infos[Info.InfosCounter].module);
			++Info.InfosCounter;
		}

		BMRVertexInput VertexInput[BMRPipelineHandles::PipelineHandlesCount];
		VertexInput[BMRPipelineHandles::Entity] = EntityVertexInput;
		VertexInput[BMRPipelineHandles::Terrain] = TerrainVertexInput;
		VertexInput[BMRPipelineHandles::Deferred] = { };
		VertexInput[BMRPipelineHandles::SkyBox] = SkyBoxVertexInput;
		VertexInput[BMRPipelineHandles::Depth] = DepthVertexInput;

		BMRPipelineSettings PipelineSettings[BMRPipelineHandles::PipelineHandlesCount];
		PipelineSettings[BMRPipelineHandles::Entity] = EntityPipelineSettings;
		PipelineSettings[BMRPipelineHandles::Terrain] = TerrainPipelineSettings;
		PipelineSettings[BMRPipelineHandles::Deferred] = DeferredPipelineSettings;
		PipelineSettings[BMRPipelineHandles::SkyBox] = SkyBoxPipelineSettings;
		PipelineSettings[BMRPipelineHandles::Depth] = DepthPipelineSettings;


		BMRPipelineResourceInfo PipelineResourceInfo[BMRPipelineHandles::PipelineHandlesCount];
		PipelineResourceInfo[BMRPipelineHandles::Entity].PipelineLayout = Pipelines[BMRPipelineHandles::Entity].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::Entity].RenderPass = RenderPasses[RenderPassHandles::Main];
		PipelineResourceInfo[BMRPipelineHandles::Entity].SubpassIndex = SubpassIndex::MainSubpass;

		PipelineResourceInfo[BMRPipelineHandles::Terrain].PipelineLayout = Pipelines[BMRPipelineHandles::Terrain].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::Terrain].RenderPass = RenderPasses[RenderPassHandles::Main];
		PipelineResourceInfo[BMRPipelineHandles::Terrain].SubpassIndex = SubpassIndex::MainSubpass;

		PipelineResourceInfo[BMRPipelineHandles::Deferred].PipelineLayout = Pipelines[BMRPipelineHandles::Deferred].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::Deferred].RenderPass = RenderPasses[RenderPassHandles::Main];
		PipelineResourceInfo[BMRPipelineHandles::Deferred].SubpassIndex = SubpassIndex::DeferredSubpas;

		PipelineResourceInfo[BMRPipelineHandles::SkyBox].PipelineLayout = Pipelines[BMRPipelineHandles::SkyBox].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::SkyBox].RenderPass = RenderPasses[RenderPassHandles::Main];
		PipelineResourceInfo[BMRPipelineHandles::SkyBox].SubpassIndex = SubpassIndex::MainSubpass;

		PipelineResourceInfo[BMRPipelineHandles::Depth].PipelineLayout = Pipelines[BMRPipelineHandles::Depth].PipelineLayout;
		PipelineResourceInfo[BMRPipelineHandles::Depth].RenderPass = RenderPasses[RenderPassHandles::Depth];
		PipelineResourceInfo[BMRPipelineHandles::Depth].SubpassIndex = TmpDepthSubpassIndex;


		VkPipeline* NewPipelines = VulkanResourceManagementSystem::CreatePipelines(ShaderInfos, VertexInput, PipelineSettings,
			PipelineResourceInfo, BMRPipelineHandles::PipelineHandlesCount);

		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			Pipelines[i].Pipeline = NewPipelines[i];
		}
	}

	void BMRMainRenderPass::CreateImages(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 ImagesCount, VkExtent2D SwapExtent,
		VkFormat DepthFormat, VkFormat ColorFormat)
	{
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = 0;

		for (u32 i = 0; i < ImagesCount; ++i)
		{
			ImageCreateInfo.extent.width = SwapExtent.width;
			ImageCreateInfo.extent.height = SwapExtent.height;
			ImageCreateInfo.format = DepthFormat;
			ImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

			DepthBuffers[i] = VulkanMemoryManagementSystem::CreateImageBuffer(&ImageCreateInfo);
			DepthBufferViews[i] = CreateImageView(LogicalDevice, DepthBuffers[i].Image,
				DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

			ImageCreateInfo.format = ColorFormat;
			ImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

			ColorBuffers[i] = VulkanMemoryManagementSystem::CreateImageBuffer(&ImageCreateInfo);
			ColorBufferViews[i] = CreateImageView(LogicalDevice, ColorBuffers[i].Image,
				ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

			ImageCreateInfo.extent.width = DepthViewportExtent.width;
			ImageCreateInfo.extent.height = DepthViewportExtent.height;
			ImageCreateInfo.format = DepthFormat;
			ImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			ImageCreateInfo.arrayLayers = MAX_LIGHT_SOURCES;

			ShadowDepthBuffers[i] = VulkanMemoryManagementSystem::CreateImageBuffer(&ImageCreateInfo);
			ShadowArrayViews[i] = CreateImageView(LogicalDevice, ShadowDepthBuffers[i].Image,
				DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, MAX_LIGHT_SOURCES);

			ShadowDepthBufferViews1[i] = CreateImageView(LogicalDevice, ShadowDepthBuffers[i].Image,
				DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1, 0);
			ShadowDepthBufferViews2[i] = CreateImageView(LogicalDevice, ShadowDepthBuffers[i].Image,
				DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1, 1);
		}
	}

	void BMRMainRenderPass::CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
		u32 ImagesCount)
	{
		const VkDeviceSize VpBufferSize = sizeof(BMRUboViewProjection);
		const VkDeviceSize LightBufferSize = sizeof(BMRLightBuffer);
		const VkDeviceSize MaterialBufferSize = sizeof(BMRMaterial);
		const VkDeviceSize LightSpaceBufferSize = sizeof(BMRLightSpaceMatrix);

		//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);
		//const VkDeviceSize AlignedAmbientLightSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(AmbientLightBufferSize);
		//const VkDeviceSize AlignedPointLightSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(PointLightBufferSize);

		for (u32 i = 0; i < ImagesCount; i++)
		{
			TestVpBuffer[i] = CreateUniformBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VpBufferSize);

			LightBuffers[i] = CreateUniformBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, LightBufferSize);


			DepthLightSpaceMatrixBuffers[i] = CreateUniformBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, LightSpaceBufferSize);
		}

		MaterialBuffer = CreateUniformBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, MaterialBufferSize);
	}

	void BMRMainRenderPass::CreateSets(VkDescriptorPool Pool, VkDevice LogicalDevice, u32 ImagesCount,
		VkSampler ShadowMapSampler)
	{
		VkDescriptorSetLayout Layouts[DescriptorLayoutHandles::Count][MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSetLayout SetLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout TerrainLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout LightingSetLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout DeferredSetLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout SkyBoxVpLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout DepthLightSpaceLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout ShadowMapLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];

		for (u32 i = 0; i < ImagesCount; i++)
		{
			LightingSetLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::EntityLigh];
			DeferredSetLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::DeferredInputAttachments];
			DepthLightSpaceLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::DepthLightSpaceMatrix];
			ShadowMapLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::EntityShadowMapSampler];
		}

		VulkanMemoryManagementSystem::AllocateSets(Pool, LightingSetLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::EntityLigh]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, DeferredSetLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::DeferredInputAttachments]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, DepthLightSpaceLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::DepthLightSpaceMatrix]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, ShadowMapLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::ShadowMapSampler]);

		for (u32 i = 0; i < ImagesCount; i++)
		{
			VkDescriptorBufferInfo DepthLightSpaceBufferInfo = { };
			DepthLightSpaceBufferInfo.buffer = DepthLightSpaceMatrixBuffers[i].Buffer;
			DepthLightSpaceBufferInfo.offset = 0;
			DepthLightSpaceBufferInfo.range = sizeof(BMRLightSpaceMatrix);

			VkDescriptorBufferInfo LightBufferInfo = { };
			LightBufferInfo.buffer = LightBuffers[i].Buffer;
			LightBufferInfo.offset = 0;
			LightBufferInfo.range = sizeof(BMRLightBuffer);

			VkDescriptorImageInfo ColourAttachmentDescriptor = { };
			ColourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColourAttachmentDescriptor.imageView = ColorBufferViews[i];
			ColourAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkDescriptorImageInfo DepthAttachmentDescriptor = { };
			DepthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthAttachmentDescriptor.imageView = DepthBufferViews[i];
			DepthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkDescriptorImageInfo ShadowMapTextureDescriptor = { };
			ShadowMapTextureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapTextureDescriptor.imageView = ShadowArrayViews[i];
			ShadowMapTextureDescriptor.sampler = ShadowMapSampler;

			VkWriteDescriptorSet ShadowDepthWrite = { };
			ShadowDepthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ShadowDepthWrite.dstSet = DescriptorsToImages[DescriptorHandles::ShadowMapSampler][i];
			ShadowDepthWrite.dstBinding = 0;
			ShadowDepthWrite.dstArrayElement = 0;
			ShadowDepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			ShadowDepthWrite.descriptorCount = 1;
			ShadowDepthWrite.pImageInfo = &ShadowMapTextureDescriptor;

			VkWriteDescriptorSet DepthLightSpaceWrite = { };
			DepthLightSpaceWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DepthLightSpaceWrite.dstSet = DescriptorsToImages[DescriptorHandles::DepthLightSpaceMatrix][i];
			DepthLightSpaceWrite.dstBinding = 0;
			DepthLightSpaceWrite.dstArrayElement = 0;
			DepthLightSpaceWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			DepthLightSpaceWrite.descriptorCount = 1;
			DepthLightSpaceWrite.pBufferInfo = &DepthLightSpaceBufferInfo;

			VkWriteDescriptorSet LightSetWrite = { };
			LightSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			LightSetWrite.dstSet = DescriptorsToImages[DescriptorHandles::EntityLigh][i];
			LightSetWrite.dstBinding = 0;
			LightSetWrite.dstArrayElement = 0;
			LightSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			LightSetWrite.descriptorCount = 1;
			LightSetWrite.pBufferInfo = &LightBufferInfo;

			VkWriteDescriptorSet ColourWrite = { };
			ColourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ColourWrite.dstSet = DescriptorsToImages[DescriptorHandles::DeferredInputAttachments][i];
			ColourWrite.dstBinding = 0;
			ColourWrite.dstArrayElement = 0;
			ColourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			ColourWrite.descriptorCount = 1;
			ColourWrite.pImageInfo = &ColourAttachmentDescriptor;

			VkWriteDescriptorSet DepthWrite = { };
			DepthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DepthWrite.dstSet = DescriptorsToImages[DescriptorHandles::DeferredInputAttachments][i];
			DepthWrite.dstBinding = 1;
			DepthWrite.dstArrayElement = 0;
			DepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			DepthWrite.descriptorCount = 1;
			DepthWrite.pImageInfo = &DepthAttachmentDescriptor;

			const u32 WriteSetsCount = 5;
			VkWriteDescriptorSet WriteSets[WriteSetsCount] = { LightSetWrite,
				ColourWrite, DepthWrite, DepthLightSpaceWrite, ShadowDepthWrite };

			vkUpdateDescriptorSets(LogicalDevice, WriteSetsCount, WriteSets, 0, nullptr);

			CreateUniformSets(&TestVpLayout, 1, TestVpSet + i);
			VkDeviceSize test = sizeof(BMRUboViewProjection);
			AttachBuffersToSet(TestVpSet[i], &(TestVpBuffer[i]), &test, 1);
		}

		VulkanMemoryManagementSystem::AllocateSets(Pool, &DescriptorLayouts[DescriptorLayoutHandles::EntityMaterial], 1, &MaterialSet);

		VkDescriptorBufferInfo MaterialBufferInfo = { };
		MaterialBufferInfo.buffer = MaterialBuffer.Buffer;
		MaterialBufferInfo.offset = 0;
		MaterialBufferInfo.range = sizeof(BMRMaterial);

		VkWriteDescriptorSet MaterialSetWrite = { };
		MaterialSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		MaterialSetWrite.dstSet = MaterialSet;
		MaterialSetWrite.dstBinding = 0;
		MaterialSetWrite.dstArrayElement = 0;
		MaterialSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		MaterialSetWrite.descriptorCount = 1;
		MaterialSetWrite.pBufferInfo = &MaterialBufferInfo;

		vkUpdateDescriptorSets(LogicalDevice, 1, &MaterialSetWrite, 0, nullptr);
	}
}
