#include "BMRInterface.h"

#include <Windows.h>

#include <vulkan/vulkan.h>

#include "Memory/MemoryManagmentSystem.h"
#include "VulkanMemoryManagementSystem.h"

#include "VulkanHelper.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "BMRVulkan/BMRVulkan.h"

namespace BMR
{
	struct BMRPassSharedResources
	{
		BMRUniform VertexBuffer;
		u32 VertexBufferOffset = 0;

		BMRUniform IndexBuffer;
		u32 IndexBufferOffset = 0;

		BMR::BMRUniform ShadowMapArray[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkPushConstantRange PushConstants;
	};

	struct BMRDepthPass
	{
		VkDescriptorSetLayout LightSpaceMatrixLayout = nullptr;

		BMR::BMRUniform LightSpaceMatrixBuffer[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSet LightSpaceMatrixSet[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkImageView ShadowMapElement1ImageInterface[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowMapElement2ImageInterface[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRPipeline Pipeline;
		BMRRenderPass RenderPass;
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

		BMR::BMRUniform VpBuffer[MAX_SWAPCHAIN_IMAGES_COUNT];
		BMR::BMRUniform EntityLightBuffer[MAX_SWAPCHAIN_IMAGES_COUNT];
		BMR::BMRUniform MaterialBuffer;
		BMR::BMRUniform DeferredInputDepthImage[MAX_SWAPCHAIN_IMAGES_COUNT];
		BMR::BMRUniform DeferredInputColorImage[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkImageView DeferredInputDepthImageInterface[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DeferredInputColorImageInterface[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowMapArrayImageInterface[MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSet VpSet[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet EntityLightSet[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet DeferredInputSet[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet MaterialSet;
		VkDescriptorSet ShadowMapArraySet[MAX_SWAPCHAIN_IMAGES_COUNT];

		BMRPipeline Pipelines[4];
		BMRRenderPass RenderPass;
	};

	static void CreatePassSharedResources();
	static void CreateDepthPass();
	static void CreateMainPass();

	static void DestroyPassSharedResources();
	static void DestroyDepthPass();
	static void DestroyMainPass();

	static void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection);
	static void UpdateLightBuffer(const BMRLightBuffer* Buffer);
	static void UpdateLightSpaceBuffer(const BMRLightSpaceMatrix* LightSpaceMatrix);

	static void DepthPassDraw(const BMRDrawScene& Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex);
	static void MainPassDraw(const BMRDrawScene& Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex);

	static BMRConfig Config;

	static BMRPassSharedResources PassSharedResources;
	static BMRDepthPass DepthPass;
	static BMRMainPass MainPass;

	static u32 ActiveLightSet = 0;
	static u32 ActiveVpSet = 0;
	static u32 ActiveLightSpaceMatrixSet = 0;

	bool Init(HWND WindowHandler, const BMRConfig& InConfig)
	{
		Config = InConfig;

		SetLogHandler(Config.LogHandler);

		BMRVkConfig BMRVkConfig;
		BMRVkConfig.EnableValidationLayers = Config.EnableValidationLayers;
		BMRVkConfig.LogHandler = (BMRVkLogHandler)Config.LogHandler;
		BMRVkConfig.MaxTextures = Config.MaxTextures;

		BMRVkInit(WindowHandler, BMRVkConfig);
		CreatePassSharedResources();
		CreateDepthPass();
		CreateMainPass();

		return true;
	}

	void DeInit()
	{
		BMR::WaitDevice();
		DestroyDepthPass();
		DestroyMainPass();
		DestroyPassSharedResources();

		BMRVkDeInit();
	}

	void TestAttachEntityTexture(VkImageView DefuseImage, VkImageView SpecularImage, VkDescriptorSet* SetToAttach)
	{
		BMR::CreateUniformSets(&MainPass.EntitySamplerLayout, 1, SetToAttach);

		BMR::BMRUniformSetAttachmentInfo SetInfo[2];
		SetInfo[0].ImageInfo.imageView = DefuseImage;
		SetInfo[0].ImageInfo.sampler = MainPass.DiffuseSampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		SetInfo[1].ImageInfo.imageView = SpecularImage;
		SetInfo[1].ImageInfo.sampler = MainPass.SpecularSampler;
		SetInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[1].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		BMR::AttachUniformsToSet(*SetToAttach, SetInfo, 2);
	}

	void TestAttachSkyNoxTerrainTexture(VkImageView DefuseImage, VkDescriptorSet* SetToAttach)
	{
		BMR::CreateUniformSets(&MainPass.TerrainSkyBoxLayout, 1, SetToAttach);

		BMR::BMRUniformSetAttachmentInfo SetInfo[1];
		SetInfo[0].ImageInfo.imageView = DefuseImage;
		SetInfo[0].ImageInfo.sampler = MainPass.DiffuseSampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		BMR::AttachUniformsToSet(*SetToAttach, SetInfo, 1);
	}

	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount)
	{
		assert(Vertices);

		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		const VkDeviceSize AlignedSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshVerticesSize);

		VulkanMemoryManagementSystem::CopyDataToBuffer(PassSharedResources.VertexBuffer.Buffer, PassSharedResources.VertexBufferOffset, MeshVerticesSize, Vertices);

		const VkDeviceSize CurrentOffset = PassSharedResources.VertexBufferOffset;
		PassSharedResources.VertexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	u64 LoadIndices(const u32* Indices, u32 IndicesCount)
	{
		assert(Indices);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshIndicesSize);

		VulkanMemoryManagementSystem::CopyDataToBuffer(PassSharedResources.IndexBuffer.Buffer, PassSharedResources.IndexBufferOffset, MeshIndicesSize, Indices);

		const VkDeviceSize CurrentOffset = PassSharedResources.IndexBufferOffset;
		PassSharedResources.IndexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	void UpdateLightBuffer(const BMRLightBuffer* Buffer)
	{
		assert(Buffer);

		const u32 UpdateIndex = (ActiveLightSet + 1) % BMR::GetImageCount();

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
		const u32 UpdateIndex = (ActiveLightSpaceMatrixSet + 1) % BMR::GetImageCount();

		UpdateUniformBuffer(DepthPass.LightSpaceMatrixBuffer[UpdateIndex], sizeof(BMRLightSpaceMatrix), 0,
			LightSpaceMatrix);

		ActiveLightSpaceMatrixSet = UpdateIndex;
	}

	void CreatePassSharedResources()
	{
		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = MB64;
		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		PassSharedResources.VertexBuffer = CreateUniformBuffer(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		PassSharedResources.IndexBuffer = CreateUniformBuffer(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
		for (u32 i = 0; i < BMR::GetImageCount(); i++)
		{
			PassSharedResources.ShadowMapArray[i] = BMR::CreateUniformImage(&ShadowMapArrayCreateInfo);
		}

		PassSharedResources.PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PassSharedResources.PushConstants.offset = 0;
		// Todo: check constant and model size?
		PassSharedResources.PushConstants.size = sizeof(BMRModel);
	}

	void CreateDepthPass()
	{
		DepthPass.LightSpaceMatrixLayout = BMR::CreateUniformLayout(&LightSpaceMatrixDescriptorType, &LightSpaceMatrixStageFlags, 1);

		for (u32 i = 0; i < GetImageCount(); i++)
		{
			const VkDeviceSize LightSpaceMatrixSize = sizeof(BMR::BMRLightSpaceMatrix);

			VpBufferInfo.size = LightSpaceMatrixSize;
			DepthPass.LightSpaceMatrixBuffer[i] = BMR::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			BMR::CreateUniformSets(&DepthPass.LightSpaceMatrixLayout, 1, DepthPass.LightSpaceMatrixSet + i);

			DepthPass.ShadowMapElement1ImageInterface[i] = BMR::CreateImageInterface(&ShadowMapElement1InterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);
			DepthPass.ShadowMapElement2ImageInterface[i] = BMR::CreateImageInterface(&ShadowMapElement2InterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);

			BMR::BMRUniformSetAttachmentInfo LightSpaceMatrixAttachmentInfo;
			LightSpaceMatrixAttachmentInfo.BufferInfo.buffer = DepthPass.LightSpaceMatrixBuffer[i].Buffer;
			LightSpaceMatrixAttachmentInfo.BufferInfo.offset = 0;
			LightSpaceMatrixAttachmentInfo.BufferInfo.range = LightSpaceMatrixSize;
			LightSpaceMatrixAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			BMR::AttachUniformsToSet(DepthPass.LightSpaceMatrixSet[i], &LightSpaceMatrixAttachmentInfo, 1);
		}

		BMR::BMRRenderTarget RenderTargets[2];
		for (u32 ImageIndex = 0; ImageIndex < BMR::GetImageCount(); ImageIndex++)
		{
			BMR::BMRAttachmentView* Target1AttachmentView = RenderTargets[0].AttachmentViews + ImageIndex;
			BMR::BMRAttachmentView* Target2AttachmentView = RenderTargets[1].AttachmentViews + ImageIndex;

			Target1AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(DepthRenderPassSettings.AttachmentDescriptionsCount);
			Target2AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(DepthRenderPassSettings.AttachmentDescriptionsCount);

			Target1AttachmentView->ImageViews[0] = DepthPass.ShadowMapElement1ImageInterface[ImageIndex];
			Target2AttachmentView->ImageViews[0] = DepthPass.ShadowMapElement2ImageInterface[ImageIndex];
		}

		BMR::CreateRenderPass(&DepthRenderPassSettings, RenderTargets, DepthViewportExtent, 2, BMR::GetImageCount(), &DepthPass.RenderPass);
	
		DepthPass.Pipeline.PipelineLayout = CreatePipelineLayout(&DepthPass.LightSpaceMatrixLayout, 1,
			&PassSharedResources.PushConstants, 1);

		std::vector<char> ShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/Depth_vert.spv", ShaderCode, "rb");

		VkPipelineShaderStageCreateInfo Info = { };
		Info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		Info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		Info.pName = "main";
		CreateShader((u32*)ShaderCode.data(), ShaderCode.size(), Info.module);

		BMRSPipelineShaderInfo ShaderInfo;
		ShaderInfo.Infos = &Info;
		ShaderInfo.InfosCounter = 1;

		BMRPipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = DepthPass.Pipeline.PipelineLayout;
		ResourceInfo.RenderPass = DepthPass.RenderPass.Pass;
		ResourceInfo.SubpassIndex = 0;

		BMR::CreatePipelines(&ShaderInfo, &DepthVertexInput, &DepthPipelineSettings, &ResourceInfo, 1, &DepthPass.Pipeline.Pipeline);
		BMR::DestroyShader(Info.module);
	}

	void CreateMainPass()
	{
		MainPass.DiffuseSampler = BMR::CreateSampler(&DiffuseSamplerCreateInfo);
		MainPass.SpecularSampler = BMR::CreateSampler(&SpecularSamplerCreateInfo);
		MainPass.ShadowMapArraySampler = BMR::CreateSampler(&ShadowMapSamplerCreateInfo);

		MainPass.VpLayout = BMR::CreateUniformLayout(&VpDescriptorType, &VpStageFlags, 1);
		MainPass.EntityLightLayout = BMR::CreateUniformLayout(&EntityLightDescriptorType, &EntityLightStageFlags, 1);
		MainPass.MaterialLayout = BMR::CreateUniformLayout(&MaterialDescriptorType, &MaterialStageFlags, 1);
		MainPass.DeferredInputLayout = BMR::CreateUniformLayout(DeferredInputDescriptorType, DeferredInputFlags, 2);
		MainPass.ShadowMapArrayLayout = BMR::CreateUniformLayout(&ShadowMapArrayDescriptorType, &ShadowMapArrayFlags, 1);
		MainPass.EntitySamplerLayout = BMR::CreateUniformLayout(EntitySamplerDescriptorType, EntitySamplerInputFlags, 2);
		MainPass.TerrainSkyBoxLayout = BMR::CreateUniformLayout(&TerrainSkyBoxSamplerDescriptorType, &TerrainSkyBoxArrayFlags, 1);
		
		for (u32 i = 0; i < GetImageCount(); i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);
			const VkDeviceSize VpBufferSize = sizeof(BMR::BMRUboViewProjection);
			const VkDeviceSize LightBufferSize = sizeof(BMR::BMRLightBuffer);

			VpBufferInfo.size = VpBufferSize;
			MainPass.VpBuffer[i] = BMR::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VpBufferInfo.size = LightBufferSize;
			MainPass.EntityLightBuffer[i] = BMR::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			MainPass.DeferredInputDepthImage[i] = BMR::CreateUniformImage(&DeferredInputDepthUniformCreateInfo);
			MainPass.DeferredInputColorImage[i] = BMR::CreateUniformImage(&DeferredInputColorUniformCreateInfo);

			MainPass.DeferredInputDepthImageInterface[i] = BMR::CreateImageInterface(&DeferredInputDepthUniformInterfaceCreateInfo,
				MainPass.DeferredInputDepthImage[i].Image);
			MainPass.DeferredInputColorImageInterface[i] = BMR::CreateImageInterface(&DeferredInputUniformColorInterfaceCreateInfo,
				MainPass.DeferredInputColorImage[i].Image);
			MainPass.ShadowMapArrayImageInterface[i] = BMR::CreateImageInterface(&ShadowMapArrayInterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);

			BMR::CreateUniformSets(&MainPass.VpLayout, 1, MainPass.VpSet + i);
			BMR::CreateUniformSets(&MainPass.EntityLightLayout, 1, MainPass.EntityLightSet + i);
			BMR::CreateUniformSets(&MainPass.DeferredInputLayout, 1, MainPass.DeferredInputSet + i);
			BMR::CreateUniformSets(&MainPass.ShadowMapArrayLayout, 1, MainPass.ShadowMapArraySet + i);

			BMR::BMRUniformSetAttachmentInfo VpBufferAttachmentInfo;
			VpBufferAttachmentInfo.BufferInfo.buffer = MainPass.VpBuffer[i].Buffer;
			VpBufferAttachmentInfo.BufferInfo.offset = 0;
			VpBufferAttachmentInfo.BufferInfo.range = VpBufferSize;
			VpBufferAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			BMR::BMRUniformSetAttachmentInfo EntityLightAttachmentInfo;
			EntityLightAttachmentInfo.BufferInfo.buffer = MainPass.EntityLightBuffer[i].Buffer;
			EntityLightAttachmentInfo.BufferInfo.offset = 0;
			EntityLightAttachmentInfo.BufferInfo.range = LightBufferSize;
			EntityLightAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			BMR::BMRUniformSetAttachmentInfo DeferredInputAttachmentInfo[2];
			DeferredInputAttachmentInfo[0].ImageInfo.imageView = MainPass.DeferredInputColorImageInterface[i];
			DeferredInputAttachmentInfo[0].ImageInfo.sampler = nullptr;
			DeferredInputAttachmentInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[0].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			DeferredInputAttachmentInfo[1].ImageInfo.imageView = MainPass.DeferredInputDepthImageInterface[i];
			DeferredInputAttachmentInfo[1].ImageInfo.sampler = nullptr;
			DeferredInputAttachmentInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[1].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			BMR::BMRUniformSetAttachmentInfo ShadowMapArrayAttachmentInfo;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageView = MainPass.ShadowMapArrayImageInterface[i];
			ShadowMapArrayAttachmentInfo.ImageInfo.sampler = MainPass.ShadowMapArraySampler;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapArrayAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			BMR::AttachUniformsToSet(MainPass.VpSet[i], &VpBufferAttachmentInfo, 1);
			BMR::AttachUniformsToSet(MainPass.EntityLightSet[i], &EntityLightAttachmentInfo, 1);
			BMR::AttachUniformsToSet(MainPass.DeferredInputSet[i], DeferredInputAttachmentInfo, 2);
			BMR::AttachUniformsToSet(MainPass.ShadowMapArraySet[i], &ShadowMapArrayAttachmentInfo, 1);
		}

		const VkDeviceSize MaterialSize = sizeof(BMR::BMRMaterial);
		VpBufferInfo.size = MaterialSize;
		MainPass.MaterialBuffer = BMR::CreateUniformBuffer(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		BMR::CreateUniformSets(&MainPass.MaterialLayout, 1, &MainPass.MaterialSet);

		BMR::BMRUniformSetAttachmentInfo MaterialAttachmentInfo;
		MaterialAttachmentInfo.BufferInfo.buffer = MainPass.MaterialBuffer.Buffer;
		MaterialAttachmentInfo.BufferInfo.offset = 0;
		MaterialAttachmentInfo.BufferInfo.range = MaterialSize;
		MaterialAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		BMR::AttachUniformsToSet(MainPass.MaterialSet, &MaterialAttachmentInfo, 1);

		BMR::BMRRenderTarget RenderTarget;
		for (u32 ImageIndex = 0; ImageIndex < BMR::GetImageCount(); ImageIndex++)
		{
			BMR::BMRAttachmentView* AttachmentView = RenderTarget.AttachmentViews + ImageIndex;

			AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(MainRenderPassSettings.AttachmentDescriptionsCount);
			AttachmentView->ImageViews[0] = BMR::GetSwapchainImageViews()[ImageIndex];
			AttachmentView->ImageViews[1] = MainPass.DeferredInputColorImageInterface[ImageIndex];
			AttachmentView->ImageViews[2] = MainPass.DeferredInputDepthImageInterface[ImageIndex];

		}

		BMR::CreateRenderPass(&MainRenderPassSettings, &RenderTarget, MainScreenExtent, 1, BMR::GetImageCount(), &MainPass.RenderPass);
	
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

		const u32 TerrainIndex = 0;
		const u32 EntityIndex = 1;
		const u32 SkyBoxIndex = 2;
		const u32 DeferredIndex = 3;

		u32 SetLayoutCountTable[4];
		SetLayoutCountTable[EntityIndex] = EntityDescriptorLayoutCount;
		SetLayoutCountTable[TerrainIndex] = TerrainDescriptorLayoutsCount;
		SetLayoutCountTable[DeferredIndex] = 1;
		SetLayoutCountTable[SkyBoxIndex] = SkyBoxDescriptorLayoutCount;

		const VkDescriptorSetLayout* SetLayouts[4];
		SetLayouts[EntityIndex] = EntityDescriptorLayouts;
		SetLayouts[TerrainIndex] = TerrainDescriptorLayouts;
		SetLayouts[DeferredIndex] = &MainPass.DeferredInputLayout;
		SetLayouts[SkyBoxIndex] = SkyBoxDescriptorLayouts;



		u32 PushConstantRangeCountTable[4];
		PushConstantRangeCountTable[EntityIndex] = 1;
		PushConstantRangeCountTable[TerrainIndex] = 0;
		PushConstantRangeCountTable[DeferredIndex] = 0;
		PushConstantRangeCountTable[SkyBoxIndex] = 0;

		for (u32 i = 0; i < 4; ++i)
		{
			MainPass.Pipelines[i].PipelineLayout = CreatePipelineLayout(SetLayouts[i],
				SetLayoutCountTable[i], &PassSharedResources.PushConstants, PushConstantRangeCountTable[i]);
		}

		const char* Paths[4][2] = {
			{ "./Resources/Shaders/TerrainGenerator_vert.spv", "./Resources/Shaders/TerrainGenerator_frag.spv" },
			{"./Resources/Shaders/vert.spv", "./Resources/Shaders/frag.spv" },
			{"./Resources/Shaders/SkyBox_vert.spv", "./Resources/Shaders/SkyBox_frag.spv" },
			{"./Resources/Shaders/second_vert.spv", "./Resources/Shaders/second_frag.spv" },
		};

		VkShaderStageFlagBits Stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };		

		BMRSPipelineShaderInfo ShaderInfo[4];
		VkPipelineShaderStageCreateInfo ShaderInfos[4][2];
		for (u32 i = 0; i < 4; ++i)
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
				CreateShader((u32*)ShaderCode.data(), ShaderCode.size(), Info->module);
			}

			ShaderInfo[i].Infos = ShaderInfos[i];
			ShaderInfo[i].InfosCounter = 2;
		}


		BMRVertexInput VertexInput[4];
		VertexInput[EntityIndex] = EntityVertexInput;
		VertexInput[TerrainIndex] = TerrainVertexInput;
		VertexInput[DeferredIndex] = { };
		VertexInput[SkyBoxIndex] = SkyBoxVertexInput;

		BMRPipelineSettings PipelineSettings[4];
		PipelineSettings[EntityIndex] = EntityPipelineSettings;
		PipelineSettings[TerrainIndex] = TerrainPipelineSettings;
		PipelineSettings[DeferredIndex] = DeferredPipelineSettings;
		PipelineSettings[SkyBoxIndex] = SkyBoxPipelineSettings;


		BMRPipelineResourceInfo PipelineResourceInfo[4];
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

		VkPipeline NewPipelines[4];
		CreatePipelines(ShaderInfo, VertexInput, PipelineSettings, PipelineResourceInfo, 4, NewPipelines);

		for (u32 i = 0; i < 4; ++i)
		{
			MainPass.Pipelines[i].Pipeline = NewPipelines[i];
		}
	}

	void DestroyPassSharedResources()
	{
		DestroyUniformBuffer(PassSharedResources.VertexBuffer);
		DestroyUniformBuffer(PassSharedResources.IndexBuffer);

		for (u32 i = 0; i < BMR::GetImageCount(); i++)
		{
			BMR::DestroyUniformImage(PassSharedResources.ShadowMapArray[i]);
		}
	}

	void DestroyDepthPass()
	{
		for (u32 i = 0; i < GetImageCount(); i++)
		{
			BMR::DestroyUniformBuffer(DepthPass.LightSpaceMatrixBuffer[i]);

			BMR::DestroyImageInterface(DepthPass.ShadowMapElement1ImageInterface[i]);
			BMR::DestroyImageInterface(DepthPass.ShadowMapElement2ImageInterface[i]);
		}

		BMR::DestroyUniformLayout(DepthPass.LightSpaceMatrixLayout);

		BMR::DestroyPipelineLayout(DepthPass.Pipeline.PipelineLayout);
		BMR::DestroyPipeline(DepthPass.Pipeline.Pipeline);

		BMR::DestroyRenderPass(&DepthPass.RenderPass);
	}

	void DestroyMainPass()
	{
		for (u32 i = 0; i < BMR::GetImageCount(); i++)
		{
			BMR::DestroyUniformBuffer(MainPass.VpBuffer[i]);
			BMR::DestroyUniformBuffer(MainPass.EntityLightBuffer[i]);
			BMR::DestroyUniformImage(MainPass.DeferredInputColorImage[i]);
			BMR::DestroyUniformImage(MainPass.DeferredInputDepthImage[i]);
			

			BMR::DestroyImageInterface(MainPass.DeferredInputColorImageInterface[i]);
			BMR::DestroyImageInterface(MainPass.DeferredInputDepthImageInterface[i]);
			BMR::DestroyImageInterface(MainPass.ShadowMapArrayImageInterface[i]);
		}

		BMR::DestroyUniformBuffer(MainPass.MaterialBuffer);

		BMR::DestroyUniformLayout(MainPass.VpLayout);
		BMR::DestroyUniformLayout(MainPass.EntityLightLayout);
		BMR::DestroyUniformLayout(MainPass.MaterialLayout);
		BMR::DestroyUniformLayout(MainPass.DeferredInputLayout);
		BMR::DestroyUniformLayout(MainPass.ShadowMapArrayLayout);
		BMR::DestroyUniformLayout(MainPass.EntitySamplerLayout);
		BMR::DestroyUniformLayout(MainPass.TerrainSkyBoxLayout);

		BMR::DestroySampler(MainPass.ShadowMapArraySampler);
		BMR::DestroySampler(MainPass.DiffuseSampler);
		BMR::DestroySampler(MainPass.SpecularSampler);

		for (u32 i = 0; i < 4; ++i)
		{
			BMR::DestroyPipelineLayout(MainPass.Pipelines[i].PipelineLayout);
			BMR::DestroyPipeline(MainPass.Pipelines[i].Pipeline);
		}

		BMR::DestroyRenderPass(&MainPass.RenderPass);
	}

	void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection)
	{
		const u32 UpdateIndex = (ActiveVpSet + 1) % BMR::GetImageCount();

		UpdateUniformBuffer(MainPass.VpBuffer[UpdateIndex], sizeof(BMRUboViewProjection), 0,
			&ViewProjection);

		ActiveVpSet = UpdateIndex;
	}

	void DepthPassDraw(const BMRDrawScene& Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex)
	{
		const BMRLightSpaceMatrix* LightViews[] =
		{
			&Scene.LightEntity->DirectionLight.LightSpaceMatrix,
			&Scene.LightEntity->SpotLight.LightSpaceMatrix,
		};

		for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
		{
			UpdateLightSpaceBuffer(LightViews[LightCaster]);

			VkRect2D RenderArea;
			RenderArea.extent = DepthViewportExtent;
			RenderArea.offset = { 0, 0 };
			BMR::BeginRenderPass(&DepthPass.RenderPass, RenderArea, LightCaster, ImageIndex);

			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DepthPass.Pipeline.Pipeline);

			for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
			{
				BMRDrawEntity* DrawEntity = Scene.DrawEntities + i;

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

	void MainPassDraw(const BMRDrawScene& Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex)
	{
		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };
		BMR::BeginRenderPass(&MainPass.RenderPass, RenderArea, 0, ImageIndex);

		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[0].Pipeline);

		for (u32 i = 0; i < Scene.DrawTerrainEntitiesCount; ++i)
		{
			BMRDrawTerrainEntity* DrawTerrainEntity = Scene.DrawTerrainEntities + i;

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

		// TODO: Support rework to not create identical index buffers
		for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
		{
			BMRDrawEntity* DrawEntity = Scene.DrawEntities + i;

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

			const VkPipelineLayout PipelineLayout = MainPass.Pipelines[1].PipelineLayout;

			vkCmdPushConstants(CommandBuffer, PipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BMRModel), &DrawEntity->Model);

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
				0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

			vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
			vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CommandBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
		}
		
		if (Scene.DrawSkyBox)
		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[2].Pipeline);

			const u32 SkyBoxDescriptorSetGroupCount = 2;
			const VkDescriptorSet SkyBoxDescriptorSetGroup[SkyBoxDescriptorSetGroupCount] = {
				MainPass.VpSet[ActiveVpSet],
				Scene.SkyBox.TextureSet,
			};

			const VkPipelineLayout PipelineLayout = MainPass.Pipelines[2].PipelineLayout;

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
				0, SkyBoxDescriptorSetGroupCount, SkyBoxDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

			vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &PassSharedResources.VertexBuffer.Buffer, &Scene.SkyBox.VertexOffset);
			vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, Scene.SkyBox.IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CommandBuffer, Scene.SkyBox.IndicesCount, 1, 0, 0, 0);
		}
		
		vkCmdNextSubpass(CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[MAX_SWAPCHAIN_IMAGES_COUNT].Pipeline);

		vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[MAX_SWAPCHAIN_IMAGES_COUNT].PipelineLayout,
			0, 1, &MainPass.DeferredInputSet[ImageIndex], 0, nullptr);
		vkCmdDraw(CommandBuffer, 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass
		
		vkCmdEndRenderPass(CommandBuffer);
	}

	void Draw(const BMRDrawScene& Scene)
	{
		// Todo Update only when changed
		UpdateLightBuffer(Scene.LightEntity);
		UpdateVpBuffer(Scene.ViewProjection);

		u32 ImageIndex = BMR::AcquireNextImageIndex();
		VkCommandBuffer CommandBuffer = BMR::BeginDraw(ImageIndex);

		DepthPassDraw(Scene, CommandBuffer, ImageIndex);
		MainPassDraw(Scene, CommandBuffer, ImageIndex);
		
		BMR::EndDraw(ImageIndex);
	}
}
