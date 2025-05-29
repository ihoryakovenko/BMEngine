#include "Render.h"

#include "Engine/Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/LightningPass.h"
#include "Engine/Systems/Render/MainPass.h"
#include "Engine/Systems/Render/DeferredPass.h"
#include "Engine/Systems/Render/DebugUI.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/Util.h"
#include "Util/Settings.h"
#include "Engine/Scene.h"

namespace Render
{
	struct PushConstantsData
	{
		glm::mat4 Model;
		s32 matIndex;
	};

	static const u64 MaxTransferSizePerFrame = MB4;

	static void AddTask(TransferTask* Task, TaskQueue* Queue)
	{
		std::unique_lock<std::mutex> PendingLock(Queue->Mutex);
		Memory::PushToRingBuffer(&Queue->TasksBuffer, Task);
	}

	static bool IsDrawEntityLoaded(const ResourceStorage* Storage, const DrawEntity* Entity)
	{
		Render::StaticMesh* Mesh = Storage->Meshes.StaticMeshes.Data + Entity->StaticMeshIndex;
		Render::Material* Material = Storage->Materials.Materials.Data + Entity->MaterialIndex;
		Render::MeshTexture2D* AlbedoTexture = Storage->Textures.Textures.Data + Material->AlbedoTexIndex;
		Render::MeshTexture2D* SpecTexture = Storage->Textures.Textures.Data + Material->SpecularTexIndex;
		return Mesh->IsLoaded && Material->IsLoaded && AlbedoTexture->IsLoaded && SpecTexture->IsLoaded;
	}

	static void InitStaticMeshPipeline(VkDevice Device, const ResourceStorage* Storage, StaticMeshPipeline* MeshPipeline)
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

		MeshPipeline->ShadowMapArraySampler = VulkanInterface::CreateSampler(&ShadowMapSamplerCreateInfo);


		const VkDeviceSize LightBufferSize = sizeof(Render::LightBuffer);
		MeshPipeline->StaticMeshLightLayout = FrameManager::CreateCompatibleLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		MeshPipeline->EntityLightBufferHandle = FrameManager::ReserveUniformMemory(LightBufferSize);
		MeshPipeline->StaticMeshLightSet = FrameManager::CreateAndBindSet(MeshPipeline->EntityLightBufferHandle, LightBufferSize, MeshPipeline->StaticMeshLightLayout);

		const VkDescriptorType ShadowMapArrayDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		const VkShaderStageFlags ShadowMapArrayFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		const VkDescriptorBindingFlags ShadowMapArrayBindingFlags[1] = { };
		MeshPipeline->ShadowMapArrayLayout = VulkanInterface::CreateUniformLayout(&ShadowMapArrayDescriptorType, &ShadowMapArrayFlags, ShadowMapArrayBindingFlags, 1, 1);

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
			MeshPipeline->ShadowMapArrayImageInterface[i] = VulkanInterface::CreateImageInterface(&ShadowMapArrayInterfaceCreateInfo,
				LightningPass::GetShadowMapArray()[i].Image);

			VulkanInterface::UniformSetAttachmentInfo ShadowMapArrayAttachmentInfo;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageView = MeshPipeline->ShadowMapArrayImageInterface[i];
			ShadowMapArrayAttachmentInfo.ImageInfo.sampler = MeshPipeline->ShadowMapArraySampler;
			ShadowMapArrayAttachmentInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapArrayAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			VulkanInterface::CreateUniformSets(&MeshPipeline->ShadowMapArrayLayout, 1, MeshPipeline->ShadowMapArraySet + i);
			VulkanInterface::AttachUniformsToSet(MeshPipeline->ShadowMapArraySet[i], &ShadowMapArrayAttachmentInfo, 1);
		}

		VkDescriptorSetLayout StaticMeshDescriptorLayouts[] =
		{
			FrameManager::GetViewProjectionLayout(),
			Storage->Textures.BindlesTexturesLayout,
			MeshPipeline->StaticMeshLightLayout,
			Storage->Materials.MaterialLayout,
			MeshPipeline->ShadowMapArrayLayout
		};

		const u32 StaticMeshDescriptorLayoutCount = sizeof(StaticMeshDescriptorLayouts) / sizeof(StaticMeshDescriptorLayouts[0]);

		MeshPipeline->PushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		MeshPipeline->PushConstants.offset = 0;
		// Todo: check constant and model size?
		MeshPipeline->PushConstants.size = sizeof(PushConstantsData);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = StaticMeshDescriptorLayoutCount;
		PipelineLayoutCreateInfo.pSetLayouts = StaticMeshDescriptorLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		PipelineLayoutCreateInfo.pPushConstantRanges = &MeshPipeline->PushConstants;

		VULKAN_CHECK_RESULT(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &MeshPipeline->Pipeline.PipelineLayout));

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
		ResourceInfo.PipelineLayout = MeshPipeline->Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData = *MainPass::GetAttachmentData();

		MeshPipeline->Pipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, VertexInputBinding, 1,
			&PipelineSettings, &ResourceInfo);
	}

	static void DeInitStaticMeshPipeline(VkDevice Device, StaticMeshPipeline* MeshPipeline)
	{
		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			vkDestroyImageView(Device, MeshPipeline->ShadowMapArrayImageInterface[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(Device, MeshPipeline->StaticMeshLightLayout, nullptr);
		vkDestroyDescriptorSetLayout(Device, MeshPipeline->ShadowMapArrayLayout, nullptr);

		vkDestroyPipeline(Device, MeshPipeline->Pipeline.Pipeline, nullptr);
		vkDestroyPipelineLayout(Device, MeshPipeline->Pipeline.PipelineLayout, nullptr);

		vkDestroySampler(Device, MeshPipeline->ShadowMapArraySampler, nullptr);
	}

	static void DrawStaticMeshes(VkDevice Device, VkCommandBuffer CmdBuffer, const ResourceStorage* Storage, StaticMeshPipeline* MeshPipeline)
	{
		FrameManager::UpdateUniformMemory(MeshPipeline->EntityLightBufferHandle, Scene.LightEntity, sizeof(Render::LightBuffer));

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.Pipeline);

		const VkDescriptorSet VpSet = FrameManager::GetViewProjectionSet();
		const u32 DynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(FrameManager::ViewProjectionBuffer);
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
			0, 1, &VpSet, 1, &DynamicOffset);

		VkDescriptorSet BindlesTexturesSet = Storage->Textures.BindlesTexturesSet;
		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
			1, 1, &BindlesTexturesSet, 0, nullptr);

		const u32 LightDynamicOffset = VulkanInterface::TestGetImageIndex() * sizeof(Render::LightBuffer);

		for (u32 i = 0; i < Storage->DrawEntities.Count; ++i)
		{
			Render::DrawEntity* DrawEntity = Storage->DrawEntities.Data + i;
			Render::StaticMesh* Mesh = Storage->Meshes.StaticMeshes.Data + DrawEntity->StaticMeshIndex;

			if (!IsDrawEntityLoaded(Storage, DrawEntity))
			{
				continue;
			}

			const VkDescriptorSet DescriptorSetGroup[] =
			{
				MeshPipeline->StaticMeshLightSet,
				Storage->Materials.MaterialSet,
				MeshPipeline->ShadowMapArraySet[VulkanInterface::TestGetImageIndex()],
			};
			const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

			PushConstantsData Constants;
			Constants.Model = DrawEntity->Model;
			Constants.matIndex = DrawEntity->MaterialIndex;

			const VkShaderStageFlags Flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			vkCmdPushConstants(CmdBuffer, MeshPipeline->Pipeline.PipelineLayout, Flags, 0, sizeof(PushConstantsData), &Constants);

			vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MeshPipeline->Pipeline.PipelineLayout,
				2, DescriptorSetGroupCount, DescriptorSetGroup, 1, &LightDynamicOffset);

			vkCmdBindVertexBuffers(CmdBuffer, 0, 1, &Storage->Meshes.VertexStageData.Buffer, &Mesh->VertexOffset);
			vkCmdBindIndexBuffer(CmdBuffer, Storage->Meshes.VertexStageData.Buffer, Mesh->IndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(CmdBuffer, Mesh->IndicesCount, 1, 0, 0, 0);
		}
	}

	static void InitStaticMeshStorage(VkDevice Device, VkPhysicalDevice PhysicalDevice, StaticMeshStorage* Storage, u64 VertexCapacity)
	{
		Storage->StaticMeshes = Memory::AllocateArray<StaticMesh>(512);

		Storage->VertexStageData = { };
		Storage->VertexStageData.Buffer = VulkanHelper::CreateBuffer(Device, VertexCapacity, VulkanHelper::BufferUsageFlag::CombinedVertexIndexFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, Storage->VertexStageData.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal);
		Storage->VertexStageData.Memory = AllocResult.Memory;
		Storage->VertexStageData.Alignment = 1;
		Storage->VertexStageData.Capacity = AllocResult.Size;
		vkBindBufferMemory(Device, Storage->VertexStageData.Buffer, Storage->VertexStageData.Memory, 0);

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}

	static void DeInitStaticMeshStorage(VkDevice Device, StaticMeshStorage* Storage)
	{
		vkDestroyBuffer(Device, Storage->VertexStageData.Buffer, nullptr);
		vkFreeMemory(Device, Storage->VertexStageData.Memory, nullptr);
		Memory::FreeArray(&Storage->StaticMeshes);
	}

	static void InitMaterialStorage(VkDevice Device, MaterialStorage* Storage)
	{
		Storage->Materials = Memory::AllocateArray<Material>(512);

		const VkDeviceSize MaterialBufferSize = sizeof(Material) * 512;
		Storage->MaterialBuffer = VulkanHelper::CreateBuffer(Device, MaterialBufferSize, VulkanHelper::BufferUsageFlag::StorageFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(VulkanInterface::GetPhysicalDevice(), Device,
			Storage->MaterialBuffer, VulkanHelper::MemoryPropertyFlag::GPULocal);
		Storage->MaterialBufferMemory = AllocResult.Memory;
		vkBindBufferMemory(Device, Storage->MaterialBuffer, Storage->MaterialBufferMemory, 0);

		VulkanInterface::UniformSetAttachmentInfo MaterialAttachmentInfo;
		MaterialAttachmentInfo.BufferInfo.buffer = Storage->MaterialBuffer;
		MaterialAttachmentInfo.BufferInfo.offset = 0;
		MaterialAttachmentInfo.BufferInfo.range = MaterialBufferSize;
		MaterialAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		const VkDescriptorType MaterialDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		const VkShaderStageFlags MaterialStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		const VkDescriptorBindingFlags MaterialBindingFlags[1] = { };
		Storage->MaterialLayout = VulkanInterface::CreateUniformLayout(&MaterialDescriptorType, &MaterialStageFlags, MaterialBindingFlags, 1, 1);
		VulkanInterface::CreateUniformSets(&Storage->MaterialLayout, 1, &Storage->MaterialSet);
		VulkanInterface::AttachUniformsToSet(Storage->MaterialSet, &MaterialAttachmentInfo, 1);
	}

	static void DeInitMaterialStorage(VkDevice Device, MaterialStorage* Storage)
	{
		vkDestroyDescriptorSetLayout(Device, Storage->MaterialLayout, nullptr);
		vkDestroyBuffer(Device, Storage->MaterialBuffer, nullptr);
		vkFreeMemory(Device, Storage->MaterialBufferMemory, nullptr);
		Memory::FreeArray(&Storage->Materials);
	}

	static void InitTextureStorage(VkDevice Device, VkPhysicalDevice PhysicalDevice, TextureStorage* Storage)
	{
		const u32 MaxTextures = 512;

		Storage->Textures = Memory::AllocateArray<MeshTexture2D>(64);

		VkSamplerCreateInfo DiffuseSamplerCreateInfo = { };
		DiffuseSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		DiffuseSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		DiffuseSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		DiffuseSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		DiffuseSamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		DiffuseSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		DiffuseSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		DiffuseSamplerCreateInfo.mipLodBias = 0.0f;
		DiffuseSamplerCreateInfo.minLod = 0.0f;
		DiffuseSamplerCreateInfo.maxLod = 0.0f;
		DiffuseSamplerCreateInfo.anisotropyEnable = VK_TRUE;
		DiffuseSamplerCreateInfo.maxAnisotropy = 16;

		VkSamplerCreateInfo SpecularSamplerCreateInfo = DiffuseSamplerCreateInfo;
		DiffuseSamplerCreateInfo.maxAnisotropy = 1;

		Storage->DiffuseSampler = VulkanInterface::CreateSampler(&DiffuseSamplerCreateInfo);
		Storage->SpecularSampler = VulkanInterface::CreateSampler(&SpecularSamplerCreateInfo);

		const VkShaderStageFlags EntitySamplerInputFlags[2] = { VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
		const VkDescriptorType EntitySamplerDescriptorType[2] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
		const VkDescriptorBindingFlags BindingFlags[2] = {
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };

		// TODO: Get max textures from limits.maxPerStageDescriptorSampledImages;
		Storage->BindlesTexturesLayout = VulkanInterface::CreateUniformLayout(EntitySamplerDescriptorType, EntitySamplerInputFlags, BindingFlags, 2, MaxTextures);
		VulkanInterface::CreateUniformSets(&Storage->BindlesTexturesLayout, 1, &Storage->BindlesTexturesSet);
	}

	static void DeInitTextureStorage(VkDevice Device, TextureStorage* Storage)
	{
		for (uint32_t i = 0; i < Storage->Textures.Count; ++i)
		{
			vkDestroyImageView(Device, Storage->Textures.Data[i].View, nullptr);
			vkDestroyImage(Device, Storage->Textures.Data[i].MeshTexture.Image, nullptr);
			vkFreeMemory(Device, Storage->Textures.Data[i].MeshTexture.Memory, nullptr);
		}

		vkDestroyDescriptorSetLayout(Device, Storage->BindlesTexturesLayout, nullptr);

		vkDestroySampler(Device, Storage->DiffuseSampler, nullptr);
		vkDestroySampler(Device, Storage->SpecularSampler, nullptr);

		Memory::FreeArray(&Storage->Textures);
	}

	static void InitTransferState(VkDevice Device, VkPhysicalDevice PhysicalDevice, DataTransferState* TransferState)
	{
		TransferState->CurrentFrame = 0;

		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = VulkanInterface::GetQueueGraphicsFamilyIndex();

		VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &PoolInfo, nullptr, &TransferState->TransferCommandPool));

		VkCommandBufferAllocateInfo TransferCommandBufferAllocateInfo = { };
		TransferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		TransferCommandBufferAllocateInfo.commandPool = TransferState->TransferCommandPool;
		TransferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		TransferCommandBufferAllocateInfo.commandBufferCount = VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT;

		VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &TransferCommandBufferAllocateInfo, TransferState->Frames.CommandBuffers));

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (u32 i = 0; i < VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, TransferState->Frames.Fences + i));
		}

		VkSemaphoreTypeCreateInfo TimelineCreateInfo = { };
		TimelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		TimelineCreateInfo.pNext = nullptr;
		TimelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		TimelineCreateInfo.initialValue = 0;

		VkSemaphoreCreateInfo SemaphoreInfo = { };
		SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		SemaphoreInfo.pNext = &TimelineCreateInfo;
		SemaphoreInfo.flags = 0;

		VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &TransferState->TransferSemaphore));
		TransferState->CompletedTransfer = 0;
		TransferState->TasksInFly = 0;

		TransferState->PendingTransferTasksQueue.TasksBuffer = Memory::AllocateRingBuffer<TransferTask>(1024);
		TransferState->ActiveTransferTasksQueue.TasksBuffer = Memory::AllocateRingBuffer<TransferTask>(1024);

		TransferState->TransferStagingPool = { };

		TransferState->TransferStagingPool.Buffer = VulkanHelper::CreateBuffer(Device, MB16, VulkanHelper::StagingFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
			TransferState->TransferStagingPool.Buffer, VulkanHelper::HostCompatible);
		TransferState->TransferStagingPool.Memory = AllocResult.Memory;
		//TransferState->TransferStagingPool.ControlBlock.Alignment = AllocResult.Alignment;
		TransferState->TransferStagingPool.ControlBlock.Capacity = AllocResult.Size;
		vkBindBufferMemory(Device, TransferState->TransferStagingPool.Buffer, TransferState->TransferStagingPool.Memory, 0);

		TransferState->TransferMemory = Memory::AllocateRingBuffer<u8>(MB16);
	}

	static void DeInitTransferState(VkDevice Device, DataTransferState* TransferState)
	{
		vkDestroyBuffer(Device, TransferState->TransferStagingPool.Buffer, nullptr);
		vkFreeMemory(Device, TransferState->TransferStagingPool.Memory, nullptr);

		vkDestroyCommandPool(Device, TransferState->TransferCommandPool, nullptr);

		for (u32 i = 0; i < VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			vkDestroyFence(Device, TransferState->Frames.Fences[i], nullptr);
		}

		vkDestroySemaphore(Device, TransferState->TransferSemaphore, nullptr);

		Memory::FreeRingBuffer(&TransferState->PendingTransferTasksQueue.TasksBuffer);
		Memory::FreeRingBuffer(&TransferState->ActiveTransferTasksQueue.TasksBuffer);
		Memory::FreeRingBuffer(&TransferState->TransferMemory);
	}

	static void Transfer(DataTransferState* TransferState, std::mutex* QueueSubmitMutex)
	{
		u64 FrameTotalTransferSize = 0;
		const u32 CurrentFrame = TransferState->CurrentFrame;
		VkDevice Device = VulkanInterface::GetDevice();

		VkFence TransferFence = TransferState->Frames.Fences[CurrentFrame];
		VkCommandBuffer TransferCommandBuffer = TransferState->Frames.CommandBuffers[CurrentFrame];

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &TransferFence));

		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo));

		std::unique_lock<std::mutex> PendingLock(TransferState->PendingTransferTasksQueue.Mutex);
		TransferState->PendingCv.wait(PendingLock, [TransferState]
		{
			return !Memory::IsRingBufferEmpty(&TransferState->PendingTransferTasksQueue.TasksBuffer);
		});

		while (!Memory::IsRingBufferEmpty(&TransferState->PendingTransferTasksQueue.TasksBuffer))
		{
			TransferTask* Task = Memory::RingBufferGetFirst(&TransferState->PendingTransferTasksQueue.TasksBuffer);

			switch (Task->Type)
			{
				case TransferTaskType::TransferTaskType_Mesh:
				{
					const u64 TransferOffset = Memory::RingAlloc(&TransferState->TransferStagingPool.ControlBlock, Task->DataSize, 1);
					VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState->TransferStagingPool.Memory, Task->DataSize,
						TransferOffset, Task->RawData);

					VkBufferMemoryBarrier2 Barrier = { };
					Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
					Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					Barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
					Barrier.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
					Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.buffer = Task->DataDescr.DstBuffer;
					Barrier.offset = Task->DataDescr.DstOffset;
					Barrier.size = Task->DataSize;

					VkDependencyInfo DepInfo = { };
					DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					DepInfo.bufferMemoryBarrierCount = 1;
					DepInfo.pBufferMemoryBarriers = &Barrier;

					VkBufferCopy IndexBufferCopyRegion = { };
					IndexBufferCopyRegion.srcOffset = TransferOffset;
					IndexBufferCopyRegion.dstOffset = Task->DataDescr.DstOffset;
					IndexBufferCopyRegion.size = Task->DataSize;

					vkCmdPipelineBarrier2(TransferCommandBuffer, &DepInfo);
					vkCmdCopyBuffer(TransferCommandBuffer, TransferState->TransferStagingPool.Buffer, Task->DataDescr.DstBuffer, 1, &IndexBufferCopyRegion);

					break;
				}
				case TransferTaskType::TransferTaskType_Material:
				{
					const u64 TransferOffset = Memory::RingAlloc(&TransferState->TransferStagingPool.ControlBlock, Task->DataSize, 1);
					VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState->TransferStagingPool.Memory, Task->DataSize,
						TransferOffset, Task->RawData);

					VkBufferMemoryBarrier2 Barrier = { };
					Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
					Barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
					Barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;
					Barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					Barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.buffer = Task->DataDescr.DstBuffer;
					Barrier.offset = Task->DataDescr.DstOffset;
					Barrier.size = Task->DataSize;

					VkDependencyInfo DepInfo = { };
					DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					DepInfo.bufferMemoryBarrierCount = 1;
					DepInfo.pBufferMemoryBarriers = &Barrier;

					VkBufferCopy IndexBufferCopyRegion = { };
					IndexBufferCopyRegion.srcOffset = TransferOffset;
					IndexBufferCopyRegion.dstOffset = Task->DataDescr.DstOffset;
					IndexBufferCopyRegion.size = Task->DataSize;

					vkCmdPipelineBarrier2(TransferCommandBuffer, &DepInfo);
					vkCmdCopyBuffer(TransferCommandBuffer, TransferState->TransferStagingPool.Buffer, Task->DataDescr.DstBuffer, 1, &IndexBufferCopyRegion);

					break;
				}
				case TransferTaskType::TransferTaskType_Texture:
				{
					const u64 TransferOffset = Memory::RingAlloc(&TransferState->TransferStagingPool.ControlBlock, Task->DataSize, 16);
					VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState->TransferStagingPool.Memory, Task->DataSize,
						TransferOffset, Task->RawData);

					VkImageMemoryBarrier2 TransferImageBarrier = { };
					TransferImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
					TransferImageBarrier.pNext = nullptr;
					TransferImageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
					TransferImageBarrier.srcAccessMask = 0;
					TransferImageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					TransferImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					TransferImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					TransferImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					TransferImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					TransferImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					TransferImageBarrier.image = Task->TextureDescr.DstImage;
					TransferImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					TransferImageBarrier.subresourceRange.baseMipLevel = 0;
					TransferImageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
					TransferImageBarrier.subresourceRange.baseArrayLayer = 0;
					TransferImageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

					VkDependencyInfo TransferDepInfo = { };
					TransferDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					TransferDepInfo.pNext = nullptr;
					TransferDepInfo.dependencyFlags = 0;
					TransferDepInfo.imageMemoryBarrierCount = 1;
					TransferDepInfo.pImageMemoryBarriers = &TransferImageBarrier;
					TransferDepInfo.memoryBarrierCount = 0;
					TransferDepInfo.pMemoryBarriers = nullptr;
					TransferDepInfo.bufferMemoryBarrierCount = 0;
					TransferDepInfo.pBufferMemoryBarriers = nullptr;

					VkBufferImageCopy ImageRegion = { };
					ImageRegion.bufferOffset = TransferOffset;
					ImageRegion.bufferRowLength = 0;
					ImageRegion.bufferImageHeight = 0;
					ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					ImageRegion.imageSubresource.mipLevel = 0;
					ImageRegion.imageSubresource.baseArrayLayer = 0;
					ImageRegion.imageSubresource.layerCount = 1;
					ImageRegion.imageOffset = { 0, 0, 0 };
					ImageRegion.imageExtent = { Task->TextureDescr.Width, Task->TextureDescr.Height, 1 };

					VkImageMemoryBarrier2 PresentationBarrier = { };
					PresentationBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
					PresentationBarrier.pNext = nullptr;
					PresentationBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					PresentationBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					PresentationBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
					PresentationBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
					PresentationBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					PresentationBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					PresentationBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					PresentationBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					PresentationBarrier.image = Task->TextureDescr.DstImage;
					PresentationBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					PresentationBarrier.subresourceRange.baseMipLevel = 0;
					PresentationBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
					PresentationBarrier.subresourceRange.baseArrayLayer = 0;
					PresentationBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

					VkDependencyInfo PresentDepInfo = { };
					PresentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					PresentDepInfo.pNext = nullptr;
					PresentDepInfo.dependencyFlags = 0;
					PresentDepInfo.imageMemoryBarrierCount = 1;
					PresentDepInfo.pImageMemoryBarriers = &PresentationBarrier;
					PresentDepInfo.memoryBarrierCount = 0;
					PresentDepInfo.pMemoryBarriers = nullptr;
					PresentDepInfo.bufferMemoryBarrierCount = 0;
					PresentDepInfo.pBufferMemoryBarriers = nullptr;

					vkCmdPipelineBarrier2(TransferCommandBuffer, &TransferDepInfo);
					vkCmdCopyBufferToImage(TransferCommandBuffer, TransferState->TransferStagingPool.Buffer,
						Task->TextureDescr.DstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);
					vkCmdPipelineBarrier2(TransferCommandBuffer, &PresentDepInfo);

					break;
				}
			}

			std::unique_lock ActiveTasksLock(TransferState->ActiveTransferTasksQueue.Mutex);
			Memory::PushToRingBuffer(&TransferState->ActiveTransferTasksQueue.TasksBuffer, Task);
			++TransferState->TasksInFly;
			ActiveTasksLock.unlock();

			Memory::RingBufferPopFirst(&TransferState->PendingTransferTasksQueue.TasksBuffer);

			FrameTotalTransferSize += Task->DataSize;
			if (FrameTotalTransferSize >= MaxTransferSizePerFrame)
			{
				break;
			}
		}

		PendingLock.unlock();

		VULKAN_CHECK_RESULT(vkEndCommandBuffer(TransferCommandBuffer));

		const u64 TasksInFly = TransferState->TasksInFly;

		VkTimelineSemaphoreSubmitInfo TimelineSubmitInfo = { };
		TimelineSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		TimelineSubmitInfo.signalSemaphoreValueCount = 1;
		TimelineSubmitInfo.pSignalSemaphoreValues = &TasksInFly;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;
		SubmitInfo.pNext = &TimelineSubmitInfo;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &TransferState->TransferSemaphore;

		std::unique_lock SubmitLock(*QueueSubmitMutex);
		vkQueueSubmit(VulkanInterface::GetTransferQueue(), 1, &SubmitInfo, TransferFence);
		SubmitLock.unlock();

		TransferState->CurrentFrame = (CurrentFrame + 1) % VulkanInterface::MAX_DRAW_FRAMES;
	}

	static void ProcessTransferTasks(VkDevice Device, VkCommandBuffer CmdBuffer, DataTransferState* TransferState, const ResourceStorage* Storage)
	{
		u64 CompletedCounter = 0;
		vkGetSemaphoreCounterValue(Device, TransferState->TransferSemaphore, &CompletedCounter);

		u64 Counter = CompletedCounter - TransferState->CompletedTransfer;
		if (Counter > 0)
		{
			std::unique_lock Lock(TransferState->ActiveTransferTasksQueue.Mutex);

			while (Counter--)
			{
				TransferTask* Task = Memory::RingBufferGetFirst(&TransferState->ActiveTransferTasksQueue.TasksBuffer);

				switch (Task->Type)
				{
					case TransferTaskType::TransferTaskType_Mesh:
					{
						VkBufferMemoryBarrier2 Barrier = { };
						Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
						Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
						Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
						Barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
						Barrier.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
						Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						Barrier.buffer = Task->DataDescr.DstBuffer;
						Barrier.offset = Task->DataDescr.DstOffset;
						Barrier.size = Task->DataSize;

						VkDependencyInfo DepInfo = { };
						DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
						DepInfo.bufferMemoryBarrierCount = 1;
						DepInfo.pBufferMemoryBarriers = &Barrier;

						vkCmdPipelineBarrier2(CmdBuffer, &DepInfo);

						Storage->Meshes.StaticMeshes.Data[Task->ResourceIndex].IsLoaded = true;
						Memory::RingFree(&TransferState->TransferMemory.ControlBlock, Task->DataSize, 1);

						break;
					}
					case TransferTaskType::TransferTaskType_Material:
					{
						VkBufferMemoryBarrier2 Barrier = { };
						Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
						Barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
						Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
						Barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
						Barrier.dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;
						Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						Barrier.buffer = Task->DataDescr.DstBuffer;
						Barrier.offset = Task->DataDescr.DstOffset;
						Barrier.size = Task->DataSize;

						VkDependencyInfo DepInfo = { };
						DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
						DepInfo.bufferMemoryBarrierCount = 1;
						DepInfo.pBufferMemoryBarriers = &Barrier;

						vkCmdPipelineBarrier2(CmdBuffer, &DepInfo);

						Storage->Materials.Materials.Data[Task->ResourceIndex].IsLoaded = true;

						break;
					}
					case TransferTaskType::TransferTaskType_Texture:
					{
						Storage->Textures.Textures.Data[Task->ResourceIndex].IsLoaded = true;
						Memory::RingFree(&TransferState->TransferMemory.ControlBlock, Task->DataSize, 1);

						break;
					}
				}

				Memory::RingBufferPopFirst(&TransferState->ActiveTransferTasksQueue.TasksBuffer);
			}

			TransferState->CompletedTransfer = CompletedCounter;
		}
	}



	static RenderState State;

	static void* RequestTransferMemory(u64 Size)
	{
		return State.TransferState.TransferMemory.DataArray + Memory::RingAlloc(&State.TransferState.TransferMemory.ControlBlock, Size, 1);
	}

	void Init(GLFWwindow* WindowHandler, DebugUi::GuiData* GuiData)
	{
		VulkanInterface::VulkanInterfaceConfig RenderConfig;
		//RenderConfig.MaxTextures = 90;
		RenderConfig.MaxTextures = 500; // TODO: FIX!!!!

		State.RenderResources.DrawEntities = Memory::AllocateArray<DrawEntity>(512);

		VulkanInterface::Init(WindowHandler, RenderConfig);
		
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkDevice Device = VulkanInterface::GetDevice();

		InitTransferState(Device, PhysicalDevice, &State.TransferState);
		InitTextureStorage(Device, PhysicalDevice, &State.RenderResources.Textures);
		InitMaterialStorage(Device, &State.RenderResources.Materials);
		InitStaticMeshStorage(Device, PhysicalDevice, &State.RenderResources.Meshes, MB4);

		FrameManager::Init();

		DeferredPass::Init();
		MainPass::Init();
		LightningPass::Init();

		//TerrainRender::Init();
		//DynamicMapSystem::Init();
		InitStaticMeshPipeline(Device, &State.RenderResources, &State.MeshPipeline);
		DebugUi::Init(WindowHandler, GuiData);

		

		std::thread TransferThread(
			[]()
			{
				while (true) // todo use atomic or think about valid break
				{
					Transfer(&State.TransferState, &State.QueueSubmitMutex);
				}
			});

		TransferThread.detach();
	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();

		VkDevice Device = VulkanInterface::GetDevice();

		DeInitTransferState(Device, &State.TransferState);
		DeInitTextureStorage(Device, &State.RenderResources.Textures);
		DeInitMaterialStorage(Device, &State.RenderResources.Materials);
		DeInitStaticMeshStorage(Device, &State.RenderResources.Meshes);

		DebugUi::DeInit();
		//TerrainRender::DeInit();
		DeInitStaticMeshPipeline(Device, &State.MeshPipeline);

		MainPass::DeInit();
		LightningPass::DeInit();
		DeferredPass::DeInit();
		FrameManager::DeInit();
		VulkanInterface::DeInit();



		Memory::FreeArray(&State.RenderResources.DrawEntities);
	}

	void Draw(const FrameManager::ViewProjectionBuffer* Data)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		const u32 ImageIndex = VulkanInterface::AcquireNextImageIndex();

		FrameManager::UpdateViewProjection(Data);

		VulkanInterface::BeginFrame();

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		ProcessTransferTasks(Device, CmdBuffer, &State.TransferState, &State.RenderResources);

		LightningPass::Draw();

		MainPass::BeginPass();

		//TerrainRender::Draw();

		DrawStaticMeshes(Device, CmdBuffer, &State.RenderResources, &State.MeshPipeline);

		MainPass::EndPass();
		DeferredPass::BeginPass();

		DeferredPass::Draw();
		DebugUi::Draw();

		DeferredPass::EndPass();

		VulkanInterface::EndFrame(State.QueueSubmitMutex);
	}

	void NotifyTransfer()
	{
		State.TransferState.PendingCv.notify_one();
	}

	u32 CreateEntity(const DrawEntity* Entity)
	{
		Memory::PushBackToArray(&State.RenderResources.DrawEntities, Entity);
		return 0;
	}

	u32 CreateMaterial(Material* Mat)
	{
		Mat->IsLoaded = false;
		const u64 Index = State.RenderResources.Materials.Materials.Count;

		Material* NewMaterial = State.RenderResources.Materials.Materials.Data + State.RenderResources.Materials.Materials.Count;
		Memory::PushBackToArray(&State.RenderResources.Materials.Materials, Mat);

		TransferTask Task = { };
		//Task.DataSize = sizeof(Material);
		Task.DataSize = 12;
		Task.DataDescr.DstBuffer = State.RenderResources.Materials.MaterialBuffer;
		//Task.DataDescr.DstOffset = sizeof(Material) * Index;
		Task.DataDescr.DstOffset = 12 * Index;
		Task.RawData = NewMaterial;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Material;

		AddTask(&Task, &State.TransferState.PendingTransferTasksQueue);

		return Index;
	}

	u64 CreateStaticMesh(void* MeshVertexData, u64 VertexSize, u64 VerticesCount, u64 IndicesCount)
	{
		const u64 VerticesSize = VertexSize * VerticesCount;
		const u64 DataSize = sizeof(u32) * IndicesCount + VerticesSize;

		// TODO: TMP solution
		void* TransferMemory = Render::RequestTransferMemory(DataSize);
		memcpy(TransferMemory, MeshVertexData, DataSize);

		const u64 Index = State.RenderResources.Meshes.StaticMeshes.Count;
		StaticMesh* Mesh = Memory::ArrayGetNew(&State.RenderResources.Meshes.StaticMeshes);
		*Mesh = { };
		Mesh->IndicesCount = IndicesCount;
		Mesh->VertexOffset = State.RenderResources.Meshes.VertexStageData.Offset;
		Mesh->IndexOffset = State.RenderResources.Meshes.VertexStageData.Offset + VerticesSize;
		Mesh->VertexDataSize = DataSize;

		TransferTask Task = { };
		Task.DataSize = DataSize;
		Task.DataDescr.DstBuffer = State.RenderResources.Meshes.VertexStageData.Buffer;
		Task.DataDescr.DstOffset = State.RenderResources.Meshes.VertexStageData.Offset;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Mesh;

		AddTask(&Task, &State.TransferState.PendingTransferTasksQueue);

		State.RenderResources.Meshes.VertexStageData.Offset += DataSize;
		return Index;
	}

	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height)
	{
		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkQueue TransferQueue = VulkanInterface::GetTransferQueue();

		const u64 Index = State.RenderResources.Textures.Textures.Count;
		MeshTexture2D* NextTexture = Memory::ArrayGetNew(&State.RenderResources.Textures.Textures);
		*NextTexture = { };

		VkImageCreateInfo ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Width;
		ImageCreateInfo.extent.height = Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.format = VK_FORMAT_BC7_SRGB_BLOCK;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.flags = 0;

		VULKAN_CHECK_RESULT(vkCreateImage(Device, &ImageCreateInfo, nullptr, &NextTexture->MeshTexture.Image));

		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
			NextTexture->MeshTexture.Image, VulkanHelper::GPULocal);
		NextTexture->MeshTexture.Memory = AllocResult.Memory;
		NextTexture->MeshTexture.Alignment = AllocResult.Alignment;
		NextTexture->MeshTexture.Size = AllocResult.Size;
		VULKAN_CHECK_RESULT(vkBindImageMemory(Device, NextTexture->MeshTexture.Image, NextTexture->MeshTexture.Memory, 0));

		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.flags = 0;
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = VK_FORMAT_BC7_SRGB_BLOCK;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;
		ViewCreateInfo.image = NextTexture->MeshTexture.Image;

		VULKAN_CHECK_RESULT(vkCreateImageView(Device, &ViewCreateInfo, nullptr, &NextTexture->View));

		VkDescriptorImageInfo DiffuseImageInfo = { };
		DiffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DiffuseImageInfo.sampler = State.RenderResources.Textures.DiffuseSampler;
		DiffuseImageInfo.imageView = NextTexture->View;

		VkDescriptorImageInfo SpecularImageInfo = { };
		SpecularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SpecularImageInfo.sampler = State.RenderResources.Textures.SpecularSampler;
		SpecularImageInfo.imageView = NextTexture->View;

		VkWriteDescriptorSet WriteDiffuse = { };
		WriteDiffuse.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDiffuse.dstSet = State.RenderResources.Textures.BindlesTexturesSet;
		WriteDiffuse.dstBinding = 0;
		WriteDiffuse.dstArrayElement = Index;
		WriteDiffuse.descriptorCount = 1;
		WriteDiffuse.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDiffuse.pImageInfo = &DiffuseImageInfo;

		VkWriteDescriptorSet WriteSpecular = { };
		WriteSpecular.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteSpecular.dstSet = State.RenderResources.Textures.BindlesTexturesSet;
		WriteSpecular.dstBinding = 1;
		WriteSpecular.dstArrayElement = Index;
		WriteSpecular.descriptorCount = 1;
		WriteSpecular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteSpecular.pImageInfo = &SpecularImageInfo;

		VkWriteDescriptorSet Writes[] = { WriteDiffuse, WriteSpecular };
		vkUpdateDescriptorSets(VulkanInterface::GetDevice(), 2, Writes, 0, nullptr);

		// TODO: TMP solution
		void* TransferMemory = Render::RequestTransferMemory(NextTexture->MeshTexture.Size);
		memcpy(TransferMemory, Data, NextTexture->MeshTexture.Size);

		TransferTask Task = { };
		Task.DataSize = NextTexture->MeshTexture.Size;
		Task.TextureDescr.DstImage = NextTexture->MeshTexture.Image;
		Task.TextureDescr.Width = Width;
		Task.TextureDescr.Height = Height;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Texture;

		AddTask(&Task, &State.TransferState.PendingTransferTasksQueue);

		return Index;
	}

	RenderState* GetRenderState()
	{
		return &State;
	}
}