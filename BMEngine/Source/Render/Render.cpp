#include "Render.h"

#include <vulkan/vulkan.h>

#include "Memory/MemoryManagmentSystem.h"

#include "Util/Settings.h"
#include "Util/Util.h"

#include "Engine/Systems/DynamicMapSystem.h"
#include "FrameManager.h"

namespace Render
{
	struct BMRPassSharedResources
	{
		VulkanInterface::UniformBuffer VertexBuffer;
		u32 VertexBufferOffset = 0;

		VulkanInterface::UniformBuffer IndexBuffer;
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
		VkSampler DiffuseSampler = nullptr;
		VkSampler SpecularSampler = nullptr;
		VkSampler ShadowMapArraySampler = nullptr;

		VkDescriptorSetLayout EntityLightLayout = nullptr;
		VkDescriptorSetLayout MaterialLayout = nullptr;
		VkDescriptorSetLayout DeferredInputLayout = nullptr;
		VkDescriptorSetLayout ShadowMapArrayLayout = nullptr;
		VkDescriptorSetLayout EntitySamplerLayout = nullptr;
		VkDescriptorSetLayout TerrainSkyBoxLayout = nullptr;
		

		VulkanInterface::UniformBuffer EntityLightBuffer[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VulkanInterface::UniformBuffer MaterialBuffer;
		VulkanInterface::UniformImage DeferredInputDepthImage[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VulkanInterface::UniformImage DeferredInputColorImage[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkImageView DeferredInputDepthImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView DeferredInputColorImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImageView ShadowMapArrayImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSet EntityLightSet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet DeferredInputSet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSet MaterialSet;
		VkDescriptorSet ShadowMapArraySet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

		VulkanInterface::RenderPipeline Pipelines[MainPathPipelinesCount];
		VulkanInterface::RenderPass RenderPass;
	};

	static VkDeviceSize CalculateBufferAlignedSize(VkDeviceSize BufferSize);
	static VkDeviceSize CalculateImageAlignedSize(VkDeviceSize BufferSize);

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

	VkDescriptorSetLayout TestGetTerrainSkyBoxLayout()
	{
		return MainPass.TerrainSkyBoxLayout;
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

	void BindPipeline(VkPipeline RenderPipeline)
	{
		vkCmdBindPipeline(TestCurrentFrameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, RenderPipeline);
	}

	void BindDescriptorSet(const VkDescriptorSet* Sets, u32 DescriptorsCount,
		VkPipelineLayout PipelineLayout, u32 FirstSet, const u32* DynamicOffset, u32 DynamicOffsetsCount)
	{
		vkCmdBindDescriptorSets(TestCurrentFrameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
			FirstSet, DescriptorsCount, Sets, DynamicOffsetsCount, DynamicOffset);
	}

	void BindIndexBuffer(VulkanInterface::IndexBuffer IndexBuffer, u32 Offset)
	{
		vkCmdBindIndexBuffer(TestCurrentFrameCommandBuffer, IndexBuffer.Buffer, Offset, VK_INDEX_TYPE_UINT32);
	}

	void DrawIndexed(u32 IndexCount)
	{
		vkCmdDrawIndexed(TestCurrentFrameCommandBuffer, IndexCount, 1, 0, 0, 0);
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

	void TestAttachEntityTexture(VkImageView DefuseImage, VkImageView SpecularImage, VkDescriptorSet* SetToAttach)
	{
		VulkanInterface::CreateUniformSets(&MainPass.EntitySamplerLayout, 1, SetToAttach);

		VulkanInterface::UniformSetAttachmentInfo SetInfo[2];
		SetInfo[0].ImageInfo.imageView = DefuseImage;
		SetInfo[0].ImageInfo.sampler = MainPass.DiffuseSampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		SetInfo[1].ImageInfo.imageView = SpecularImage;
		SetInfo[1].ImageInfo.sampler = MainPass.SpecularSampler;
		SetInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[1].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VulkanInterface::AttachUniformsToSet(*SetToAttach, SetInfo, 2);
	}

	void TestAttachSkyNoxTerrainTexture(VkImageView DefuseImage, VkDescriptorSet* SetToAttach)
	{
		VulkanInterface::CreateUniformSets(&MainPass.TerrainSkyBoxLayout, 1, SetToAttach);

		VulkanInterface::UniformSetAttachmentInfo SetInfo[1];
		SetInfo[0].ImageInfo.imageView = DefuseImage;
		SetInfo[0].ImageInfo.sampler = MainPass.DiffuseSampler;
		SetInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SetInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		VulkanInterface::AttachUniformsToSet(*SetToAttach, SetInfo, 1);
	}

	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount)
	{
		assert(Vertices);

		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshVerticesSize);

		VulkanInterface::CopyDataToBuffer(PassSharedResources.VertexBuffer.Buffer, PassSharedResources.VertexBufferOffset, MeshVerticesSize, Vertices);

		const VkDeviceSize CurrentOffset = PassSharedResources.VertexBufferOffset;
		PassSharedResources.VertexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	u64 LoadIndices(const u32* Indices, u32 IndicesCount)
	{
		assert(Indices);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshIndicesSize);

		VulkanInterface::CopyDataToBuffer(PassSharedResources.IndexBuffer.Buffer, PassSharedResources.IndexBufferOffset, MeshIndicesSize, Indices);

		const VkDeviceSize CurrentOffset = PassSharedResources.IndexBufferOffset;
		PassSharedResources.IndexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	void LoadIndices(VulkanInterface::IndexBuffer IndexBuffer, const u32* Indices, u32 IndicesCount, u64 Offset)
	{
		assert(Indices);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = CalculateBufferAlignedSize(MeshIndicesSize);

		VulkanInterface::CopyDataToBuffer(IndexBuffer.Buffer, Offset, MeshIndicesSize, Indices);
	}

	void ClearIndices()
	{
		PassSharedResources.IndexBufferOffset = 0;
	}

	void UpdateMaterialBuffer(const Material* Buffer)
	{
		assert(Buffer);
		UpdateUniformBuffer(MainPass.MaterialBuffer, sizeof(Material), 0,
			Buffer);
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

		VulkanInterface::RenderTarget RenderTargets[2];
		for (u32 ImageIndex = 0; ImageIndex < VulkanInterface::GetImageCount(); ImageIndex++)
		{
			VulkanInterface::AttachmentView* Target1AttachmentView = RenderTargets[0].AttachmentViews + ImageIndex;
			VulkanInterface::AttachmentView* Target2AttachmentView = RenderTargets[1].AttachmentViews + ImageIndex;

			Target1AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(DepthRenderPassSettings.AttachmentDescriptionsCount);
			Target2AttachmentView->ImageViews = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageView>(DepthRenderPassSettings.AttachmentDescriptionsCount);

			Target1AttachmentView->ImageViews[0] = DepthPass.ShadowMapElement1ImageInterface[ImageIndex];
			Target2AttachmentView->ImageViews[0] = DepthPass.ShadowMapElement2ImageInterface[ImageIndex];
		}

		VulkanInterface::CreateRenderPass(&DepthRenderPassSettings, RenderTargets, DepthViewportExtent, 2, VulkanInterface::GetImageCount(), &DepthPass.RenderPass);
	
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
		ResourceInfo.RenderPass = DepthPass.RenderPass.Pass;
		ResourceInfo.SubpassIndex = 0;

		VulkanInterface::CreatePipelines(&ShaderInfo, &DepthVertexInput, &DepthPipelineSettings, &ResourceInfo, 1, &DepthPass.Pipeline.Pipeline);
		VulkanInterface::DestroyShader(Info.module);
	}

	void CreateMainPass()
	{
		MainPass.DiffuseSampler = VulkanInterface::CreateSampler(&DiffuseSamplerCreateInfo);
		MainPass.SpecularSampler = VulkanInterface::CreateSampler(&SpecularSamplerCreateInfo);
		MainPass.ShadowMapArraySampler = VulkanInterface::CreateSampler(&ShadowMapSamplerCreateInfo);

		MainPass.EntityLightLayout = VulkanInterface::CreateUniformLayout(&EntityLightDescriptorType, &EntityLightStageFlags, 1);
		MainPass.MaterialLayout = VulkanInterface::CreateUniformLayout(&MaterialDescriptorType, &MaterialStageFlags, 1);
		MainPass.DeferredInputLayout = VulkanInterface::CreateUniformLayout(DeferredInputDescriptorType, DeferredInputFlags, 2);
		MainPass.ShadowMapArrayLayout = VulkanInterface::CreateUniformLayout(&ShadowMapArrayDescriptorType, &ShadowMapArrayFlags, 1);
		MainPass.EntitySamplerLayout = VulkanInterface::CreateUniformLayout(EntitySamplerDescriptorType, EntitySamplerInputFlags, 2);
		MainPass.TerrainSkyBoxLayout = VulkanInterface::CreateUniformLayout(&TerrainSkyBoxSamplerDescriptorType, &TerrainSkyBoxArrayFlags, 1);

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);
			
			const VkDeviceSize LightBufferSize = sizeof(Render::LightBuffer);
			
			VpBufferInfo.size = LightBufferSize;
			MainPass.EntityLightBuffer[i] = VulkanInterface::CreateUniformBufferInternal(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			MainPass.DeferredInputDepthImage[i] = VulkanInterface::CreateUniformImage(&DeferredInputDepthUniformCreateInfo);
			MainPass.DeferredInputColorImage[i] = VulkanInterface::CreateUniformImage(&DeferredInputColorUniformCreateInfo);

			MainPass.DeferredInputDepthImageInterface[i] = VulkanInterface::CreateImageInterface(&DeferredInputDepthUniformInterfaceCreateInfo,
				MainPass.DeferredInputDepthImage[i].Image);
			MainPass.DeferredInputColorImageInterface[i] = VulkanInterface::CreateImageInterface(&DeferredInputUniformColorInterfaceCreateInfo,
				MainPass.DeferredInputColorImage[i].Image);
			MainPass.ShadowMapArrayImageInterface[i] = VulkanInterface::CreateImageInterface(&ShadowMapArrayInterfaceCreateInfo,
				PassSharedResources.ShadowMapArray[i].Image);

			VulkanInterface::CreateUniformSets(&MainPass.EntityLightLayout, 1, MainPass.EntityLightSet + i);
			VulkanInterface::CreateUniformSets(&MainPass.DeferredInputLayout, 1, MainPass.DeferredInputSet + i);
			VulkanInterface::CreateUniformSets(&MainPass.ShadowMapArrayLayout, 1, MainPass.ShadowMapArraySet + i);

			VulkanInterface::UniformSetAttachmentInfo EntityLightAttachmentInfo;
			EntityLightAttachmentInfo.BufferInfo.buffer = MainPass.EntityLightBuffer[i].Buffer;
			EntityLightAttachmentInfo.BufferInfo.offset = 0;
			EntityLightAttachmentInfo.BufferInfo.range = LightBufferSize;
			EntityLightAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			VulkanInterface::UniformSetAttachmentInfo DeferredInputAttachmentInfo[2];
			DeferredInputAttachmentInfo[0].ImageInfo.imageView = MainPass.DeferredInputColorImageInterface[i];
			DeferredInputAttachmentInfo[0].ImageInfo.sampler = nullptr;
			DeferredInputAttachmentInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[0].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			DeferredInputAttachmentInfo[1].ImageInfo.imageView = MainPass.DeferredInputDepthImageInterface[i];
			DeferredInputAttachmentInfo[1].ImageInfo.sampler = nullptr;
			DeferredInputAttachmentInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[1].Type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

			VulkanInterface::UniformSetAttachmentInfo ShadowMapArrayAttachmentInfo;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageView = MainPass.ShadowMapArrayImageInterface[i];
			ShadowMapArrayAttachmentInfo.ImageInfo.sampler = MainPass.ShadowMapArraySampler;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapArrayAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			VulkanInterface::AttachUniformsToSet(MainPass.EntityLightSet[i], &EntityLightAttachmentInfo, 1);
			VulkanInterface::AttachUniformsToSet(MainPass.DeferredInputSet[i], DeferredInputAttachmentInfo, 2);
			VulkanInterface::AttachUniformsToSet(MainPass.ShadowMapArraySet[i], &ShadowMapArrayAttachmentInfo, 1);
		}

		const VkDeviceSize MaterialSize = sizeof(Render::Material);
		VpBufferInfo.size = MaterialSize;
		MainPass.MaterialBuffer = VulkanInterface::CreateUniformBufferInternal(&VpBufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VulkanInterface::CreateUniformSets(&MainPass.MaterialLayout, 1, &MainPass.MaterialSet);

		VulkanInterface::UniformSetAttachmentInfo MaterialAttachmentInfo;
		MaterialAttachmentInfo.BufferInfo.buffer = MainPass.MaterialBuffer.Buffer;
		MaterialAttachmentInfo.BufferInfo.offset = 0;
		MaterialAttachmentInfo.BufferInfo.range = MaterialSize;
		MaterialAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VulkanInterface::AttachUniformsToSet(MainPass.MaterialSet, &MaterialAttachmentInfo, 1);

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

		const u32 TerrainDescriptorLayoutsCount = 2;
		VkDescriptorSetLayout TerrainDescriptorLayouts[TerrainDescriptorLayoutsCount] =
		{
			VpLayout,
			MainPass.TerrainSkyBoxLayout
		};

		const u32 EntityDescriptorLayoutCount = 5;
		VkDescriptorSetLayout EntityDescriptorLayouts[EntityDescriptorLayoutCount] =
		{
			VpLayout,
			MainPass.EntitySamplerLayout,
			MainPass.EntityLightLayout,
			MainPass.MaterialLayout,
			MainPass.ShadowMapArrayLayout
		};

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] =
		{
			VpLayout,
			MainPass.TerrainSkyBoxLayout,
		};

		const u32 TerrainIndex = 0;
		const u32 EntityIndex = 1;
		const u32 SkyBoxIndex = 2;
		const u32 DeferredIndex = 3;

		u32 SetLayoutCountTable[MainPathPipelinesCount];
		SetLayoutCountTable[EntityIndex] = EntityDescriptorLayoutCount;
		SetLayoutCountTable[TerrainIndex] = TerrainDescriptorLayoutsCount;
		SetLayoutCountTable[DeferredIndex] = 1;
		SetLayoutCountTable[SkyBoxIndex] = SkyBoxDescriptorLayoutCount;

		const VkDescriptorSetLayout* SetLayouts[MainPathPipelinesCount];
		SetLayouts[EntityIndex] = EntityDescriptorLayouts;
		SetLayouts[TerrainIndex] = TerrainDescriptorLayouts;
		SetLayouts[DeferredIndex] = &MainPass.DeferredInputLayout;
		SetLayouts[SkyBoxIndex] = SkyBoxDescriptorLayouts;

		u32 PushConstantRangeCountTable[MainPathPipelinesCount];
		PushConstantRangeCountTable[EntityIndex] = 1;
		PushConstantRangeCountTable[TerrainIndex] = 0;
		PushConstantRangeCountTable[DeferredIndex] = 0;
		PushConstantRangeCountTable[SkyBoxIndex] = 0;

		for (u32 i = 0; i < MainPathPipelinesCount; ++i)
		{
			MainPass.Pipelines[i].PipelineLayout = VulkanInterface::CreatePipelineLayout(SetLayouts[i],
				SetLayoutCountTable[i], &PassSharedResources.PushConstants, PushConstantRangeCountTable[i]);
		}

		const char* Paths[MainPathPipelinesCount][2] = {
			{ "./Resources/Shaders/TerrainGenerator_vert.spv", "./Resources/Shaders/TerrainGenerator_frag.spv" },
			{"./Resources/Shaders/vert.spv", "./Resources/Shaders/frag.spv" },
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
		VertexInput[EntityIndex] = EntityVertexInput;
		VertexInput[TerrainIndex] = TerrainVertexInput;
		VertexInput[DeferredIndex] = { };
		VertexInput[SkyBoxIndex] = SkyBoxVertexInput;

		VulkanInterface::PipelineSettings PipelineSettings[MainPathPipelinesCount];
		PipelineSettings[EntityIndex] = EntityPipelineSettings;
		PipelineSettings[TerrainIndex] = TerrainPipelineSettings;
		PipelineSettings[DeferredIndex] = DeferredPipelineSettings;
		PipelineSettings[SkyBoxIndex] = SkyBoxPipelineSettings;

		VulkanInterface::PipelineResourceInfo PipelineResourceInfo[MainPathPipelinesCount];
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
			VulkanInterface::DestroyUniformBuffer(MainPass.EntityLightBuffer[i]);
			VulkanInterface::DestroyUniformImage(MainPass.DeferredInputColorImage[i]);
			VulkanInterface::DestroyUniformImage(MainPass.DeferredInputDepthImage[i]);
			

			VulkanInterface::DestroyImageInterface(MainPass.DeferredInputColorImageInterface[i]);
			VulkanInterface::DestroyImageInterface(MainPass.DeferredInputDepthImageInterface[i]);
			VulkanInterface::DestroyImageInterface(MainPass.ShadowMapArrayImageInterface[i]);
		}

		VulkanInterface::DestroyUniformBuffer(MainPass.MaterialBuffer);

		VulkanInterface::DestroyUniformLayout(MainPass.EntityLightLayout);
		VulkanInterface::DestroyUniformLayout(MainPass.MaterialLayout);
		VulkanInterface::DestroyUniformLayout(MainPass.DeferredInputLayout);
		VulkanInterface::DestroyUniformLayout(MainPass.ShadowMapArrayLayout);
		VulkanInterface::DestroyUniformLayout(MainPass.EntitySamplerLayout);
		VulkanInterface::DestroyUniformLayout(MainPass.TerrainSkyBoxLayout);

		VulkanInterface::DestroySampler(MainPass.ShadowMapArraySampler);
		VulkanInterface::DestroySampler(MainPass.DiffuseSampler);
		VulkanInterface::DestroySampler(MainPass.SpecularSampler);

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
			VulkanInterface::BeginRenderPass(&DepthPass.RenderPass, RenderArea, LightCaster, ImageIndex);

			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, DepthPass.Pipeline.Pipeline);

			for (u32 i = 0; i < Scene->DrawEntitiesCount; ++i)
			{
				DrawEntity* DrawEntity = Scene->DrawEntities + i;

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

			vkCmdEndRenderPass(CommandBuffer);
		}
	}

	void MainPassDraw(const DrawScene* Scene, VkCommandBuffer CommandBuffer, u32 ImageIndex)
	{
		//const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet()[ImageIndex];

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };
		VulkanInterface::BeginRenderPass(&MainPass.RenderPass, RenderArea, 0, ImageIndex);

		//vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[0].Pipeline);

		//for (u32 i = 0; i < Scene->DrawTerrainEntitiesCount; ++i)
		//{
		//	DrawTerrainEntity* DrawTerrainEntity = Scene->DrawTerrainEntities + i;

		//	const VkBuffer TerrainVertexBuffers[] = { PassSharedResources.VertexBuffer.Buffer };
		//	const VkDeviceSize TerrainBuffersOffsets[] = { DrawTerrainEntity->VertexOffset };

		//	const u32 TerrainDescriptorSetGroupCount = 2;
		//	const VkDescriptorSet TerrainDescriptorSetGroup[TerrainDescriptorSetGroupCount] = {
		//		VpSet,
		//		DrawTerrainEntity->TextureSet
		//	};

		//	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[0].PipelineLayout,
		//		0, TerrainDescriptorSetGroupCount, TerrainDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

		//	vkCmdBindVertexBuffers(CommandBuffer, 0, 1, TerrainVertexBuffers, TerrainBuffersOffsets);
		//	vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, DrawTerrainEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
		//	vkCmdDrawIndexed(CommandBuffer, DrawTerrainEntity->IndicesCount, 1, 0, 0, 0);
		//}

		DynamicMapSystem::OnDraw();

		//vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[1].Pipeline);

		//for (u32 i = 0; i < Scene->DrawEntitiesCount; ++i)
		//{
		//	DrawEntity* DrawEntity = Scene->DrawEntities + i;

		//	const VkBuffer VertexBuffers[] = { PassSharedResources.VertexBuffer.Buffer };
		//	const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

		//	const VkDescriptorSet DescriptorSetGroup[] =
		//	{
		//		VpSet,
		//		DrawEntity->TextureSet,
		//		MainPass.EntityLightSet[ImageIndex],
		//		MainPass.MaterialSet,
		//		MainPass.ShadowMapArraySet[ImageIndex],
		//	};
		//	const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

		//	const VkPipelineLayout PipelineLayout = MainPass.Pipelines[1].PipelineLayout;

		//	vkCmdPushConstants(CommandBuffer, PipelineLayout,
		//		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BMRModel), &DrawEntity->Model);

		//	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
		//		0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

		//	vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
		//	vkCmdBindIndexBuffer(CommandBuffer, PassSharedResources.IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
		//	vkCmdDrawIndexed(CommandBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
		//}
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
		
		vkCmdNextSubpass(CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[3].Pipeline);

		vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.Pipelines[3].PipelineLayout,
			0, 1, &MainPass.DeferredInputSet[ImageIndex], 0, nullptr);
		vkCmdDraw(CommandBuffer, 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass
		
		vkCmdEndRenderPass(CommandBuffer);
	}

	void Draw(const DrawScene* Scene)
	{
		u32 ImageIndex = VulkanInterface::AcquireNextImageIndex();

		VulkanInterface::UpdateUniformBuffer(MainPass.EntityLightBuffer[ImageIndex], sizeof(LightBuffer), 0,
			Scene->LightEntity);

		TestCurrentFrameCommandBuffer = VulkanInterface::BeginDraw(ImageIndex);

		DepthPassDraw(Scene, TestCurrentFrameCommandBuffer, ImageIndex);
		MainPassDraw(Scene, TestCurrentFrameCommandBuffer, ImageIndex);
		
		VulkanInterface::EndDraw(ImageIndex);
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
