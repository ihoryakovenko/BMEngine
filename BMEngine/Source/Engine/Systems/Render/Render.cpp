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
	static const u64 MaxTransferSizePerFrame = MB4;

	static RenderState State;

	static void AddTask(TransferTask* Task);

	static void* RequestTransferMemory(u64 Size);
	
	void InitStaticMeshStorage(VkDevice Device, VkPhysicalDevice PhysicalDevice, StaticMeshStorage* Buffer, u64 VertexCapacity)
	{
		Buffer->StaticMeshes = Memory::AllocateArray<StaticMesh>(512);

		Buffer->VertexStageData = { };
		Buffer->VertexStageData.Buffer = VulkanHelper::CreateBuffer(Device, VertexCapacity, VulkanHelper::BufferUsageFlag::CombinedVertexIndexFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device, Buffer->VertexStageData.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal);
		Buffer->VertexStageData.Memory = AllocResult.Memory;
		Buffer->VertexStageData.Alignment = 1;
		Buffer->VertexStageData.Capacity = AllocResult.Size;
		vkBindBufferMemory(Device, Buffer->VertexStageData.Buffer, Buffer->VertexStageData.Memory, 0);

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}

	void InitMaterialStorage(VkDevice Device, MaterialStorage* Storage)
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

		InitStaticMeshStorage(Device, PhysicalDevice, &State.Meshes, MB4);

		std::thread TransferThread(
			[]()
			{
				while (true)
				{
					Transfer();
				}
			});

		TransferThread.detach();
	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();

		VkDevice Device = VulkanInterface::GetDevice();

		vkDestroyBuffer(Device, State.Meshes.VertexStageData.Buffer, nullptr);
		vkFreeMemory(Device, State.Meshes.VertexStageData.Memory, nullptr);

		vkDestroyBuffer(Device, State.TransferState.TransferStagingPool.Buffer, nullptr);
		vkFreeMemory(Device, State.TransferState.TransferStagingPool.Memory, nullptr);

		vkDestroyCommandPool(Device, State.TransferState.TransferCommandPool, nullptr);

		for (u32 i = 0; i < VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			vkDestroyFence(Device, State.TransferState.Frames.Fences[i], nullptr);
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

		vkDestroySemaphore(Device, State.TransferState.TransferSemaphore, nullptr);

		//DebugUi::DeInit();
		//TerrainRender::DeInit();
		StaticMeshRender::DeInit();

		MainPass::DeInit();
		LightningPass::DeInit();
		DeferredPass::DeInit();
		FrameManager::DeInit();
		VulkanInterface::DeInit();

		Memory::FreeArray(&State.Meshes.StaticMeshes);
		Memory::FreeArray(&State.Materials.Materials);
		Memory::MemoryManagementSystem::Free(State.Textures.Textures);
		Memory::FreeArray(&State.DrawEntities);
		Memory::FreeRingBuffer(&State.TransferState.PendingTransferTasksQueue.TasksBuffer);
		Memory::FreeRingBuffer(&State.TransferState.ActiveTransferTasksQueue.TasksBuffer);
		Memory::FreeRingBuffer(&State.TransferState.TransferMemory);
	}

	void Transfer()
	{
		u64 FrameTotalTransferSize = 0;
		const u32 CurrentFrame = State.TransferState.CurrentFrame;
		VkDevice Device = VulkanInterface::GetDevice();

		VkFence TransferFence = State.TransferState.Frames.Fences[CurrentFrame];
		VkCommandBuffer TransferCommandBuffer = State.TransferState.Frames.CommandBuffers[CurrentFrame];

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX));
		VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &TransferFence));

		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo));

		std::unique_lock<std::mutex> PendingLock(State.TransferState.PendingTransferTasksQueue.Mutex);
		State.TransferState.PendingCv.wait(PendingLock, []
		{
			return !Memory::IsRingBufferEmpty(&State.TransferState.PendingTransferTasksQueue.TasksBuffer);
		});

		while (!Memory::IsRingBufferEmpty(&State.TransferState.PendingTransferTasksQueue.TasksBuffer))
		{
			TransferTask* Task = Memory::RingBufferGetFirst(&State.TransferState.PendingTransferTasksQueue.TasksBuffer);

			const u64 TransferOffset = Memory::RingAlloc(&State.TransferState.TransferStagingPool.ControlBlock, Task->DataSize);
			VulkanHelper::UpdateHostCompatibleBufferMemory(Device, State.TransferState.TransferStagingPool.Memory, Task->DataSize,
				TransferOffset, Task->RawData);

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

					VkBufferCopy IndexBufferCopyRegion = { };
					IndexBufferCopyRegion.srcOffset = TransferOffset;
					IndexBufferCopyRegion.dstOffset = Task->DataDescr.DstOffset;
					IndexBufferCopyRegion.size = Task->DataSize;

					vkCmdPipelineBarrier2(TransferCommandBuffer, &DepInfo);
					vkCmdCopyBuffer(TransferCommandBuffer, State.TransferState.TransferStagingPool.Buffer, Task->DataDescr.DstBuffer, 1, &IndexBufferCopyRegion);

					break;
				}
				case TransferTaskType::TransferTaskType_Material:
				{
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
					vkCmdCopyBuffer(TransferCommandBuffer, State.TransferState.TransferStagingPool.Buffer, Task->DataDescr.DstBuffer, 1, &IndexBufferCopyRegion);

					break;
				}
				case TransferTaskType::TransferTaskType_Texture:
				{
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
					vkCmdCopyBufferToImage(TransferCommandBuffer, State.TransferState.TransferStagingPool.Buffer,
						Task->TextureDescr.DstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);
					vkCmdPipelineBarrier2(TransferCommandBuffer, &PresentDepInfo);

					break;
				}
			}

			std::unique_lock ActiveTasksLock(State.TransferState.ActiveTransferTasksQueue.Mutex);
			Memory::PushToRingBuffer(&State.TransferState.ActiveTransferTasksQueue.TasksBuffer, Task);
			++State.TransferState.TasksInFly;
			ActiveTasksLock.unlock();

			Memory::RingBufferPopFirst(&State.TransferState.PendingTransferTasksQueue.TasksBuffer);

			FrameTotalTransferSize += Task->DataSize;
			if (FrameTotalTransferSize >= MaxTransferSizePerFrame)
			{
				break;
			}
		}

		PendingLock.unlock();

		VULKAN_CHECK_RESULT(vkEndCommandBuffer(TransferCommandBuffer));

		const u64 TasksInFly = State.TransferState.TasksInFly;

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
		SubmitInfo.pSignalSemaphores = &State.TransferState.TransferSemaphore;

		std::unique_lock SubmitLock(State.QueueSubmitMutex);
		vkQueueSubmit(VulkanInterface::GetTransferQueue(), 1, &SubmitInfo, TransferFence);
		SubmitLock.unlock();

		State.TransferState.CurrentFrame = (CurrentFrame + 1) % VulkanInterface::MAX_DRAW_FRAMES;
	}

	void Draw(const FrameManager::ViewProjectionBuffer* Data)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		const u32 ImageIndex = VulkanInterface::AcquireNextImageIndex();

		FrameManager::UpdateViewProjection(Data);

		VulkanInterface::BeginFrame();

		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		u64 CompletedCounter = 0;
		vkGetSemaphoreCounterValue(Device, State.TransferState.TransferSemaphore, &CompletedCounter);

		u64 Counter = CompletedCounter - State.TransferState.CompletedTransfer;
		if (Counter > 0)
		{
			std::unique_lock Lock(State.TransferState.ActiveTransferTasksQueue.Mutex);

			while (Counter--)
			{
				TransferTask* Task = Memory::RingBufferGetFirst(&State.TransferState.ActiveTransferTasksQueue.TasksBuffer);

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

						State.Meshes.StaticMeshes.Data[Task->ResourceIndex].IsLoaded = true;
						Memory::RingFree(&State.TransferState.TransferMemory.ControlBlock, Task->DataSize);

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

						State.Materials.Materials.Data[Task->ResourceIndex].IsLoaded = true;

						break;
					}
					case TransferTaskType::TransferTaskType_Texture:
					{
						State.Textures.Textures[Task->ResourceIndex].IsLoaded = true;
						Memory::RingFree(&State.TransferState.TransferMemory.ControlBlock, Task->DataSize);

						break;
					}
				}

				Memory::RingBufferPopFirst(&State.TransferState.ActiveTransferTasksQueue.TasksBuffer);
			}

			State.TransferState.CompletedTransfer = CompletedCounter;
		}

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

	void AddTask(TransferTask* Task)
	{
		std::unique_lock<std::mutex> PendingLock(State.TransferState.PendingTransferTasksQueue.Mutex);
		Memory::PushToRingBuffer(&State.TransferState.PendingTransferTasksQueue.TasksBuffer, Task);
	}

	void NotifyTransfer()
	{
		State.TransferState.PendingCv.notify_one();
	}

	u32 CreateEntity(const DrawEntity* Entity)
	{
		Memory::PushBackToArray(&State.DrawEntities, Entity);
		return 0;
	}

	u32 CreateMaterial(Material* Mat)
	{
		Mat->IsLoaded = false;
		const u64 Index = State.Materials.Materials.Count;

		Material* NewMaterial = State.Materials.Materials.Data + State.Materials.Materials.Count;
		Memory::PushBackToArray(&State.Materials.Materials, Mat);

		TransferTask Task = { };
		//Task.DataSize = sizeof(Material);
		Task.DataSize = 12;
		Task.DataDescr.DstBuffer = State.Materials.MaterialBuffer;
		//Task.DataDescr.DstOffset = sizeof(Material) * Index;
		Task.DataDescr.DstOffset = 12 * Index;
		Task.RawData = NewMaterial;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Material;

		AddTask(&Task);

		return Index;
	}

	u64 CreateStaticMesh(void* MeshVertexData, u64 VertexSize, u64 VerticesCount, u64 IndicesCount)
	{
		const u64 VerticesSize = VertexSize * VerticesCount;
		const u64 DataSize = sizeof(u32) * IndicesCount + VerticesSize;

		// TODO: TMP solution
		void* TransferMemory = Render::RequestTransferMemory(DataSize);
		memcpy(TransferMemory, MeshVertexData, DataSize);

		const u64 Index = State.Meshes.StaticMeshes.Count;
		StaticMesh* Mesh = Memory::ArrayGetNew(&State.Meshes.StaticMeshes);
		*Mesh = { };
		Mesh->IndicesCount = IndicesCount;
		Mesh->VertexOffset = State.Meshes.VertexStageData.Offset;
		Mesh->IndexOffset = State.Meshes.VertexStageData.Offset + VerticesSize;
		Mesh->VertexDataSize = DataSize;

		TransferTask Task = { };
		Task.DataSize = DataSize;
		Task.DataDescr.DstBuffer = State.Meshes.VertexStageData.Buffer;
		Task.DataDescr.DstOffset = State.Meshes.VertexStageData.Offset;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = Index;
		Task.Type = TransferTaskType::TransferTaskType_Mesh;

		AddTask(&Task);

		State.Meshes.VertexStageData.Offset += DataSize;
		return Index;
	}

	u32 CreateTexture2DSRGB(u64 Hash, void* Data, u32 Width, u32 Height)
	{
		assert(State.Textures.TextureIndex < 512);

		VkDevice Device = VulkanInterface::GetDevice();
		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkQueue TransferQueue = VulkanInterface::GetTransferQueue();
		MeshTexture2D* NextTexture = State.Textures.Textures + State.Textures.TextureIndex;
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

		// TODO: TMP solution
		void* TransferMemory = Render::RequestTransferMemory(NextTexture->MeshTexture.Size);
		memcpy(TransferMemory, Data, NextTexture->MeshTexture.Size);

		TransferTask Task = { };
		Task.DataSize = NextTexture->MeshTexture.Size;
		Task.TextureDescr.DstImage = NextTexture->MeshTexture.Image;
		Task.TextureDescr.Width = Width;
		Task.TextureDescr.Height = Height;
		Task.RawData = TransferMemory;
		Task.ResourceIndex = State.Textures.TextureIndex;
		Task.Type = TransferTaskType::TransferTaskType_Texture;

		AddTask(&Task);

		State.Textures.TexturesPhysicalIndexes[Hash] = State.Textures.TextureIndex;
		return State.Textures.TextureIndex++;
	}

	void* RequestTransferMemory(u64 Size)
	{
		return State.TransferState.TransferMemory.DataArray + Memory::RingAlloc(&State.TransferState.TransferMemory.ControlBlock, Size);
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