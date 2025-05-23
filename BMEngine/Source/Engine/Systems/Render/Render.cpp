#include "Render.h"

#include "Engine/Systems/Render/TerrainRender.h"
#include "Engine/Systems/Render/StaticMeshRender.h"
#include "Engine/Systems/Render/RenderResources.h"
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


	static VkBuffer MaterialBuffer;
	static VkDeviceMemory MaterialBufferMemory;
	static VkDescriptorSetLayout MaterialLayout;
	static VkDescriptorSet MaterialSet;
	static u32 MaterialIndex;

	struct StagingPool
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		VkDeviceSize Size;
		VkDeviceSize Offset;
		VkDeviceSize Alignment;
	};

	struct ImageCopyData
	{
		VkBufferImageCopy BufferImageCopyInfo;
		VkImage Image;
	};

	Memory::DynamicArray<VkImageMemoryBarrier2> ImageTransferring;
	Memory::DynamicArray<VkImageMemoryBarrier2> ImagePresenting;
	Memory::DynamicArray<ImageCopyData> ImageCopy;

	static StagingPool TransferStagingPool;
	static VkSemaphore TransferCompleted;
	static u64 TransferTimelineValue = 0;
	

	static void PrepareImageForTransferring(VkImage Image);
	static void PrepareImageForPresentation(VkImage Image);

	static u64 GetOffsetFromStagingPool(StagingPool* Pool, u64 Size);

	u64 GetOffsetFromStagingPool(StagingPool* Pool, u64 Size)
	{
		u64 AlignedOffset = (Pool->Offset + Pool->Alignment - 1) & ~(Pool->Alignment - 1);
		Pool->Offset = AlignedOffset + Size;
		return AlignedOffset;
	}

	bool PushTransferTask(TaskQueue* queue, const TransferTask* NewTask)
	{
		if (queue->Count >= MaxTasks)
		{
			assert(false);
			return false;
		}

		queue->Tasks[queue->Tail] = *NewTask;
		queue->Tail = (queue->Tail + 1) % MaxTasks;
		queue->Count++;

		return true;
	}

	void ProcessCompletedTransferTasks(VkDevice Device, TaskQueue* Queue, VkFence Fence)
	{
		while (Queue->Count > 0)
		{
			std::scoped_lock Lock(Queue->Mutex);

			TransferTask* Task = &Queue->Tasks[Queue->Head];
			if (vkGetFenceStatus(Device, Fence) == VK_SUCCESS)
			{
				Task->State = TRANSFER_COMPLETE;

				Memory::PushBackToArray(&State.DrawEntities, &Task->Entity);

				Queue->Head = (Queue->Head + 1) % MaxTasks;
				Queue->Count--;
			}
			else
			{
				break;
			}
		}
	}

	void Transfer(u32 FrameIndex)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkDependencyInfo TransferDepInfo = { };
		TransferDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		TransferDepInfo.pNext = nullptr;
		TransferDepInfo.dependencyFlags = 0;
		TransferDepInfo.imageMemoryBarrierCount = ImageTransferring.Count;
		TransferDepInfo.pImageMemoryBarriers = ImageTransferring.Data;
		TransferDepInfo.memoryBarrierCount = 0;
		TransferDepInfo.pMemoryBarriers = nullptr;
		TransferDepInfo.bufferMemoryBarrierCount = 0;
		TransferDepInfo.pBufferMemoryBarriers = nullptr;

		VkDependencyInfo PresentDepInfo = { };
		PresentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		PresentDepInfo.pNext = nullptr;
		PresentDepInfo.dependencyFlags = 0;
		PresentDepInfo.imageMemoryBarrierCount = ImagePresenting.Count;
		PresentDepInfo.pImageMemoryBarriers = ImagePresenting.Data;
		PresentDepInfo.memoryBarrierCount = 0;
		PresentDepInfo.pMemoryBarriers = nullptr;
		PresentDepInfo.bufferMemoryBarrierCount = 0;
		PresentDepInfo.pBufferMemoryBarriers = nullptr;

		VkFence TransferFence = State.TransferState.Frames.Fences[FrameIndex];
		VkCommandBuffer TransferCommandBuffer = State.TransferState.Frames.CommandBuffers[FrameIndex];
		vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX);
		vkResetFences(Device, 1, &TransferFence);

		VkTimelineSemaphoreSubmitInfo TimelineInfo = { };
		TimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		TimelineInfo.waitSemaphoreValueCount = 0;
		TimelineInfo.signalSemaphoreValueCount = 1;
		TimelineInfo.pSignalSemaphoreValues = &TransferTimelineValue;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &TransferCompleted;
		SubmitInfo.pNext = &TimelineInfo;

		VkResult Result = vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			Util::RenderLog(Util::BMRVkLogType_Error, "vkBeginCommandBuffer result is %d", Result);
		}

		vkCmdPipelineBarrier2(TransferCommandBuffer, &TransferDepInfo);

		for (u32 i = 0; i < ImageCopy.Count; ++i)
		{
			const ImageCopyData* CopyData = ImageCopy.Data + i;
			vkCmdCopyBufferToImage(TransferCommandBuffer, TransferStagingPool.Buffer, CopyData->Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyData->BufferImageCopyInfo);
		}

		vkCmdPipelineBarrier2(TransferCommandBuffer, &PresentDepInfo);

		Memory::ClearArray(&ImageTransferring);
		Memory::ClearArray(&ImagePresenting);
		Memory::ClearArray(&ImageCopy);

		Result = vkEndCommandBuffer(TransferCommandBuffer);
		if (Result != VK_SUCCESS)
		{
			Util::RenderLog(Util::BMRVkLogType_Error, "vkBeginCommandBuffer result is %d", Result);
		}

		{
			std::scoped_lock lock(State.QueueSubmitMutex);

			TransferTimelineValue++;
			vkQueueSubmit(VulkanInterface::GetTransferQueue(), 1, &SubmitInfo, TransferFence); // TODO Check GetTransferQueue
		}
	}

	void QueueImageDataLoad(VkImage Image, u32 Width, u32 Height, void* Data)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device, Image, &MemoryRequirements); // todo do not use vkGetImageMemoryRequirements

		const u64 TransferOffset = GetOffsetFromStagingPool(&TransferStagingPool, MemoryRequirements.size);

		ImageCopyData* ImageRegion = Memory::ArrayGetNew(&ImageCopy);
		*ImageRegion = { };
		ImageRegion->BufferImageCopyInfo.bufferOffset = TransferOffset;
		ImageRegion->BufferImageCopyInfo.bufferRowLength = 0;
		ImageRegion->BufferImageCopyInfo.bufferImageHeight = 0;
		ImageRegion->BufferImageCopyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageRegion->BufferImageCopyInfo.imageSubresource.mipLevel = 0;
		ImageRegion->BufferImageCopyInfo.imageSubresource.baseArrayLayer = 0;
		ImageRegion->BufferImageCopyInfo.imageSubresource.layerCount = 1;
		ImageRegion->BufferImageCopyInfo.imageOffset = { 0, 0, 0 };
		ImageRegion->BufferImageCopyInfo.imageExtent = { Width, Height, 1 };
		ImageRegion->Image = Image;

		VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferStagingPool.Memory, MemoryRequirements.size, TransferOffset, Data);
		PrepareImageForTransferring(Image);
		PrepareImageForPresentation(Image);
	}

	void PrepareImageForTransferring(VkImage Image)
	{
		VkImageMemoryBarrier2* TransferImageBarrier = Memory::ArrayGetNew(&ImageTransferring);
		*TransferImageBarrier = { };
		TransferImageBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		TransferImageBarrier->pNext = nullptr;
		TransferImageBarrier->srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		TransferImageBarrier->srcAccessMask = 0;
		TransferImageBarrier->dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		TransferImageBarrier->dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		TransferImageBarrier->oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		TransferImageBarrier->newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		TransferImageBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		TransferImageBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		TransferImageBarrier->image = Image;
		TransferImageBarrier->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		TransferImageBarrier->subresourceRange.baseMipLevel = 0;
		TransferImageBarrier->subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		TransferImageBarrier->subresourceRange.baseArrayLayer = 0;
		TransferImageBarrier->subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	}

	void PrepareImageForPresentation(VkImage Image)
	{
		VkImageMemoryBarrier2* PresentationBarrier = Memory::ArrayGetNew(&ImagePresenting);
		*PresentationBarrier = { };
		PresentationBarrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		PresentationBarrier->pNext = nullptr;
		PresentationBarrier->srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		PresentationBarrier->srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		PresentationBarrier->dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		PresentationBarrier->dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		PresentationBarrier->oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		PresentationBarrier->newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		PresentationBarrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		PresentationBarrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		PresentationBarrier->image = Image;
		PresentationBarrier->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		PresentationBarrier->subresourceRange.baseMipLevel = 0;
		PresentationBarrier->subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		PresentationBarrier->subresourceRange.baseArrayLayer = 0;
		PresentationBarrier->subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	}








	u64 TmpGetTransferTimelineValue()
	{
		return TransferTimelineValue;
	}

	VkSemaphore TmpGetTransferCompleted()
	{
		return TransferCompleted;
	}




	VkDescriptorSetLayout TmpGetMaterialLayout()
	{
		return MaterialLayout;
	}

	VkDescriptorSet TmpGetMaterialSet()
	{
		return MaterialSet;
	}



	u32 CreateEntity(const DrawEntity* Entity, u32 FrameIndex)
	{
		std::scoped_lock lock(State.TransferState.Frames.Tasks[FrameIndex].Mutex);

		TransferTask Task = { };
		Task.State = TransferTaskState::TRANSFER_IN_FLIGHT;
		Task.Entity = *Entity;
		PushTransferTask(State.TransferState.Frames.Tasks + FrameIndex, &Task);

		return 0;
	}



	

	static void InitStaticSceneBuffer(VkDevice Device, VkPhysicalDevice PhysicalDevice, StaticMeshStorage* Buffer, u64 VertexCapacity, u64 IndexCapacity);

	void InitStaticSceneBuffer(VkDevice Device, VkPhysicalDevice PhysicalDevice, StaticMeshStorage* Buffer, u64 VertexCapacity, u64 IndexCapacity)
	{
		Buffer->StaticMeshes = Memory::AllocateArray<StaticMesh>(256);

		Buffer->IndexBuffer.Offset = 0;
		Buffer->VertexBuffer.Offset = 0;

		Buffer->VertexBuffer.Buffer = VulkanHelper::CreateBuffer(Device, VertexCapacity, VulkanHelper::BufferUsageFlag::VertexFlag);
		Buffer->VertexBuffer.Memory = VulkanHelper::AllocateDeviceMemoryForBuffer(PhysicalDevice, Device, Buffer->VertexBuffer.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal, &Buffer->VertexBuffer.Alignment, &Buffer->VertexBuffer.Capacity);
		vkBindBufferMemory(Device, Buffer->VertexBuffer.Buffer, Buffer->VertexBuffer.Memory, 0);

		Buffer->IndexBuffer.Buffer = VulkanHelper::CreateBuffer(Device, VertexCapacity, VulkanHelper::BufferUsageFlag::IndexFlag);
		Buffer->IndexBuffer.Memory = VulkanHelper::AllocateDeviceMemoryForBuffer(PhysicalDevice, Device, Buffer->IndexBuffer.Buffer,
			VulkanHelper::MemoryPropertyFlag::GPULocal, &Buffer->IndexBuffer.Alignment, &Buffer->IndexBuffer.Capacity);
		vkBindBufferMemory(Device, Buffer->IndexBuffer.Buffer, Buffer->IndexBuffer.Memory, 0);

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		//VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, &Buffer->CPUArrayLock));
	}

	void Init(GLFWwindow* WindowHandler, DebugUi::GuiData* GuiData)
	{
		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VulkanInterface::VulkanInterfaceConfig RenderConfig;
		//RenderConfig.MaxTextures = 90;
		RenderConfig.MaxTextures = 500; // TODO: FIX!!!!

		VulkanInterface::Init(WindowHandler, RenderConfig);
		








		ImageTransferring = Memory::AllocateArray<VkImageMemoryBarrier2>(512);
		ImagePresenting = Memory::AllocateArray<VkImageMemoryBarrier2>(512);
		ImageCopy = Memory::AllocateArray<ImageCopyData>(512);

		State.DrawEntities = Memory::AllocateArray<DrawEntity>(512);

		VkPhysicalDevice PhysicalDevice = VulkanInterface::GetPhysicalDevice();
		VkDevice Device = VulkanInterface::GetDevice();

		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = VulkanInterface::GetQueueGraphicsFamilyIndex();

		vkCreateCommandPool(Device, &PoolInfo, nullptr, &State.TransferState.TransferCommandPool);

		VkCommandBufferAllocateInfo TransferCommandBufferAllocateInfo = { };
		TransferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		TransferCommandBufferAllocateInfo.commandPool = State.TransferState.TransferCommandPool;
		TransferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		TransferCommandBufferAllocateInfo.commandBufferCount = VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT;
		
		VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &TransferCommandBufferAllocateInfo, State.TransferState.Frames.CommandBuffers));

		for (u32 i = 0; i < VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, State.TransferState.Frames.Fences + i));
		}

		

		VkSemaphoreTypeCreateInfo TypeInfo = { };
		TypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		TypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		TypeInfo.initialValue = 0;


		VkSemaphoreCreateInfo TimeLineSemaphoreInfo = { };
		TimeLineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		TimeLineSemaphoreInfo.pNext = &TypeInfo;

		if (vkCreateSemaphore(Device, &TimeLineSemaphoreInfo, nullptr, &TransferCompleted) != VK_SUCCESS)
		{
			Util::RenderLog(Util::BMRVkLogType_Error, "TransferCompleted creation error");
			assert(false);
		}

		TransferStagingPool.Buffer = VulkanHelper::CreateBuffer(Device, MB32, VulkanHelper::StagingFlag);
		TransferStagingPool.Memory = VulkanHelper::AllocateDeviceMemoryForBuffer(PhysicalDevice, Device, TransferStagingPool.Buffer, VulkanHelper::HostCompatible,
			&TransferStagingPool.Alignment, &TransferStagingPool.Size);
		TransferStagingPool.Offset = 0;
		vkBindBufferMemory(Device, TransferStagingPool.Buffer, TransferStagingPool.Memory, 0);










		u64 Size;
		u64 Alignment;

		const VkDeviceSize MaterialBufferSize = sizeof(Material) * 512;
		MaterialBuffer = VulkanHelper::CreateBuffer(Device, MaterialBufferSize, VulkanHelper::BufferUsageFlag::StorageFlag);
		MaterialBufferMemory = VulkanHelper::AllocateDeviceMemoryForBuffer(VulkanInterface::GetPhysicalDevice(), Device,
			MaterialBuffer, VulkanHelper::MemoryPropertyFlag::GPULocal, &Size, &Alignment);
		vkBindBufferMemory(Device, MaterialBuffer, MaterialBufferMemory, 0);

		VulkanInterface::UniformSetAttachmentInfo MaterialAttachmentInfo;
		MaterialAttachmentInfo.BufferInfo.buffer = MaterialBuffer;
		MaterialAttachmentInfo.BufferInfo.offset = 0;
		MaterialAttachmentInfo.BufferInfo.range = MaterialBufferSize;
		MaterialAttachmentInfo.Type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		const VkDescriptorType MaterialDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		const VkShaderStageFlags MaterialStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		const VkDescriptorBindingFlags MaterialBindingFlags[1] = { };
		MaterialLayout = VulkanInterface::CreateUniformLayout(&MaterialDescriptorType, &MaterialStageFlags, MaterialBindingFlags, 1, 1);
		VulkanInterface::CreateUniformSets(&MaterialLayout, 1, &MaterialSet);
		VulkanInterface::AttachUniformsToSet(MaterialSet, &MaterialAttachmentInfo, 1);




		RenderResources::Init(MB32, MB32, 512, 256);
		FrameManager::Init();

		DeferredPass::Init();
		MainPass::Init();
		LightningPass::Init();

		//TerrainRender::Init();
		//DynamicMapSystem::Init();
		StaticMeshRender::Init();
		//DebugUi::Init(WindowHandler, GuiData);


		VkPhysicalDevice PhDevice = VulkanInterface::GetPhysicalDevice();

		InitStaticSceneBuffer(Device, PhDevice, &State.StaticMeshLinearStorage, MB32, MB32);



		State.TexturesLoad.Textures = Memory::AllocateArray<TextureData*>(512);
		State.TextureReady.Textures = Memory::AllocateArray<Texture>(512);

		VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, &State.TexturesLoad.Fence));
		VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, &State.TextureReady.Fence));
	}

	void DeInit()
	{
		VulkanInterface::WaitDevice();

		VkDevice Device = VulkanInterface::GetDevice();

		vkDestroyFence(Device, State.TexturesLoad.Fence, nullptr);
		vkDestroyFence(Device, State.TextureReady.Fence, nullptr);

		vkDestroyBuffer(Device, State.StaticMeshLinearStorage.IndexBuffer.Buffer, nullptr);
		vkDestroyBuffer(Device, State.StaticMeshLinearStorage.VertexBuffer.Buffer, nullptr);

		vkFreeMemory(Device, State.StaticMeshLinearStorage.VertexBuffer.Memory, nullptr);
		vkFreeMemory(Device, State.StaticMeshLinearStorage.IndexBuffer.Memory, nullptr);

		//DynamicMapSystem::DeInit();
		







		vkDestroyBuffer(Device, TransferStagingPool.Buffer, nullptr);
		vkFreeMemory(Device, TransferStagingPool.Memory, nullptr);
		vkDestroySemaphore(Device, TransferCompleted, nullptr);

		vkDestroyCommandPool(Device, State.TransferState.TransferCommandPool, nullptr);

		for (u32 i = 0; i < VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			vkDestroyFence(Device, State.TransferState.Frames.Fences[i], nullptr);
		}

		Memory::FreeArray(&ImageTransferring);
		Memory::FreeArray(&ImagePresenting);
		Memory::FreeArray(&ImageCopy);
		Memory::FreeArray(&State.DrawEntities);













		vkDestroyDescriptorSetLayout(Device, MaterialLayout, nullptr);
		vkDestroyBuffer(Device, MaterialBuffer, nullptr);
		vkFreeMemory(Device, MaterialBufferMemory, nullptr);




		//DebugUi::DeInit();
		//TerrainRender::DeInit();
		StaticMeshRender::DeInit();

		RenderResources::DeInit();
		MainPass::DeInit();
		LightningPass::DeInit();
		DeferredPass::DeInit();
		FrameManager::DeInit();
		VulkanInterface::DeInit();

		Memory::FreeArray(&State.TexturesLoad.Textures);
		Memory::FreeArray(&State.TextureReady.Textures);
		Memory::FreeArray(&State.StaticMeshLinearStorage.StaticMeshes);
	}

	void SubmitTransferring(VkCommandBuffer TransferCommandBuffer, VkFence TransferFence)
	{
		VkTimelineSemaphoreSubmitInfo TimelineInfo = { };
		TimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		TimelineInfo.waitSemaphoreValueCount = 0;
		TimelineInfo.signalSemaphoreValueCount = 1;
		TimelineInfo.pSignalSemaphoreValues = &TransferTimelineValue;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &TransferCompleted;
		SubmitInfo.pNext = &TimelineInfo;

		std::scoped_lock lock(State.QueueSubmitMutex);
		TransferTimelineValue++;
		vkQueueSubmit(VulkanInterface::GetTransferQueue(), 1, &SubmitInfo, TransferFence);
	}

	u32 CreateMaterial(Material* Mat, u32 FrameIndex)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		const u64 TransferOffset = GetOffsetFromStagingPool(&TransferStagingPool, sizeof(Material));

		VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferStagingPool.Memory, sizeof(Material), TransferOffset, Mat);

		VkBufferCopy BufferCopyRegion = { };
		BufferCopyRegion.srcOffset = TransferOffset;
		BufferCopyRegion.dstOffset = sizeof(Material) * MaterialIndex;
		BufferCopyRegion.size = sizeof(Material);

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkFence TransferFence = State.TransferState.Frames.Fences[FrameIndex];
		VkCommandBuffer TransferCommandBuffer = State.TransferState.Frames.CommandBuffers[FrameIndex];

		std::scoped_lock Lock(State.TransferState.Frames.Tasks[FrameIndex].Mutex);

		vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX);
		vkResetFences(Device, 1, &TransferFence);

		vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo);
		vkCmdCopyBuffer(TransferCommandBuffer, TransferStagingPool.Buffer, MaterialBuffer, 1, &BufferCopyRegion);
		vkEndCommandBuffer(TransferCommandBuffer);

		SubmitTransferring(TransferCommandBuffer, TransferFence);
		return MaterialIndex++;
	}

	u64 CreateStaticMesh(const StaticMeshRender::StaticMeshVertex* Vertices, u64 VerticesCount, const u32* Indices, u64 IndicesCount, u32 FrameIndex)
	{
		VkDevice Device = VulkanInterface::GetDevice();

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		const u64 Index = State.StaticMeshLinearStorage.StaticMeshes.Count;

		StaticMesh* Mesh = Memory::ArrayGetNew(&State.StaticMeshLinearStorage.StaticMeshes);
		Mesh->IndicesCount = IndicesCount;
		Mesh->IndexOffset = State.StaticMeshLinearStorage.IndexBuffer.Offset;
		Mesh->VertexOffset = State.StaticMeshLinearStorage.VertexBuffer.Offset;

		const u64 IndicesSize = sizeof(u32) * IndicesCount;
		u64 TransferOffset = GetOffsetFromStagingPool(&TransferStagingPool, IndicesSize);

		VkBufferCopy IndexBufferCopyRegion = { };
		IndexBufferCopyRegion.srcOffset = TransferOffset;
		IndexBufferCopyRegion.dstOffset = State.StaticMeshLinearStorage.IndexBuffer.Offset;
		IndexBufferCopyRegion.size = IndicesSize;
		VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferStagingPool.Memory, IndicesSize, TransferOffset, Indices);

		const u64 VerticesSize = sizeof(StaticMeshRender::StaticMeshVertex) * VerticesCount;
		TransferOffset = GetOffsetFromStagingPool(&TransferStagingPool, VerticesSize);

		VkBufferCopy VertexBufferCopyRegion = { };
		VertexBufferCopyRegion.srcOffset = TransferOffset;
		VertexBufferCopyRegion.dstOffset = State.StaticMeshLinearStorage.VertexBuffer.Offset;
		VertexBufferCopyRegion.size = VerticesSize;
		VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferStagingPool.Memory, VerticesSize, TransferOffset, Vertices);

		State.StaticMeshLinearStorage.IndexBuffer.Offset += sizeof(u32) * IndicesCount;
		State.StaticMeshLinearStorage.VertexBuffer.Offset += sizeof(StaticMeshRender::StaticMeshVertex) * VerticesCount;

		VkFence TransferFence = State.TransferState.Frames.Fences[FrameIndex];
		VkCommandBuffer TransferCommandBuffer = State.TransferState.Frames.CommandBuffers[FrameIndex];

		std::scoped_lock Lock(State.TransferState.Frames.Tasks[FrameIndex].Mutex);
		vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX);
		vkResetFences(Device, 1, &TransferFence);

		vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo);
		vkCmdCopyBuffer(TransferCommandBuffer, TransferStagingPool.Buffer, State.StaticMeshLinearStorage.IndexBuffer.Buffer, 1, &IndexBufferCopyRegion);
		vkCmdCopyBuffer(TransferCommandBuffer, TransferStagingPool.Buffer, State.StaticMeshLinearStorage.VertexBuffer.Buffer, 1, &VertexBufferCopyRegion);
		vkEndCommandBuffer(TransferCommandBuffer);
		
		SubmitTransferring(TransferCommandBuffer, TransferFence);		
		return Index;
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

	RenderState* GetRenderState()
	{
		return &State;
	}

}