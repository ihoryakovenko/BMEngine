#include "BMRInterface.h"

#include <vulkan/vulkan.h>

#include "Memory/MemoryManagmentSystem.h"

#include "Util/Settings.h"
#include "Util/Util.h"

namespace BMR
{
	struct BMRPassSharedResources
	{
		BMRVulkan::BMRUniform VertexBuffer;
		u32 VertexBufferOffset = 0;

		BMRVulkan::BMRUniform IndexBuffer;
		u32 IndexBufferOffset = 0;

		BMRVulkan::BMRUniform ShadowMapArray[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkPushConstantRange PushConstants;
	};

	struct BMRDepthPass
	{
		VkDescriptorSetLayout LightSpaceMatrixLayout = nullptr;

		BMRVulkan::BMRUniform LightSpaceMatrixBuffer[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSet LightSpaceMatrixSet[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkImageView ShadowMapElement1ImageInterface[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowMapElement2ImageInterface[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRVulkan::BMRPipeline Pipeline;
		BMRVulkan::BMRRenderPass RenderPass;
	};

	struct BMRMainPass
	{
		VkSampler DiffuseSampler = nullptr;
		VkSampler SpecularSampler = nullptr;
		VkSampler ShadowMapArraySampler = nullptr;

		VkDescriptorSetLayout VpLayout = nullptr;
		VkDescriptorSetLayout EntityLightLayout = nullptr;
		VkDescriptorSetLayout MaterialLayout = nullptr;
		VkDescriptorSetLayout DeferredInputLayout = nullptr;
		VkDescriptorSetLayout ShadowMapArrayLayout = nullptr;
		VkDescriptorSetLayout EntitySamplerLayout = nullptr;
		VkDescriptorSetLayout TerrainSkyBoxLayout = nullptr;
		VkDescriptorSetLayout MapTileSettingsLayout = nullptr;

		BMRVulkan::BMRUniform VpBuffer[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		BMRVulkan::BMRUniform EntityLightBuffer[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		BMRVulkan::BMRUniform MaterialBuffer;
		BMRVulkan::BMRUniform DeferredInputDepthImage[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		BMRVulkan::BMRUniform DeferredInputColorImage[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		BMRVulkan::BMRUniform MapTileSettingsBuffer[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkImageView DeferredInputDepthImageInterface[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DeferredInputColorImageInterface[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowMapArrayImageInterface[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSet VpSet[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet EntityLightSet[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet DeferredInputSet[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet MaterialSet;
		VkDescriptorSet ShadowMapArraySet[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet MapTileSettingsSet[BMRVulkan::MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRVulkan::BMRPipeline Pipelines[MainPathPipelinesCount];
		BMRVulkan::BMRRenderPass RenderPass;
	};

	static VkDeviceSize CalculateBufferAlignedSize(VkDeviceSize BufferSize);
	static VkDeviceSize CalculateImageAlignedSize(VkDeviceSize BufferSize);

	static void CreatePassSharedResources();
	static void CreateDepthPass();
	static void CreateMainPass();

	static void DestroyPassSharedResources();
	static void DestroyDepthPass();
	static void DestroyMainPass();

	static void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection);
	static void UpdateLightBuffer(const BMRLightBuffer* Buffer);
	static void UpdateLightSpaceBuffer(const BMRLightSpaceMatrix* LightSpaceMatrix);
	static void UpdateTileSettingsBuffer(const BMRTileSettings* TileSettings, u32 ImageIndex);

	static void DepthPassDraw(const BMRDrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex);
	static void MainPassDraw(const BMRDrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex);

	static BMRConfig Config;

	static BMRPassSharedResources PassSharedResources;
	static BMRDepthPass DepthPass;
	static BMRMainPass MainPass;

	static u32 ActiveLightSet = 0;
	static u32 ActiveVpSet = 0;
	static u32 ActiveLightSpaceMatrixSet = 0;

	bool Init(HWND WindowHandler, const BMRConfig* InConfig)
	{
		Config = *InConfig;

		BMRVulkan::BMRVkConfig BMRVkConfig;
		BMRVkConfig.EnableValidationLayers = Config.EnableValidationLayers;
		BMRVkConfig.LogHandler = (BMRVulkan::BMRVkLogHandler)Config.LogHandler;
		BMRVkConfig.MaxTextures = Config.MaxTextures;

		Init(WindowHandler, BMRVkConfig);
		CreatePassSharedResources();
		CreateDepthPass();
		CreateMainPass();

		return true;
	}

	void DeInit()
	{
		BMRVulkan::WaitDevice();
		DestroyDepthPass();
		DestroyMainPass();
		DestroyPassSharedResources();

		BMRVulkan::DeInit();
	}

	BMRTexture CreateTexture(BMRTextureArrayInfo* Info)
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

		BMRVulkan::BMRUniformImageInterfaceCreateInfo InterfaceCreateInfo = { };
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

		BMRTexture Texture;
		Texture.UniformData = BMRVulkan::CreateUniformImage(&ImageCreateInfo);
		BMRVulkan::CopyDataToImage(Texture.UniformData.Image, Info->Width, Info->Height, Info->Format, Info->LayersCount, Info->Data);
		Texture.ImageView = BMRVulkan::CreateImageInterface(&InterfaceCreateInfo, Texture.UniformData.Image);

		return Texture;
	}

	void DestroyTexture(BMRTexture* Texture)
	{
		BMRVulkan::WaitDevice();
		BMRVulkan::DestroyImageInterface(Texture->ImageView);
		BMRVulkan::DestroyUniformImage(Texture->UniformData);
	}

	void TestAttachEntityTexture(VkImageView DefuseImage, VkImageView SpecularImage, VkDescriptorSet* SetToAttach)
	{
		BMRVulkan::CreateUniformSets(&MainPass.EntitySamplerLayout, 1, SetToAttach);

		BMRVulkan::BMRUniformSetAttachmentInfo SetInfo[2];
		SetInfo[0].ImageInfo.imageView = DefuseImage;
		SetInfo[0].ImageInfo.sampler = MainPass.DiffuseSampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		SetInfo[1].ImageInfo.imageView = SpecularImage;
		SetInfo[1].ImageInfo.sampler = MainPass.SpecularSampler;
		SetInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[1].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		BMRVulkan::AttachUniformsToSet(*SetToAttach, SetInfo, 2);
	}

	void TestAttachSkyNoxTerrainTexture(VkImageView DefuseImage, VkDescriptorSet* SetToAttach)
	{
		BMRVulkan::CreateUniformSets(&MainPass.TerrainSkyBoxLayout, 1, SetToAttach);

		BMRVulkan::BMRUniformSetAttachmentInfo SetInfo[1];
		SetInfo[0].ImageInfo.imageView = DefuseImage;
		SetInfo[0].ImageInfo.sampler = MainPass.DiffuseSampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		BMRVulkan::AttachUniformsToSet(*SetToAttach, SetInfo, 1);
	}

	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount)
	{
		assert(Vertices);

		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshVerticesSize);

		BMRVulkan::CopyDataToBuffer(PassSharedResources.VertexBuffer.Buffer, PassSharedResources.VertexBufferOffset, MeshVerticesSize, Vertices);

		const VkDeviceSize CurrentOffset = PassSharedResources.VertexBufferOffset;
		PassSharedResources.VertexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	u64 LoadIndices(const u32* Indices, u32 IndicesCount)
	{
		assert(Indices);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshIndicesSize);

		BMRVulkan::CopyDataToBuffer(PassSharedResources.IndexBuffer.Buffer, PassSharedResources.IndexBufferOffset, MeshIndicesSize, Indices);

		const VkDeviceSize CurrentOffset = PassSharedResources.IndexBufferOffset;
		PassSharedResources.IndexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	void ClearIndices()
	{
		PassSharedResources.IndexBufferOffset = 0;
	}

	void UpdateLightBuffer(const BMRLightBuffer* Buffer)
	{
		assert(Buffer);

		const u32 UpdateIndex = (ActiveLightSet + 1) % BMRVulkan::GetImageCount();

		UpdateUniformBuffer(MainPass.EntityLightBuffer[UpdateIndex], sizeof(BMRLightBuffer), 0,
			Buffer);

		ActiveLightSet = UpdateIndex;
	}

	void UpdateMaterialBuffer(const BMRMaterial* Buffer)
	{
		assert(Buffer);
		UpdateUniformBuffer(MainPass.MaterialBuffer, sizeof(BMRMaterial), 0,
			Buffer);
	}

	void UpdateLightSpaceBuffer(const BMRLightSpaceMatrix* LightSpaceMatrix)
	{
		const u32 UpdateIndex = (ActiveLightSpaceMatrixSet + 1) % BMRVulkan::GetImageCount();

		UpdateUniformBuffer(DepthPass.LightSpaceMatrixBuffer[UpdateIndex], sizeof(BMRLightSpaceMatrix), 0,
			LightSpaceMatrix);

		ActiveLightSpaceMatrixSet = UpdateIndex;
	}

	void UpdateTileSettingsBuffer(const BMRTileSettings* TileSettings, u32 ImageIndex)
	{
		UpdateUniformBuffer(MainPass.MapTileSettingsBuffer[ImageIndex], sizeof(BMRTileSettings), 0,
			TileSettings);
	}

	void CreatePassSharedResources()
	{
		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = MB64;
		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		PassSharedResources.VertexBuffer = BMRVulkan::CreateUniformBuffer(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		PassSharedResources.IndexBuffer = BMRVulkan::CreateUniformBuffer(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
		for (u32 i = 0; i < BMRVulkan::GetImageCount(); i++)
		{
			PassSharedResources.ShadowMapArray[i] = BMRVulkan::CreateUniformImage(&ShadowMapArrayCreateInfo);
		}

		PassSharedResources.PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PassSharedResources.PushConstants.offset = 0;
		// Todo: check constant and model size?
		PassSharedResources.PushConstants.size = sizeof(BMRModel);
	}

	void CreateDepthPass()
	{
		DepthPass.LightSpaceMatrixLayout = BMRVulkan::CreateUniformLayout(&LightSpaceMatrixDescriptorType, &LightSpaceMatrixStageFlags, 1);

		for (u32 i = 0; i < BMRVulkan::GetImageCount(); i++)
		{
			const VkDeviceSize LightSpaceMatrixSize = sizeof(BMR::BMRLightSpaceMatrix);

			VpBufferInfo.size = LightSpaceMatrixSize;
			DepthPass.LightSpaceMatrixBuffer[i] = BMRVulkan::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			BMRVulkan::CreateUniformSets(&DepthPass.LightSpaceMatrixLayout, 1, DepthPass.LightSpaceMatrixSet + i);

			DepthPass.ShadowMapElement1ImageInterface[i] = BMRVulkan::CreateImageInterface(&ShadowMapElement1InterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);
			DepthPass.ShadowMapElement2ImageInterface[i] = BMRVulkan::CreateImageInterface(&ShadowMapElement2InterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);

			BMRVulkan::BMRUniformSetAttachmentInfo LightSpaceMatrixAttachmentInfo;
			LightSpaceMatrixAttachmentInfo.BufferInfo.buffer = DepthPass.LightSpaceMatrixBuffer[i].Buffer;
			LightSpaceMatrixAttachmentInfo.BufferInfo.offset = 0;
			LightSpaceMatrixAttachmentInfo.BufferInfo.range = LightSpaceMatrixSize;
			LightSpaceMatrixAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			BMRVulkan::AttachUniformsToSet(DepthPass.LightSpaceMatrixSet[i], &LightSpaceMatrixAttachmentInfo, 1);
		}

		BMRVulkan::BMRRenderTarget RenderTargets[2];
		for (u32 ImageIndex = 0; ImageIndex < BMRVulkan::GetImageCount(); ImageIndex++)
		{
			BMRVulkan::BMRAttachmentView* Target1AttachmentView = RenderTargets[0].AttachmentViews + ImageIndex;
			BMRVulkan::BMRAttachmentView* Target2AttachmentView = RenderTargets[1].AttachmentViews + ImageIndex;

			Target1AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(DepthRenderPassSettings.AttachmentDescriptionsCount);
			Target2AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(DepthRenderPassSettings.AttachmentDescriptionsCount);

			Target1AttachmentView->ImageViews[0] = DepthPass.ShadowMapElement1ImageInterface[ImageIndex];
			Target2AttachmentView->ImageViews[0] = DepthPass.ShadowMapElement2ImageInterface[ImageIndex];
		}

		BMRVulkan::CreateRenderPass(&DepthRenderPassSettings, RenderTargets, DepthViewportExtent, 2, BMRVulkan::GetImageCount(), &DepthPass.RenderPass);
	
		DepthPass.Pipeline.PipelineLayout = BMRVulkan::CreatePipelineLayout(&DepthPass.LightSpaceMatrixLayout, 1,
			&PassSharedResources.PushConstants, 1);

		std::vector<char> ShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/Depth_vert.spv", ShaderCode, "rb");

		VkPipelineShaderStageCreateInfo Info = { };
		Info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		Info.pName = "main";
		BMRVulkan::CreateShader((u32*)ShaderCode.data(), ShaderCode.size(), Info.module);

		BMRVulkan::BMRSPipelineShaderInfo ShaderInfo;
		ShaderInfo.Infos = &Info;
		ShaderInfo.InfosCounter = 1;

		BMRVulkan::BMRPipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = DepthPass.Pipeline.PipelineLayout;
		ResourceInfo.RenderPass = DepthPass.RenderPass.Pass;
		ResourceInfo.SubpassIndex = 0;

		BMRVulkan::CreatePipelines(&ShaderInfo, &DepthVertexInput, &DepthPipelineSettings, &ResourceInfo, 1, &DepthPass.Pipeline.Pipeline);
		BMRVulkan::DestroyShader(Info.module);
	}

	void CreateMainPass()
	{
		MainPass.DiffuseSampler = BMRVulkan::CreateSampler(&DiffuseSamplerCreateInfo);
		MainPass.SpecularSampler = BMRVulkan::CreateSampler(&SpecularSamplerCreateInfo);
		MainPass.ShadowMapArraySampler = BMRVulkan::CreateSampler(&ShadowMapSamplerCreateInfo);

		MainPass.VpLayout = BMRVulkan::CreateUniformLayout(&VpDescriptorType, &VpStageFlags, 1);
		MainPass.EntityLightLayout = BMRVulkan::CreateUniformLayout(&EntityLightDescriptorType, &EntityLightStageFlags, 1);
		MainPass.MaterialLayout = BMRVulkan::CreateUniformLayout(&MaterialDescriptorType, &MaterialStageFlags, 1);
		MainPass.DeferredInputLayout = BMRVulkan::CreateUniformLayout(DeferredInputDescriptorType, DeferredInputFlags, 2);
		MainPass.ShadowMapArrayLayout = BMRVulkan::CreateUniformLayout(&ShadowMapArrayDescriptorType, &ShadowMapArrayFlags, 1);
		MainPass.EntitySamplerLayout = BMRVulkan::CreateUniformLayout(EntitySamplerDescriptorType, EntitySamplerInputFlags, 2);
		MainPass.TerrainSkyBoxLayout = BMRVulkan::CreateUniformLayout(&TerrainSkyBoxSamplerDescriptorType, &TerrainSkyBoxArrayFlags, 1);
		MainPass.MapTileSettingsLayout = BMRVulkan::CreateUniformLayout(&VpDescriptorType, &VpStageFlags, 1);

		for (u32 i = 0; i < BMRVulkan::GetImageCount(); i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);
			const VkDeviceSize VpBufferSize = sizeof(BMR::BMRUboViewProjection);
			const VkDeviceSize LightBufferSize = sizeof(BMR::BMRLightBuffer);
			const VkDeviceSize MapTileSettingsSize = sizeof(BMR::BMRTileSettings);

			VpBufferInfo.size = VpBufferSize;
			MainPass.VpBuffer[i] = BMRVulkan::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VpBufferInfo.size = LightBufferSize;
			MainPass.EntityLightBuffer[i] = BMRVulkan::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VpBufferInfo.size = MapTileSettingsSize;
			MainPass.MapTileSettingsBuffer[i] = BMRVulkan::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			MainPass.DeferredInputDepthImage[i] = BMRVulkan::CreateUniformImage(&DeferredInputDepthUniformCreateInfo);
			MainPass.DeferredInputColorImage[i] = BMRVulkan::CreateUniformImage(&DeferredInputColorUniformCreateInfo);

			MainPass.DeferredInputDepthImageInterface[i] = BMRVulkan::CreateImageInterface(&DeferredInputDepthUniformInterfaceCreateInfo,
				MainPass.DeferredInputDepthImage[i].Image);
			MainPass.DeferredInputColorImageInterface[i] = BMRVulkan::CreateImageInterface(&DeferredInputUniformColorInterfaceCreateInfo,
				MainPass.DeferredInputColorImage[i].Image);
			MainPass.ShadowMapArrayImageInterface[i] = BMRVulkan::CreateImageInterface(&ShadowMapArrayInterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);

			BMRVulkan::CreateUniformSets(&MainPass.VpLayout, 1, MainPass.VpSet + i);
			BMRVulkan::CreateUniformSets(&MainPass.EntityLightLayout, 1, MainPass.EntityLightSet + i);
			BMRVulkan::CreateUniformSets(&MainPass.DeferredInputLayout, 1, MainPass.DeferredInputSet + i);
			BMRVulkan::CreateUniformSets(&MainPass.ShadowMapArrayLayout, 1, MainPass.ShadowMapArraySet + i);
			BMRVulkan::CreateUniformSets(&MainPass.MapTileSettingsLayout, 1, MainPass.MapTileSettingsSet + i);

			BMRVulkan::BMRUniformSetAttachmentInfo VpBufferAttachmentInfo;
			VpBufferAttachmentInfo.BufferInfo.buffer = MainPass.VpBuffer[i].Buffer;
			VpBufferAttachmentInfo.BufferInfo.offset = 0;
			VpBufferAttachmentInfo.BufferInfo.range = VpBufferSize;
			VpBufferAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			BMRVulkan::BMRUniformSetAttachmentInfo EntityLightAttachmentInfo;
			EntityLightAttachmentInfo.BufferInfo.buffer = MainPass.EntityLightBuffer[i].Buffer;
			EntityLightAttachmentInfo.BufferInfo.offset = 0;
			EntityLightAttachmentInfo.BufferInfo.range = LightBufferSize;
			EntityLightAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			BMRVulkan::BMRUniformSetAttachmentInfo DeferredInputAttachmentInfo[2];
			DeferredInputAttachmentInfo[0].ImageInfo.imageView = MainPass.DeferredInputColorImageInterface[i];
			DeferredInputAttachmentInfo[0].ImageInfo.sampler = nullptr;
			DeferredInputAttachmentInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[0].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			DeferredInputAttachmentInfo[1].ImageInfo.imageView = MainPass.DeferredInputDepthImageInterface[i];
			DeferredInputAttachmentInfo[1].ImageInfo.sampler = nullptr;
			DeferredInputAttachmentInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[1].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			BMRVulkan::BMRUniformSetAttachmentInfo ShadowMapArrayAttachmentInfo;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageView = MainPass.ShadowMapArrayImageInterface[i];
			ShadowMapArrayAttachmentInfo.ImageInfo.sampler = MainPass.ShadowMapArraySampler;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapArrayAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			BMRVulkan::BMRUniformSetAttachmentInfo MapTileSettingsAttachmentInfo;
			MapTileSettingsAttachmentInfo.BufferInfo.buffer = MainPass.MapTileSettingsBuffer[i].Buffer;
			MapTileSettingsAttachmentInfo.BufferInfo.offset = 0;
			MapTileSettingsAttachmentInfo.BufferInfo.range = MapTileSettingsSize;
			MapTileSettingsAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			BMRVulkan::AttachUniformsToSet(MainPass.VpSet[i], &VpBufferAttachmentInfo, 1);
			BMRVulkan::AttachUniformsToSet(MainPass.EntityLightSet[i], &EntityLightAttachmentInfo, 1);
			BMRVulkan::AttachUniformsToSet(MainPass.DeferredInputSet[i], DeferredInputAttachmentInfo, 2);
			BMRVulkan::AttachUniformsToSet(MainPass.ShadowMapArraySet[i], &ShadowMapArrayAttachmentInfo, 1);
			BMRVulkan::AttachUniformsToSet(MainPass.MapTileSettingsSet[i], &MapTileSettingsAttachmentInfo, 1);
		}

		const VkDeviceSize MaterialSize = sizeof(BMR::BMRMaterial);
		VpBufferInfo.size = MaterialSize;
		MainPass.MaterialBuffer = BMRVulkan::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		BMRVulkan::CreateUniformSets(&MainPass.MaterialLayout, 1, &MainPass.MaterialSet);

		BMRVulkan::BMRUniformSetAttachmentInfo MaterialAttachmentInfo;
		MaterialAttachmentInfo.BufferInfo.buffer = MainPass.MaterialBuffer.Buffer;
		MaterialAttachmentInfo.BufferInfo.offset = 0;
		MaterialAttachmentInfo.BufferInfo.range = MaterialSize;
		MaterialAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		BMRVulkan::AttachUniformsToSet(MainPass.MaterialSet, &MaterialAttachmentInfo, 1);

		BMRVulkan::BMRRenderTarget RenderTarget;
		for (u32 ImageIndex = 0; ImageIndex < BMRVulkan::GetImageCount(); ImageIndex++)
		{
			BMRVulkan::BMRAttachmentView* AttachmentView = RenderTarget.AttachmentViews + ImageIndex;

			AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(MainRenderPassSettings.AttachmentDescriptionsCount);
			AttachmentView->ImageViews[0] = BMRVulkan::GetSwapchainImageViews()[ImageIndex];
			AttachmentView->ImageViews[1] = MainPass.DeferredInputColorImageInterface[ImageIndex];
			AttachmentView->ImageViews[2] = MainPass.DeferredInputDepthImageInterface[ImageIndex];

		}

		BMRVulkan::CreateRenderPass(&MainRenderPassSettings, &RenderTarget, MainScreenExtent, 1, BMRVulkan::GetImageCount(), &MainPass.RenderPass);
	
		const u32 TerrainDescriptorLayoutsCount = 2;
		VkDescriptorSetLayout TerrainDescriptorLayouts[TerrainDescriptorLayoutsCount] =
		{
			MainPass.VpLayout,
			MainPass.TerrainSkyBoxLayout
		};

		const u32 EntityDescriptorLayoutCount = 5;
		VkDescriptorSetLayout EntityDescriptorLayouts[EntityDescriptorLayoutCount] =
		{
			MainPass.VpLayout,
			MainPass.EntitySamplerLayout,
			MainPass.EntityLightLayout,
			MainPass.MaterialLayout,
			MainPass.ShadowMapArrayLayout
		};

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] =
		{
			MainPass.VpLayout,
			MainPass.TerrainSkyBoxLayout,
		};

		VkDescriptorSetLayout MapDescriptorLayouts[] = 
		{
			MainPass.VpLayout,
			MainPass.TerrainSkyBoxLayout,
			MainPass.MapTileSettingsLayout,
		};
		const u32 MapDescriptorLayoutCount = sizeof(MapDescriptorLayouts) / sizeof(MapDescriptorLayouts[0]);

		const u32 TerrainIndex = 0;
		const u32 EntityIndex = 2;
		const u32 SkyBoxIndex = 3;
		const u32 DeferredIndex = 4;
		const u32 MapIndex = 1;

		u32 SetLayoutCountTable[MainPathPipelinesCount];
		SetLayoutCountTable[EntityIndex] = EntityDescriptorLayoutCount;
		SetLayoutCountTable[TerrainIndex] = TerrainDescriptorLayoutsCount;
		SetLayoutCountTable[DeferredIndex] = 1;
		SetLayoutCountTable[SkyBoxIndex] = SkyBoxDescriptorLayoutCount;
		SetLayoutCountTable[MapIndex] = MapDescriptorLayoutCount;

		const VkDescriptorSetLayout* SetLayouts[MainPathPipelinesCount];
		SetLayouts[EntityIndex] = EntityDescriptorLayouts;
		SetLayouts[TerrainIndex] = TerrainDescriptorLayouts;
		SetLayouts[DeferredIndex] = &MainPass.DeferredInputLayout;
		SetLayouts[SkyBoxIndex] = SkyBoxDescriptorLayouts;
		SetLayouts[MapIndex] = MapDescriptorLayouts;

		u32 PushConstantRangeCountTable[MainPathPipelinesCount];
		PushConstantRangeCountTable[EntityIndex] = 1;
		PushConstantRangeCountTable[TerrainIndex] = 0;
		PushConstantRangeCountTable[DeferredIndex] = 0;
		PushConstantRangeCountTable[SkyBoxIndex] = 0;
		PushConstantRangeCountTable[MapIndex] = 0;

		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			MainPass.Pipelines[i].PipelineLayout = BMRVulkan::CreatePipelineLayout(SetLayouts[i],
				SetLayoutCountTable[i], &PassSharedResources.PushConstants, PushConstantRangeCountTable[i]);
		}

		const char* Paths[MainPathPipelinesCount][2] = {
			{ "./Resources/Shaders/TerrainGenerator_vert.spv", "./Resources/Shaders/TerrainGenerator_frag.spv" },
			{ "./Resources/Shaders/QuadBasedSphere_vert.spv", "./Resources/Shaders/QuadBasedSphere_frag.spv" },
			{"./Resources/Shaders/vert.spv", "./Resources/Shaders/frag.spv" },
			{"./Resources/Shaders/SkyBox_vert.spv", "./Resources/Shaders/SkyBox_frag.spv" },
			{"./Resources/Shaders/second_vert.spv", "./Resources/Shaders/second_frag.spv" },
		};

		VkShaderStageFlagBits Stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };		

		BMRVulkan::BMRSPipelineShaderInfo ShaderInfo[MainPathPipelinesCount];
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
				BMRVulkan::CreateShader((u32*)ShaderCode.data(), ShaderCode.size(), Info->module);
			}

			ShaderInfo[i].Infos = ShaderInfos[i];
			ShaderInfo[i].InfosCounter = 2;
		}


		BMRVulkan::BMRVertexInput VertexInput[MainPathPipelinesCount];
		VertexInput[EntityIndex] = EntityVertexInput;
		VertexInput[TerrainIndex] = TerrainVertexInput;
		VertexInput[DeferredIndex] = { };
		VertexInput[SkyBoxIndex] = SkyBoxVertexInput;
		VertexInput[MapIndex] = { };

		BMRVulkan::BMRPipelineSettings PipelineSettings[MainPathPipelinesCount];
		PipelineSettings[EntityIndex] = EntityPipelineSettings;
		PipelineSettings[TerrainIndex] = TerrainPipelineSettings;
		PipelineSettings[DeferredIndex] = DeferredPipelineSettings;
		PipelineSettings[SkyBoxIndex] = SkyBoxPipelineSettings;
		PipelineSettings[MapIndex] = MapPipelineSettings;

		BMRVulkan::BMRPipelineResourceInfo PipelineResourceInfo[MainPathPipelinesCount];
		PipelineResourceInfo[EntityIndex].PipelineLayout = MainPass.Pipelines[EntityIndex].PipelineLayout;
		PipelineResourceInfo[EntityIndex].RenderPass = MainPass.RenderPass.Pass;
		PipelineResourceInfo[EntityIndex].SubpassIndex = 0;

		PipelineResourceInfo[TerrainIndex].PipelineLayout = MainPass.Pipelines[TerrainIndex].PipelineLayout;
		PipelineResourceInfo[TerrainIndex].RenderPass = MainPass.RenderPass.Pass;
		PipelineResourceInfo[TerrainIndex].SubpassIndex = 0;

		PipelineResourceInfo[DeferredIndex].PipelineLayout = MainPass.Pipelines[DeferredIndex].PipelineLayout;
		PipelineResourceInfo[DeferredIndex].RenderPass = MainPass.RenderPass.Pass;
		PipelineResourceInfo[DeferredIndex].SubpassIndex = 1;

		PipelineResourceInfo[SkyBoxIndex].PipelineLayout = MainPass.Pipelines[SkyBoxIndex].PipelineLayout;
		PipelineResourceInfo[SkyBoxIndex].RenderPass = MainPass.RenderPass.Pass;
		PipelineResourceInfo[SkyBoxIndex].SubpassIndex = 0;

		PipelineResourceInfo[MapIndex].PipelineLayout = MainPass.Pipelines[MapIndex].PipelineLayout;
		PipelineResourceInfo[MapIndex].RenderPass = MainPass.RenderPass.Pass;
		PipelineResourceInfo[MapIndex].SubpassIndex = 0;

		VkPipeline NewPipelines[MainPathPipelinesCount];
		CreatePipelines(ShaderInfo, VertexInput, PipelineSettings, PipelineResourceInfo, MainPathPipelinesCount, NewPipelines);

		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			MainPass.Pipelines[i].Pipeline = NewPipelines[i];

			for (u32 j = 0; j < 2; ++j)
			{
				BMRVulkan::DestroyShader(ShaderInfo[i].Infos[j].module);
			}
		}
	}

	void DestroyPassSharedResources()
	{
		BMRVulkan::DestroyUniformBuffer(PassSharedResources.VertexBuffer);
		BMRVulkan::DestroyUniformBuffer(PassSharedResources.IndexBuffer);

		for (u32 i = 0; i < BMRVulkan::GetImageCount(); i++)
		{
			BMRVulkan::DestroyUniformImage(PassSharedResources.ShadowMapArray[i]);
		}
	}

	void DestroyDepthPass()
	{
		for (u32 i = 0; i < BMRVulkan::GetImageCount(); i++)
		{
			BMRVulkan::DestroyUniformBuffer(DepthPass.LightSpaceMatrixBuffer[i]);

			BMRVulkan::DestroyImageInterface(DepthPass.ShadowMapElement1ImageInterface[i]);
			BMRVulkan::DestroyImageInterface(DepthPass.ShadowMapElement2ImageInterface[i]);
		}

		BMRVulkan::DestroyUniformLayout(DepthPass.LightSpaceMatrixLayout);

		BMRVulkan::DestroyPipelineLayout(DepthPass.Pipeline.PipelineLayout);
		BMRVulkan::DestroyPipeline(DepthPass.Pipeline.Pipeline);

		BMRVulkan::DestroyRenderPass(&DepthPass.RenderPass);
	}

	void DestroyMainPass()
	{
		for (u32 i = 0; i < BMRVulkan::GetImageCount(); i++)
		{
			BMRVulkan::DestroyUniformBuffer(MainPass.VpBuffer[i]);
			BMRVulkan::DestroyUniformBuffer(MainPass.EntityLightBuffer[i]);
			BMRVulkan::DestroyUniformBuffer(MainPass.MapTileSettingsBuffer[i]);
			BMRVulkan::DestroyUniformImage(MainPass.DeferredInputColorImage[i]);
			BMRVulkan::DestroyUniformImage(MainPass.DeferredInputDepthImage[i]);
			

			BMRVulkan::DestroyImageInterface(MainPass.DeferredInputColorImageInterface[i]);
			BMRVulkan::DestroyImageInterface(MainPass.DeferredInputDepthImageInterface[i]);
			BMRVulkan::DestroyImageInterface(MainPass.ShadowMapArrayImageInterface[i]);
		}

		BMRVulkan::DestroyUniformBuffer(MainPass.MaterialBuffer);

		BMRVulkan::DestroyUniformLayout(MainPass.VpLayout);
		BMRVulkan::DestroyUniformLayout(MainPass.EntityLightLayout);
		BMRVulkan::DestroyUniformLayout(MainPass.MaterialLayout);
		BMRVulkan::DestroyUniformLayout(MainPass.DeferredInputLayout);
		BMRVulkan::DestroyUniformLayout(MainPass.ShadowMapArrayLayout);
		BMRVulkan::DestroyUniformLayout(MainPass.EntitySamplerLayout);
		BMRVulkan::DestroyUniformLayout(MainPass.TerrainSkyBoxLayout);
		BMRVulkan::DestroyUniformLayout(MainPass.MapTileSettingsLayout);

		BMRVulkan::DestroySampler(MainPass.ShadowMapArraySampler);
		BMRVulkan::DestroySampler(MainPass.DiffuseSampler);
		BMRVulkan::DestroySampler(MainPass.SpecularSampler);

		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			BMRVulkan::DestroyPipelineLayout(MainPass.Pipelines[i].PipelineLayout);
			BMRVulkan::DestroyPipeline(MainPass.Pipelines[i].Pipeline);
		}

		BMRVulkan::DestroyRenderPass(&MainPass.RenderPass);
	}

	void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection)
	{
		const u32 UpdateIndex = (ActiveVpSet + 1) % BMRVulkan::GetImageCount();

		UpdateUniformBuffer(MainPass.VpBuffer[UpdateIndex], sizeof(BMRUboViewProjection), 0,
			&ViewProjection);

		ActiveVpSet = UpdateIndex;
	}

	void DepthPassDraw(const BMRDrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex)
	{
		const BMRLightSpaceMatrix* LightViews[] =
		{
			&Scene->LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene->LightEntity->SpotLight.LightSpaceMatrix,
		};

		for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
		{
			UpdateLightSpaceBuffer(LightViews[LightCaster]);

			VkRect2D RenderArea;
			RenderArea.extent = DepthViewportExtent;
			RenderArea.offset = { 0, 0 };
			BMRVulkan::BeginRenderPass(&DepthPass.RenderPass, RenderArea, LightCaster, ImageIndex);

			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DepthPass.Pipeline.Pipeline);

			for (u32 i = 0; i < Scene->DrawEntitiesCount; ++i)
			{
				BMRDrawEntity* DrawEntity = Scene->DrawEntities + i;

				const VkBuffer VertexBuffers[] = { PassSharedResources.VertexBuffer.Buffer };
				const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

				const u32 DescriptorSetGroupCount = 1;
				const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
				{
					DepthPass.LightSpaceMatrixSet[ActiveLightSpaceMatrixSet],
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

			vkCmdEndRenderPass(CommandBuffer);
		}
	}

	void MainPassDraw(const BMRDrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex)
	{
		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };
		BMRVulkan::BeginRenderPass(&MainPass.RenderPass, RenderArea, 0, ImageIndex);

		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[0].Pipeline);

		for (u32 i = 0; i < Scene->DrawTerrainEntitiesCount; ++i)
		{
			BMRDrawTerrainEntity* DrawTerrainEntity = Scene->DrawTerrainEntities + i;

			const VkBuffer TerrainVertexBuffers[] = { PassSharedResources.VertexBuffer.Buffer };
			const VkDeviceSize TerrainBuffersOffsets[] = { DrawTerrainEntity->VertexOffset };

			const u32 TerrainDescriptorSetGroupCount = 2;
			const VkDescriptorSet TerrainDescriptorSetGroup[TerrainDescriptorSetGroupCount] = {
				MainPass.VpSet[ActiveVpSet],
				DrawTerrainEntity->TextureSet
			};

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[0].PipelineLayout,
				0, TerrainDescriptorSetGroupCount, TerrainDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

			vkCmdBindVertexBuffers(CommandBuffer, 0, 1, TerrainVertexBuffers, TerrainBuffersOffsets);
			vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, DrawTerrainEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CommandBuffer, DrawTerrainEntity->IndicesCount, 1, 0, 0, 0);
		}

		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[1].Pipeline);

		const u32 TerrainDescriptorSetGroupCount = 3;
		const VkDescriptorSet TerrainDescriptorSetGroup[TerrainDescriptorSetGroupCount] = {
			MainPass.VpSet[ActiveVpSet],
			Scene->MapEntity.TextureSet,
			MainPass.MapTileSettingsSet[ImageIndex]
		};
		
		vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[1].PipelineLayout,
			0, TerrainDescriptorSetGroupCount, TerrainDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

		vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, Scene->MapEntity.IndexOffset, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(CommandBuffer, Scene->MapEntity.IndicesCount, 1, 0, 0, 0);
			
		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[2].Pipeline);

		// TODO: Support rework to not create identical index buffers
		for (u32 i = 0; i < Scene->DrawEntitiesCount; ++i)
		{
			BMRDrawEntity* DrawEntity = Scene->DrawEntities + i;

			const VkBuffer VertexBuffers[] = { PassSharedResources.VertexBuffer.Buffer };
			const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

			const VkDescriptorSet DescriptorSetGroup[] =
			{
				MainPass.VpSet[ActiveVpSet],
				DrawEntity->TextureSet,
				MainPass.EntityLightSet[ActiveLightSet],
				MainPass.MaterialSet,
				MainPass.ShadowMapArraySet[ImageIndex],
			};
			const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

			const VkPipelineLayout PipelineLayout = MainPass.Pipelines[2].PipelineLayout;

			vkCmdPushConstants(CommandBuffer, PipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BMRModel), &DrawEntity->Model);

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
				0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

			vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
			vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CommandBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
		}
		
		if (Scene->DrawSkyBox)
		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[3].Pipeline);

			const u32 SkyBoxDescriptorSetGroupCount = 2;
			const VkDescriptorSet SkyBoxDescriptorSetGroup[SkyBoxDescriptorSetGroupCount] = {
				MainPass.VpSet[ActiveVpSet],
				Scene->SkyBox.TextureSet,
			};

			const VkPipelineLayout PipelineLayout = MainPass.Pipelines[3].PipelineLayout;

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
				0, SkyBoxDescriptorSetGroupCount, SkyBoxDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

			vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &PassSharedResources.VertexBuffer.Buffer, &Scene->SkyBox.VertexOffset);
			vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, Scene->SkyBox.IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CommandBuffer, Scene->SkyBox.IndicesCount, 1, 0, 0, 0);
		}
		
		vkCmdNextSubpass(CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[4].Pipeline);

		vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[4].PipelineLayout,
			0, 1, &MainPass.DeferredInputSet[ImageIndex], 0, nullptr);
		vkCmdDraw(CommandBuffer, 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass
		
		vkCmdEndRenderPass(CommandBuffer);
	}

	void Draw(const BMRDrawScene* Scene)
	{
		UpdateLightBuffer(Scene->LightEntity);
		UpdateVpBuffer(Scene->ViewProjection);
		

		u32 ImageIndex = BMRVulkan::AcquireNextImageIndex();
		UpdateTileSettingsBuffer(&Scene->MapTileSettings, ImageIndex);

		VkCommandBuffer CommandBuffer = BMRVulkan::BeginDraw(ImageIndex);

		DepthPassDraw(Scene, CommandBuffer, ImageIndex);
		MainPassDraw(Scene, CommandBuffer, ImageIndex);
		
		BMRVulkan::EndDraw(ImageIndex);
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
