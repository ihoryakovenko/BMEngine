#include "Render.h"

#include <vulkan/vulkan.h>

#include "Memory/MemoryManagmentSystem.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "FrameManager.h"
#include "MainRenderPass.h"
#include "Engine/Systems/TerrainSystem.h"

namespace Render
{
	struct BMRPassSharedResources
	{
		VulkanInterface::VertexBuffer VertexBuffer;
		u32 VertexBufferOffset = 0;

		VulkanInterface::IndexBuffer IndexBuffer;
		u32 IndexBufferOffset = 0;

		VulkanInterface::UniformImage ShadowMapArray[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkPushConstantRange PushConstants;
	};

	struct BMRDepthPass
	{
		VkDescriptorSetLayout LightSpaceMatrixLayout = nullptr;

		VulkanInterface::UniformBuffer LightSpaceMatrixBuffer[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSet LightSpaceMatrixSet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkImageView ShadowMapElement1ImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowMapElement2ImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VulkanInterface::RenderPipeline Pipeline;
		VulkanInterface::RenderPass RenderPass;
	};

	struct BMRMainPass
	{
		VkDescriptorSetLayout DeferredInputLayout;
		VkDescriptorSetLayout SkyBoxLayout;

		
		VulkanInterface::UniformImage DeferredInputDepthImage[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VulkanInterface::UniformImage DeferredInputColorImage[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkImageView DeferredInputDepthImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DeferredInputColorImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSet DeferredInputSet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VulkanInterface::RenderPipeline Pipelines[MainPathPipelinesCount];
		VulkanInterface::RenderPass RenderPass;
	};


	static void CreatePassSharedResources();
	static void CreateDepthPass();
	static void CreateMainPass();

	static void DestroyPassSharedResources();
	static void DestroyDepthPass();
	static void DestroyMainPass();

	static void DepthPassDraw(const DrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex);
	static void MainPassDraw(const DrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex);

	static BMRPassSharedResources PassSharedResources;
	static BMRDepthPass DepthPass;
	static BMRMainPass MainPass;

	static std::vector<OnDrawDelegate> OnDrawFunctions;

	static VkCommandBuffer TestCurrentFrameCommandBuffer = nullptr;

	void PreRenderInit()
	{
	}

	bool Init()
	{
		CreatePassSharedResources();
		CreateDepthPass();
		CreateMainPass();

		return true;
	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();
		DestroyDepthPass();
		DestroyMainPass();
		DestroyPassSharedResources();
	}

	void AddDrawFunction(OnDrawDelegate Delegate)
	{
		OnDrawFunctions.push_back(Delegate);
	}

	VkDescriptorSetLayout TestGetTerrainSkyBoxLayout()
	{
		return MainPass.SkyBoxLayout;
	}

	VkRenderPass TestGetRenderPass()
	{
		return MainPass.RenderPass.Pass;
	}

	void TestUpdateUniforBuffer(VulkanInterface::UniformBuffer* Buffer, u64 DataSize, u64 Offset, const void* Data)
	{
		VulkanInterface::UpdateUniformBuffer(Buffer[VulkanInterface::TestGetImageIndex()], DataSize, Offset,
			Data);
	}

	RenderTexture CreateTexture(TextureArrayInfo* Info)
	{
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Info->Width;
		ImageCreateInfo.extent.height = Info->Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = Info->LayersCount;
		ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = Info->Flags;

		VulkanInterface::UniformImageInterfaceCreateInfo InterfaceCreateInfo = { };
		InterfaceCreateInfo.Flags = 0;
		InterfaceCreateInfo.ViewType = Info->ViewType;
		InterfaceCreateInfo.Format = VK_FORMAT_R8G8B8A8_SRGB;
		InterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		InterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
		InterfaceCreateInfo.SubresourceRange.levelCount = 1;
		InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
		InterfaceCreateInfo.SubresourceRange.layerCount = Info->LayersCount;

		RenderTexture Texture;
		Texture.UniformData = VulkanInterface::CreateUniformImage(&ImageCreateInfo);
		Texture.ImageView = VulkanInterface::CreateImageInterface(&InterfaceCreateInfo, Texture.UniformData.Image);

		VulkanInterface::LayoutLayerTransitionData TransitionData;
		TransitionData.BaseArrayLayer = 0;
		TransitionData.LayerCount = Info->LayersCount;

		// FIRST TRANSITION
		VulkanInterface::TransitImageLayout(Texture.UniformData.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			&TransitionData, 1);

		VulkanInterface::CopyDataToImage(Texture.UniformData.Image, Info->Width, Info->Height, Info->Format, Info->LayersCount, Info->Data);

		// SECOND TRANSITION
		TransitImageLayout(Texture.UniformData.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, &TransitionData, 1);

		return Texture;
	}

	RenderTexture CreateEmptyTexture(TextureArrayInfo* Info)
	{
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Info->Width;
		ImageCreateInfo.extent.height = Info->Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = Info->LayersCount;
		ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = Info->Flags;

		VulkanInterface::UniformImageInterfaceCreateInfo InterfaceCreateInfo = { };
		InterfaceCreateInfo.Flags = 0;
		InterfaceCreateInfo.ViewType = Info->ViewType;
		InterfaceCreateInfo.Format = VK_FORMAT_R8G8B8A8_SRGB;
		InterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		InterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		InterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
		InterfaceCreateInfo.SubresourceRange.levelCount = 1;
		InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
		InterfaceCreateInfo.SubresourceRange.layerCount = Info->LayersCount;

		RenderTexture Texture;
		Texture.UniformData = VulkanInterface::CreateUniformImage(&ImageCreateInfo);
		Texture.ImageView = VulkanInterface::CreateImageInterface(&InterfaceCreateInfo, Texture.UniformData.Image);

		VulkanInterface::LayoutLayerTransitionData TransitionData;
		TransitionData.BaseArrayLayer = 0;
		TransitionData.LayerCount = Info->LayersCount;
		
		// If texture is empty perform transit to shader optimal layout to allow draw
		VulkanInterface::TransitImageLayout(Texture.UniformData.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			&TransitionData, 1);

		return Texture;
	}

	void UpdateTexture(RenderTexture* Texture, TextureArrayInfo* Info)
	{
		VulkanInterface::LayoutLayerTransitionData TransitionData;
		TransitionData.BaseArrayLayer = Info->BaseArrayLayer;
		TransitionData.LayerCount = Info->LayersCount;

		VulkanInterface::TransitImageLayout(Texture->UniformData.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			&TransitionData, 1);

		VulkanInterface::CopyDataToImage(Texture->UniformData.Image, Info->Width, Info->Height,
			Info->Format, Info->LayersCount, Info->Data, Info->BaseArrayLayer);
	
		TransitImageLayout(Texture->UniformData.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, &TransitionData, 1);
	}

	void DestroyTexture(RenderTexture* Texture)
	{
		VulkanInterface::WaitDevice();
		VulkanInterface::DestroyImageInterface(Texture->ImageView);
		VulkanInterface::DestroyUniformImage(Texture->UniformData);
	}

	void LoadIndices(VulkanInterface::IndexBuffer* IndexBuffer, const u32* Indices, u32 IndicesCount, u64 Offset)
	{
		assert(Indices);
		assert(IndexBuffer);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshIndicesSize);

		VulkanInterface::CopyDataToBuffer(IndexBuffer->Buffer, Offset, MeshIndicesSize, Indices);
		IndexBuffer->Offset += AlignedSize;
	}

	void LoadVertices(VulkanInterface::VertexBuffer* VertexBuffer, const void* Vertices, u32 VertexSize, u64 VerticesCount, u64 Offset)
	{
		assert(VertexBuffer);
		assert(Vertices);

		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshVerticesSize);

		VulkanInterface::CopyDataToBuffer(VertexBuffer->Buffer, Offset, MeshVerticesSize, Vertices);
		VertexBuffer->Offset += AlignedSize;
	}

	void CreatePassSharedResources()
	{
		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = MB64;
		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		PassSharedResources.VertexBuffer = VulkanInterface::CreateUniformBufferInternal(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		PassSharedResources.IndexBuffer = VulkanInterface::CreateUniformBufferInternal(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			PassSharedResources.ShadowMapArray[i] = VulkanInterface::CreateUniformImage(&ShadowMapArrayCreateInfo);
		}

		PassSharedResources.PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PassSharedResources.PushConstants.offset = 0;
		// Todo: check constant and model size?
		PassSharedResources.PushConstants.size = sizeof(BMRModel);
	}

	void CreateDepthPass()
	{
		DepthPass.LightSpaceMatrixLayout = VulkanInterface::CreateUniformLayout(&LightSpaceMatrixDescriptorType, &LightSpaceMatrixStageFlags, 1);

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			const VkDeviceSize LightSpaceMatrixSize = sizeof(Render::BMRLightSpaceMatrix);

			VpBufferInfo.size = LightSpaceMatrixSize;
			DepthPass.LightSpaceMatrixBuffer[i] = VulkanInterface::CreateUniformBufferInternal(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			VulkanInterface::CreateUniformSets(&DepthPass.LightSpaceMatrixLayout, 1, DepthPass.LightSpaceMatrixSet + i);

			DepthPass.ShadowMapElement1ImageInterface[i] = VulkanInterface::CreateImageInterface(&ShadowMapElement1InterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);
			DepthPass.ShadowMapElement2ImageInterface[i] = VulkanInterface::CreateImageInterface(&ShadowMapElement2InterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);

			VulkanInterface::UniformSetAttachmentInfo LightSpaceMatrixAttachmentInfo;
			LightSpaceMatrixAttachmentInfo.BufferInfo.buffer = DepthPass.LightSpaceMatrixBuffer[i].Buffer;
			LightSpaceMatrixAttachmentInfo.BufferInfo.offset = 0;
			LightSpaceMatrixAttachmentInfo.BufferInfo.range = LightSpaceMatrixSize;
			LightSpaceMatrixAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			VulkanInterface::AttachUniformsToSet(DepthPass.LightSpaceMatrixSet[i], &LightSpaceMatrixAttachmentInfo, 1);
		}
	
		DepthPass.Pipeline.PipelineLayout = VulkanInterface::CreatePipelineLayout(&DepthPass.LightSpaceMatrixLayout, 1,
			&PassSharedResources.PushConstants, 1);

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
		ResourceInfo.PipelineLayout = DepthPass.Pipeline.PipelineLayout;
		ResourceInfo.RenderPass = nullptr;
		ResourceInfo.SubpassIndex = 0;

		ResourceInfo.ColorAttachmentCount = 0;
		ResourceInfo.ColorAttachmentFormats = nullptr;
		ResourceInfo.DepthAttachmentFormat = DepthFormat;
		ResourceInfo.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		VulkanInterface::CreatePipelines(&ShaderInfo, &DepthVertexInput, &DepthPipelineSettings, &ResourceInfo, 1, &DepthPass.Pipeline.Pipeline);
		VulkanInterface::DestroyShader(Info.module);
	}

	void CreateMainPass()
	{
		MainPass.DeferredInputLayout = VulkanInterface::CreateUniformLayout(DeferredInputDescriptorType, DeferredInputFlags, 2);
		

		const VkDescriptorType Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags Flags = VK_SHADER_STAGE_FRAGMENT_BIT;
		MainPass.SkyBoxLayout = VulkanInterface::CreateUniformLayout(&Type, &Flags, 1);

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);
			

			

			MainPass.DeferredInputDepthImage[i] = VulkanInterface::CreateUniformImage(&DeferredInputDepthUniformCreateInfo);
			MainPass.DeferredInputColorImage[i] = VulkanInterface::CreateUniformImage(&DeferredInputColorUniformCreateInfo);

			MainPass.DeferredInputDepthImageInterface[i] = VulkanInterface::CreateImageInterface(&DeferredInputDepthUniformInterfaceCreateInfo,
				MainPass.DeferredInputDepthImage[i].Image);
			MainPass.DeferredInputColorImageInterface[i] = VulkanInterface::CreateImageInterface(&DeferredInputUniformColorInterfaceCreateInfo,
				MainPass.DeferredInputColorImage[i].Image);

			VulkanInterface::CreateUniformSets(&MainPass.DeferredInputLayout, 1, MainPass.DeferredInputSet + i);
			

			VulkanInterface::UniformSetAttachmentInfo DeferredInputAttachmentInfo[2];
			DeferredInputAttachmentInfo[0].ImageInfo.imageView = MainPass.DeferredInputColorImageInterface[i];
			DeferredInputAttachmentInfo[0].ImageInfo.sampler = nullptr;
			DeferredInputAttachmentInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[0].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			DeferredInputAttachmentInfo[1].ImageInfo.imageView = MainPass.DeferredInputDepthImageInterface[i];
			DeferredInputAttachmentInfo[1].ImageInfo.sampler = nullptr;
			DeferredInputAttachmentInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[1].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			VulkanInterface::AttachUniformsToSet(MainPass.DeferredInputSet[i], DeferredInputAttachmentInfo, 2);
		}



		VulkanInterface::RenderTarget RenderTarget;
		for (u32 ImageIndex = 0; ImageIndex < VulkanInterface::GetImageCount(); ImageIndex++)
		{
			VulkanInterface::AttachmentView* AttachmentView = RenderTarget.AttachmentViews + ImageIndex;

			AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(MainRenderPassSettings.AttachmentDescriptionsCount);
			AttachmentView->ImageViews[0] = VulkanInterface::GetSwapchainImageViews()[ImageIndex];
			AttachmentView->ImageViews[1] = MainPass.DeferredInputColorImageInterface[ImageIndex];
			AttachmentView->ImageViews[2] = MainPass.DeferredInputDepthImageInterface[ImageIndex];

		}

		VulkanInterface::CreateRenderPass(&MainRenderPassSettings, &RenderTarget, MainScreenExtent, 1, VulkanInterface::GetImageCount(), &MainPass.RenderPass);
	
		const VkDescriptorSetLayout VpLayout = FrameManager::GetViewProjectionLayout();

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] =
		{
			VpLayout,
			MainPass.SkyBoxLayout,
		};

		const u32 SkyBoxIndex = 0;
		const u32 DeferredIndex = 1;

		u32 SetLayoutCountTable[MainPathPipelinesCount];
		SetLayoutCountTable[DeferredIndex] = 1;
		SetLayoutCountTable[SkyBoxIndex] = SkyBoxDescriptorLayoutCount;

		const VkDescriptorSetLayout* SetLayouts[MainPathPipelinesCount];
		SetLayouts[DeferredIndex] = &MainPass.DeferredInputLayout;
		SetLayouts[SkyBoxIndex] = SkyBoxDescriptorLayouts;

		u32 PushConstantRangeCountTable[MainPathPipelinesCount];
		PushConstantRangeCountTable[DeferredIndex] = 0;
		PushConstantRangeCountTable[SkyBoxIndex] = 0;

		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			MainPass.Pipelines[i].PipelineLayout = VulkanInterface::CreatePipelineLayout(SetLayouts[i],
				SetLayoutCountTable[i], &PassSharedResources.PushConstants, PushConstantRangeCountTable[i]);
		}

		const char* Paths[MainPathPipelinesCount][2] = {
			{"./Resources/Shaders/SkyBox_vert.spv", "./Resources/Shaders/SkyBox_frag.spv" },
			{"./Resources/Shaders/second_vert.spv", "./Resources/Shaders/second_frag.spv" },
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
		VertexInput[DeferredIndex] = { };
		VertexInput[SkyBoxIndex] = SkyBoxVertexInput;

		VulkanInterface::PipelineSettings PipelineSettings[MainPathPipelinesCount];
		PipelineSettings[DeferredIndex] = DeferredPipelineSettings;
		PipelineSettings[SkyBoxIndex] = SkyBoxPipelineSettings;

		VulkanInterface::PipelineResourceInfo PipelineResourceInfo[MainPathPipelinesCount];

		PipelineResourceInfo[DeferredIndex].PipelineLayout = MainPass.Pipelines[DeferredIndex].PipelineLayout;
		PipelineResourceInfo[DeferredIndex].RenderPass = MainPass.RenderPass.Pass;
		PipelineResourceInfo[DeferredIndex].SubpassIndex = 1;

		PipelineResourceInfo[SkyBoxIndex].PipelineLayout = MainPass.Pipelines[SkyBoxIndex].PipelineLayout;
		PipelineResourceInfo[SkyBoxIndex].RenderPass = MainPass.RenderPass.Pass;
		PipelineResourceInfo[SkyBoxIndex].SubpassIndex = 0;

		VkPipeline NewPipelines[MainPathPipelinesCount];
		CreatePipelines(ShaderInfo, VertexInput, PipelineSettings, PipelineResourceInfo, MainPathPipelinesCount, NewPipelines);

		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			MainPass.Pipelines[i].Pipeline = NewPipelines[i];

			for (u32 j = 0; j < 2; ++j)
			{
				VulkanInterface::DestroyShader(ShaderInfo[i].Infos[j].module);
			}
		}
	}

	void DestroyPassSharedResources()
	{
		VulkanInterface::DestroyUniformBuffer(PassSharedResources.VertexBuffer);
		VulkanInterface::DestroyUniformBuffer(PassSharedResources.IndexBuffer);

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VulkanInterface::DestroyUniformImage(PassSharedResources.ShadowMapArray[i]);
		}
	}

	void DestroyDepthPass()
	{
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VulkanInterface::DestroyUniformBuffer(DepthPass.LightSpaceMatrixBuffer[i]);

			VulkanInterface::DestroyImageInterface(DepthPass.ShadowMapElement1ImageInterface[i]);
			VulkanInterface::DestroyImageInterface(DepthPass.ShadowMapElement2ImageInterface[i]);
		}

		VulkanInterface::DestroyUniformLayout(DepthPass.LightSpaceMatrixLayout);

		VulkanInterface::DestroyPipelineLayout(DepthPass.Pipeline.PipelineLayout);
		VulkanInterface::DestroyPipeline(DepthPass.Pipeline.Pipeline);

		VulkanInterface::DestroyRenderPass(&DepthPass.RenderPass);
	}

	void DestroyMainPass()
	{
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			VulkanInterface::DestroyUniformImage(MainPass.DeferredInputColorImage[i]);
			VulkanInterface::DestroyUniformImage(MainPass.DeferredInputDepthImage[i]);
			

			VulkanInterface::DestroyImageInterface(MainPass.DeferredInputColorImageInterface[i]);
			VulkanInterface::DestroyImageInterface(MainPass.DeferredInputDepthImageInterface[i]);
		}

		


		VulkanInterface::DestroyUniformLayout(MainPass.DeferredInputLayout);
		VulkanInterface::DestroyUniformLayout(MainPass.SkyBoxLayout);



		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			VulkanInterface::DestroyPipelineLayout(MainPass.Pipelines[i].PipelineLayout);
			VulkanInterface::DestroyPipeline(MainPass.Pipelines[i].Pipeline);
		}

		VulkanInterface::DestroyRenderPass(&MainPass.RenderPass);
	}

	void DepthPassDraw(const DrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex)
	{
		const BMRLightSpaceMatrix* LightViews[] =
		{
			&Scene->LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene->LightEntity->SpotLight.LightSpaceMatrix,
		};

		for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
		{
			VulkanInterface::UpdateUniformBuffer(DepthPass.LightSpaceMatrixBuffer[LightCaster], sizeof(BMRLightSpaceMatrix), 0,
				LightViews[LightCaster]);

			VkRect2D RenderArea;
			RenderArea.extent = DepthViewportExtent;
			RenderArea.offset = { 0, 0 };

			VkRenderingAttachmentInfo DepthAttachment{ };
			DepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			DepthAttachment.imageView = DepthPass.ShadowMapElement1ImageInterface[VulkanInterface::TestGetImageIndex()];
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
			DepthAttachmentTransitionBefore.image = PassSharedResources.ShadowMapArray[VulkanInterface::TestGetImageIndex()].Image;
			DepthAttachmentTransitionBefore.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			DepthAttachmentTransitionBefore.subresourceRange.baseMipLevel = 0;
			DepthAttachmentTransitionBefore.subresourceRange.levelCount = 1;
			DepthAttachmentTransitionBefore.subresourceRange.baseArrayLayer = LightCaster;
			DepthAttachmentTransitionBefore.subresourceRange.layerCount = 1;		

			VkDependencyInfo DependencyInfoBefore = { };
			DependencyInfoBefore.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			DependencyInfoBefore.imageMemoryBarrierCount = 1;
			DependencyInfoBefore.pImageMemoryBarriers = &DepthAttachmentTransitionBefore,

			vkCmdPipelineBarrier2(CommandBuffer, &DependencyInfoBefore);

			vkCmdBeginRendering(CommandBuffer, &RenderingInfo);

			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DepthPass.Pipeline.Pipeline);

			for (u32 i = 0; i < Scene->DrawEntitiesCount; ++i)
			{
				RenderResourceManager::DrawEntity* DrawEntity = Scene->DrawEntities + i;

				const VkBuffer VertexBuffers[] = { PassSharedResources.VertexBuffer.Buffer };
				const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

				const u32 DescriptorSetGroupCount = 1;
				const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
				{
					DepthPass.LightSpaceMatrixSet[LightCaster],
				};

				const VkPipelineLayout PipelineLayout = DepthPass.Pipeline.PipelineLayout;

				vkCmdPushConstants(CommandBuffer, PipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BMRModel), &DrawEntity->Model);

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
			}

			//vkCmdEndRenderPass(CommandBuffer);
			vkCmdEndRendering(CommandBuffer);

			// TODO: move to Main pass?
			VkImageMemoryBarrier2 DepthAttachmentTransitionAfter = { };
			DepthAttachmentTransitionAfter.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			DepthAttachmentTransitionAfter.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			DepthAttachmentTransitionAfter.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			DepthAttachmentTransitionAfter.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			DepthAttachmentTransitionAfter.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			DepthAttachmentTransitionAfter.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			DepthAttachmentTransitionAfter.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthAttachmentTransitionAfter.image = PassSharedResources.ShadowMapArray[VulkanInterface::TestGetImageIndex()].Image;
			DepthAttachmentTransitionAfter.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			DepthAttachmentTransitionAfter.subresourceRange.baseMipLevel = 0;
			DepthAttachmentTransitionAfter.subresourceRange.levelCount = 1;
			DepthAttachmentTransitionAfter.subresourceRange.baseArrayLayer = LightCaster;
			DepthAttachmentTransitionAfter.subresourceRange.layerCount = 1;

			VkDependencyInfo DependencyInfoAfter = { };
			DependencyInfoAfter.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			DependencyInfoAfter.imageMemoryBarrierCount = 1;
			DependencyInfoAfter.pImageMemoryBarriers = &DepthAttachmentTransitionAfter;

			vkCmdPipelineBarrier2(CommandBuffer, &DependencyInfoAfter);
		}
	}

	void MainPassDraw(const DrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex)
	{
		//const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet()[ImageIndex];

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };
		VulkanInterface::BeginRenderPass(&MainPass.RenderPass, RenderArea, 0, ImageIndex);

		for (u32 i = 0; i < OnDrawFunctions.size(); ++i)
		{
			OnDrawFunctions[i]();
		}

		//
		//if (Scene->DrawSkyBox)
		//{
		//	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[2].Pipeline);

		//	const u32 SkyBoxDescriptorSetGroupCount = 2;
		//	const VkDescriptorSet SkyBoxDescriptorSetGroup[SkyBoxDescriptorSetGroupCount] = {
		//		VpSet,
		//		Scene->SkyBox.TextureSet,
		//	};

		//	const VkPipelineLayout PipelineLayout = MainPass.Pipelines[2].PipelineLayout;

		//	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
		//		0, SkyBoxDescriptorSetGroupCount, SkyBoxDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

		//	vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &PassSharedResources.VertexBuffer.Buffer, &Scene->SkyBox.VertexOffset);
		//	vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, Scene->SkyBox.IndexOffset, VK_INDEX_TYPE_UINT32);
		//	vkCmdDrawIndexed(CommandBuffer, Scene->SkyBox.IndicesCount, 1, 0, 0, 0);
		//}
		
		//MainRenderPass::OnDraw();

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();
		vkCmdNextSubpass(CmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
		
		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[1].Pipeline);

		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[1].PipelineLayout,
			0, 1, &MainPass.DeferredInputSet[ImageIndex], 0, nullptr);

		vkCmdDraw(CmdBuffer, 3, 1, 0, 0); // 3 hardcoded vertices for second "post processing" subpass
		vkCmdEndRenderPass(CmdBuffer);
	}

	void Draw(const DrawScene* Scene)
	{
		const u32 ImageIndex = VulkanInterface::AcquireNextImageIndex();

		TestCurrentFrameCommandBuffer = VulkanInterface::BeginDraw(ImageIndex);

		DepthPassDraw(Scene, TestCurrentFrameCommandBuffer, ImageIndex);
		MainPassDraw(Scene, TestCurrentFrameCommandBuffer, ImageIndex);
		
		VulkanInterface::EndDraw(ImageIndex);
	}

	VkImage TmpGetShadowMapArray(int i)
	{
		return PassSharedResources.ShadowMapArray[i].Image;
	}

	VkDeviceSize CalculateBufferAlignedSize(VkDeviceSize BufferSize)
	{
		u32 Padding = 0;
		if (BufferSize % BUFFER_ALIGNMENT != 0)
		{
			Padding = BUFFER_ALIGNMENT - (BufferSize % BUFFER_ALIGNMENT);
		}

		return BufferSize + Padding;
	}

	VkDeviceSize CalculateImageAlignedSize(VkDeviceSize BufferSize)
	{
		u32 Padding = 0;
		if (BufferSize % IMAGE_ALIGNMENT != 0)
		{
			Padding = IMAGE_ALIGNMENT - (BufferSize % IMAGE_ALIGNMENT);
		}

		return BufferSize + Padding;
	}
}
