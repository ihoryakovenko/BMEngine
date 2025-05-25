#include "Render.h"

#include "Engine/Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/StaticMeshRender.h"
#include "Engine/Systems/Render/LightningPass.h"
#include "Engine/Systems/Render/MainPass.h"
#include "Engine/Systems/Render/DeferredPass.h"
#include "Engine/Systems/Render/DebugUI.h"
#include "Engine/Systems/Render/StaticMeshRender.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/Util.h"

namespace Render
{
	static RenderState State;
	
	static u64 StagingPoolGetMemory(StagingPool* Pool, u64 Size);
	static void ProcessCompletedTransferTasks(VkDevice Device, TaskQueue* Queue, VkFence Fence);

	void InitStaticMeshStorage(VkDevice Device, VkPhysicalDevice PhysicalDevice, StaticMeshStorage* Buffer, u64 VertexCapacity, u64 IndexCapacity)
	{
		Buffer->StaticMeshes = Memory::AllocateArray<StaticMesh>(256);

		Buffer->IndexBuffer.Offset = 0;
		Buffer->VertexBuffer.Offset = 0;

		Buffer->VertexBuffer.Buffer = VulkanHelper::CreateBuffer(Device, VertexCapacity, VulkanHelper::BufferUsageFlag::VertexFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, Buffer->VertexBuffer.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal);
		Buffer->VertexBuffer.Memory = AllocResult.Memory;
		Buffer->VertexBuffer.Alignment = AllocResult.Alignment;
		Buffer->VertexBuffer.Capacity = AllocResult.Size;
		vkBindBufferMemory(Device, Buffer->VertexBuffer.Buffer, Buffer->VertexBuffer.Memory, 0);

		Buffer->IndexBuffer.Buffer = VulkanHelper::CreateBuffer(Device, VertexCapacity, VulkanHelper::BufferUsageFlag::IndexFlag);
		AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, Buffer->IndexBuffer.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal);
		Buffer->IndexBuffer.Memory = AllocResult.Memory;
		Buffer->IndexBuffer.Alignment = AllocResult.Alignment;
		Buffer->IndexBuffer.Capacity = AllocResult.Size;
		vkBindBufferMemory(Device, Buffer->IndexBuffer.Buffer, Buffer->IndexBuffer.Memory, 0);

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}

	void InitMaterialStorage(VkDevice Device, MaterialStorage* Storage)
	{
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

	void InitTextureStorage(VkDevice Device, VkPhysicalDevice PhysicalDevice, TextureStorage* Storage)
	{
		const u32 MaxTextures = 512;

		Storage->Textures = Memory::MemoryManagementSystem::Allocate<MeshTexture2D>(MaxTextures);

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

	void InitTransferState(VkDevice Device, VkPhysicalDevice PhysicalDevice, DataTransferState* TransferState)
	{
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

			TransferState->Frames.Tasks[i].TaskBuffer = Memory::AllocateRingBuffer<TransferTask>(MaxTasks);
		}

		TransferState->TransferStagingPool.Buffer = VulkanHelper::CreateBuffer(Device, MB32, VulkanHelper::StagingFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
			TransferState->TransferStagingPool.Buffer, VulkanHelper::HostCompatible);
		TransferState->TransferStagingPool.Memory = AllocResult.Memory;
		TransferState->TransferStagingPool.Alignment = AllocResult.Alignment;
		TransferState->TransferStagingPool.Capacity = AllocResult.Size;
		TransferState->TransferStagingPool.Offset = 0;
		vkBindBufferMemory(Device, TransferState->TransferStagingPool.Buffer, TransferState->TransferStagingPool.Memory, 0);
	}

	void Init(GLFWwindow* WindowHandler, DebugUi::GuiData* GuiData)
	{
		VulkanInterface::VulkanInterfaceConfig RenderConfig;
		//RenderConfig.MaxTextures = 90;
		RenderConfig.MaxTextures = 500; // TODO: FIX!!!!

		State.DrawEntities = Memory::AllocateArray<DrawEntity>(512);

		VulkanInterface::Init(WindowHandler, RenderConfig);
		
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkDevice Device = VulkanInterface::GetDevice();

		InitTransferState(Device, PhysicalDevice, &State.TransferState);
		InitTextureStorage(Device, PhysicalDevice, &State.Textures);
		InitMaterialStorage(Device, &State.Materials);

		FrameManager::Init();

		DeferredPass::Init();
		MainPass::Init();
		LightningPass::Init();

		//TerrainRender::Init();
		//DynamicMapSystem::Init();
		StaticMeshRender::Init();
		//DebugUi::Init(WindowHandler, GuiData);

		InitStaticMeshStorage(Device, PhysicalDevice, &State.Meshes, MB32, MB32);

	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();

		VkDevice Device = VulkanInterface::GetDevice();

		vkDestroyBuffer(Device, State.Meshes.IndexBuffer.Buffer, nullptr);
		vkDestroyBuffer(Device, State.Meshes.VertexBuffer.Buffer, nullptr);

		vkFreeMemory(Device, State.Meshes.VertexBuffer.Memory, nullptr);
		vkFreeMemory(Device, State.Meshes.IndexBuffer.Memory, nullptr);

		vkDestroyBuffer(Device, State.TransferState.TransferStagingPool.Buffer, nullptr);
		vkFreeMemory(Device, State.TransferState.TransferStagingPool.Memory, nullptr);

		vkDestroyCommandPool(Device, State.TransferState.TransferCommandPool, nullptr);

		for (u32 i = 0; i < VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			vkDestroyFence(Device, State.TransferState.Frames.Fences[i], nullptr);
			Memory::FreeRingBuffer(&State.TransferState.Frames.Tasks[i].TaskBuffer); 
		}

		for (uint32_t i = 0; i < State.Textures.TextureIndex; ++i)
		{
			vkDestroyImageView(Device, State.Textures.Textures[i].View, nullptr);
			vkDestroyImage(Device, State.Textures.Textures[i].MeshTexture.Image, nullptr);
			vkFreeMemory(Device, State.Textures.Textures[i].MeshTexture.Memory, nullptr);
		}

		vkDestroyDescriptorSetLayout(Device, State.Textures.BindlesTexturesLayout, nullptr);

		vkDestroySampler(Device, State.Textures.DiffuseSampler, nullptr);
		vkDestroySampler(Device, State.Textures.SpecularSampler, nullptr);

		vkDestroyDescriptorSetLayout(Device, State.Materials.MaterialLayout, nullptr);
		vkDestroyBuffer(Device, State.Materials.MaterialBuffer, nullptr);
		vkFreeMemory(Device, State.Materials.MaterialBufferMemory, nullptr);

		//DebugUi::DeInit();
		//TerrainRender::DeInit();
		StaticMeshRender::DeInit();

		MainPass::DeInit();
		LightningPass::DeInit();
		DeferredPass::DeInit();
		FrameManager::DeInit();
		VulkanInterface::DeInit();

		Memory::FreeArray(&State.Meshes.StaticMeshes);
		Memory::MemoryManagementSystem::Free(State.Textures.Textures);
		Memory::FreeArray(&State.DrawEntities);
	}

	void Draw(const FrameManager::ViewProjectionBuffer* Data)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		const u32 CurrentIndex = VulkanInterface::TestGetImageIndex();
		ProcessCompletedTransferTasks(Device, State.TransferState.Frames.Tasks + CurrentIndex,
			State.TransferState.Frames.Fences[CurrentIndex]);


		const u32 ImageIndex = VulkanInterface::AcquireNextImageIndex();

		FrameManager::UpdateViewProjection(Data);

		VulkanInterface::BeginFrame();

		LightningPass::Draw();

		MainPass::BeginPass();

		//TerrainRender::Draw();
		StaticMeshRender::Draw();

		MainPass::EndPass();
		DeferredPass::BeginPass();

		DeferredPass::Draw();
		//DebugUi::Draw();

		DeferredPass::EndPass();

		VulkanInterface::EndFrame(State.QueueSubmitMutex);
	}

	u64 StagingPoolGetMemory(StagingPool* Pool, u64 Size)
	{
		u64 AlignedOffset = (Pool->Offset + Pool->Alignment - 1) & ~(Pool->Alignment - 1);
		Pool->Offset = AlignedOffset + Size;
		return AlignedOffset;
	}

	void ProcessCompletedTransferTasks(VkDevice Device, TaskQueue* Queue, VkFence Fence)
	{
		std::scoped_lock lock(Queue->Mutex);

		while (Queue->TaskBuffer.Head != Queue->TaskBuffer.Tail)
		{
			TransferTask* Task = Memory::RingBufferGetFirst(&Queue->TaskBuffer);
			if (vkGetFenceStatus(Device, Fence) == VK_SUCCESS)
			{
				Task->State = TRANSFER_COMPLETE;
				Memory::PushBackToArray(&State.DrawEntities, &Task->Entity);
				Memory::RingBufferPopFirst(&Queue->TaskBuffer);
			}
			else
			{
				break;
			}
		}
	}

	void SubmitTransferring(VkCommandBuffer TransferCommandBuffer, VkFence TransferFence)
	{
		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;
		SubmitInfo.signalSemaphoreCount = 0;

		std::scoped_lock lock(State.QueueSubmitMutex);
		vkQueueSubmit(VulkanInterface::GetTransferQueue(), 1, &SubmitInfo, TransferFence);
	}

	u32 CreateEntity(const DrawEntity* Entity, u32 FrameIndex)
	{
		std::scoped_lock lock(State.TransferState.Frames.Tasks[FrameIndex].Mutex);

		TransferTask Task = { };
		Task.State = TransferTaskState::TRANSFER_IN_FLIGHT;
		Task.Entity = *Entity;
		Memory::PushToRingBuffer(&State.TransferState.Frames.Tasks[FrameIndex].TaskBuffer, &Task);

		return 0;
	}

	u32 CreateMaterial(Material* Mat, u32 FrameIndex)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		const u64 TransferOffset = StagingPoolGetMemory(&State.TransferState.TransferStagingPool, sizeof(Material));

		VulkanHelper::UpdateHostCompatibleBufferMemory(Device, State.TransferState.TransferStagingPool.Memory, sizeof(Material), TransferOffset, Mat);

		VkBufferCopy BufferCopyRegion = { };
		BufferCopyRegion.srcOffset = TransferOffset;
		BufferCopyRegion.dstOffset = sizeof(Material) * State.Materials.MaterialIndex;
		BufferCopyRegion.size = sizeof(Material);

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkFence TransferFence = State.TransferState.Frames.Fences[FrameIndex];
		VkCommandBuffer TransferCommandBuffer = State.TransferState.Frames.CommandBuffers[FrameIndex];

		std::scoped_lock Lock(State.TransferState.Frames.Tasks[FrameIndex].Mutex);

		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &TransferFence));

		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo));
		vkCmdCopyBuffer(TransferCommandBuffer, State.TransferState.TransferStagingPool.Buffer, State.Materials.MaterialBuffer, 1, &BufferCopyRegion);
		VULKAN_CHECK_RESULT(vkEndCommandBuffer(TransferCommandBuffer));

		SubmitTransferring(TransferCommandBuffer, TransferFence);
		return State.Materials.MaterialIndex++;
	}

	u64 CreateStaticMesh(const StaticMeshRender::StaticMeshVertex* Vertices, u64 VerticesCount, const u32* Indices, u64 IndicesCount, u32 FrameIndex)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		const u64 Index = State.Meshes.StaticMeshes.Count;

		StaticMesh* Mesh = Memory::ArrayGetNew(&State.Meshes.StaticMeshes);
		Mesh->IndicesCount = IndicesCount;
		Mesh->IndexOffset = State.Meshes.IndexBuffer.Offset;
		Mesh->VertexOffset = State.Meshes.VertexBuffer.Offset;

		const u64 IndicesSize = sizeof(u32) * IndicesCount;
		u64 TransferOffset = StagingPoolGetMemory(&State.TransferState.TransferStagingPool, IndicesSize);

		VkBufferCopy IndexBufferCopyRegion = { };
		IndexBufferCopyRegion.srcOffset = TransferOffset;
		IndexBufferCopyRegion.dstOffset = State.Meshes.IndexBuffer.Offset;
		IndexBufferCopyRegion.size = IndicesSize;
		VulkanHelper::UpdateHostCompatibleBufferMemory(Device, State.TransferState.TransferStagingPool.Memory, IndicesSize, TransferOffset, Indices);

		const u64 VerticesSize = sizeof(StaticMeshRender::StaticMeshVertex) * VerticesCount;
		TransferOffset = StagingPoolGetMemory(&State.TransferState.TransferStagingPool, VerticesSize);

		VkBufferCopy VertexBufferCopyRegion = { };
		VertexBufferCopyRegion.srcOffset = TransferOffset;
		VertexBufferCopyRegion.dstOffset = State.Meshes.VertexBuffer.Offset;
		VertexBufferCopyRegion.size = VerticesSize;
		VulkanHelper::UpdateHostCompatibleBufferMemory(Device, State.TransferState.TransferStagingPool.Memory, VerticesSize, TransferOffset, Vertices);

		State.Meshes.IndexBuffer.Offset += sizeof(u32) * IndicesCount;
		State.Meshes.VertexBuffer.Offset += sizeof(StaticMeshRender::StaticMeshVertex) * VerticesCount;

		VkFence TransferFence = State.TransferState.Frames.Fences[FrameIndex];
		VkCommandBuffer TransferCommandBuffer = State.TransferState.Frames.CommandBuffers[FrameIndex];

		std::scoped_lock Lock(State.TransferState.Frames.Tasks[FrameIndex].Mutex);
		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &TransferFence));

		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo));
		vkCmdCopyBuffer(TransferCommandBuffer, State.TransferState.TransferStagingPool.Buffer, State.Meshes.IndexBuffer.Buffer, 1, &IndexBufferCopyRegion);
		vkCmdCopyBuffer(TransferCommandBuffer, State.TransferState.TransferStagingPool.Buffer, State.Meshes.VertexBuffer.Buffer, 1, &VertexBufferCopyRegion);
		VULKAN_CHECK_RESULT(vkEndCommandBuffer(TransferCommandBuffer));
		
		SubmitTransferring(TransferCommandBuffer, TransferFence);
		return Index;
	}

	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height, u32 FrameIndex)
	{
		assert(State.Textures.TextureIndex < 512);

		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkQueue TransferQueue = VulkanInterface::GetTransferQueue();
		MeshTexture2D* NextTexture = State.Textures.Textures + State.Textures.TextureIndex;

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
		DiffuseImageInfo.sampler = State.Textures.DiffuseSampler;
		DiffuseImageInfo.imageView = NextTexture->View;

		VkDescriptorImageInfo SpecularImageInfo = { };
		SpecularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		SpecularImageInfo.sampler = State.Textures.SpecularSampler;
		SpecularImageInfo.imageView = NextTexture->View;

		VkWriteDescriptorSet WriteDiffuse = { };
		WriteDiffuse.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDiffuse.dstSet = State.Textures.BindlesTexturesSet;
		WriteDiffuse.dstBinding = 0;
		WriteDiffuse.dstArrayElement = State.Textures.TextureIndex;
		WriteDiffuse.descriptorCount = 1;
		WriteDiffuse.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteDiffuse.pImageInfo = &DiffuseImageInfo;

		VkWriteDescriptorSet WriteSpecular = { };
		WriteSpecular.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteSpecular.dstSet = State.Textures.BindlesTexturesSet;
		WriteSpecular.dstBinding = 1;
		WriteSpecular.dstArrayElement = State.Textures.TextureIndex;
		WriteSpecular.descriptorCount = 1;
		WriteSpecular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		WriteSpecular.pImageInfo = &SpecularImageInfo;

		VkWriteDescriptorSet Writes[] = { WriteDiffuse, WriteSpecular };
		vkUpdateDescriptorSets(VulkanInterface::GetDevice(), 2, Writes, 0, nullptr);

		const u64 TransferOffset = StagingPoolGetMemory(&State.TransferState.TransferStagingPool, NextTexture->MeshTexture.Size);
		VulkanHelper::UpdateHostCompatibleBufferMemory(Device, State.TransferState.TransferStagingPool.Memory, NextTexture->MeshTexture.Size, TransferOffset, Data);

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
		TransferImageBarrier.image = NextTexture->MeshTexture.Image;
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
		PresentationBarrier.image = NextTexture->MeshTexture.Image;
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

		VkBufferImageCopy ImageRegion = { };
		ImageRegion.bufferOffset = TransferOffset;
		ImageRegion.bufferRowLength = 0;
		ImageRegion.bufferImageHeight = 0;
		ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageRegion.imageSubresource.mipLevel = 0;
		ImageRegion.imageSubresource.baseArrayLayer = 0;
		ImageRegion.imageSubresource.layerCount = 1;
		ImageRegion.imageOffset = { 0, 0, 0 };
		ImageRegion.imageExtent = { Width, Height, 1 };

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		State.Textures.TexturesPhysicalIndexes[Hash] = State.Textures.TextureIndex;

		VkFence TransferFence = State.TransferState.Frames.Fences[FrameIndex];
		VkCommandBuffer TransferCommandBuffer = State.TransferState.Frames.CommandBuffers[FrameIndex];

		std::scoped_lock Lock(State.TransferState.Frames.Tasks[FrameIndex].Mutex);
		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &TransferFence));

		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo));
		vkCmdPipelineBarrier2(TransferCommandBuffer, &TransferDepInfo);
		vkCmdCopyBufferToImage(TransferCommandBuffer, State.TransferState.TransferStagingPool.Buffer,
			NextTexture->MeshTexture.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);
		vkCmdPipelineBarrier2(TransferCommandBuffer, &PresentDepInfo);
		VULKAN_CHECK_RESULT(vkEndCommandBuffer(TransferCommandBuffer));

		SubmitTransferring(TransferCommandBuffer, TransferFence);

		return State.Textures.TextureIndex++;
	}

	RenderState* GetRenderState()
	{
		return &State;
	}

	u32 GetTexture2DSRGBIndex(u64 Hash)
	{
		auto it = State.Textures.TexturesPhysicalIndexes.find(Hash);
		if (it != State.Textures.TexturesPhysicalIndexes.end())
		{
			return it->second;
		}

		return 0;
	}
}