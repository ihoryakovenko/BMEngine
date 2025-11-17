#include "TransferSystem.h"

#include <atomic>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
FORGE_MEMORY_DEBUG
#include <forge_memory_debugger.h>
#include "Util/Util.h"
#include "Util/Math.h"
#include "RenderInterface.h"
#include "Systems.h"

namespace TransferSystem
{
	struct StagingFramePool
	{
		BmRender_GPUBuffer Buffer;
		u64 AllocatedForFrame[MAX_DRAW_FRAMES];
	};

	struct TaskQueue
	{
		TransferTask* Memory;
		u64 Capacity;
		std::atomic<u64> Tail;
		std::atomic<u64> Middle;
		std::atomic<u64> Head;
	};

	struct TransferFrames
	{
		BmRender_Fence Fences[MAX_DRAW_FRAMES];
		BmRender_CommandBuffer CommandBuffers[MAX_DRAW_FRAMES];
	};

	struct DataTransferState
	{
		const u64 MaxTransferSizePerFrame = MB32;

		Memory::HeapRingBuffer<u8> TransferMemory;

		BmRender_CommandPool TransferCommandPool;
		StagingFramePool TransferStagingPool;

		BmRender_Semaphore TransferSemaphore;
		u64 CompletedTransfer;

		TaskQueue TransferTasksQueue;

		TransferFrames Frames;
		u32 CurrentFrame;
	};

	static bool HasPendingTasks(TaskQueue* Queue)
	{
		return Queue->Middle.load(std::memory_order_acquire) != Queue->Head.load(std::memory_order_acquire);
	}

	static bool HasCompletedTasks(TaskQueue* Queue)
	{
		return Queue->Tail.load(std::memory_order_acquire) != Queue->Middle.load(std::memory_order_acquire);
	}

	static void AddPendingTask(TaskQueue* Queue, TransferTask* Task)
	{
		u64 CurrentHead = Queue->Head.load(std::memory_order_relaxed);
		Queue->Memory[CurrentHead] = *Task;
		Queue->Head.store(Math::WrapIncrement(CurrentHead, Queue->Capacity), std::memory_order_release);
	}

	static void PopPendingTask(TaskQueue* Queue)
	{
		u64 CurrentMiddle = Queue->Middle.load(std::memory_order_relaxed);
		Queue->Middle.store(Math::WrapIncrement(CurrentMiddle, Queue->Capacity), std::memory_order_release);
	}

	static TransferTask* GetFirstPendingTask(TaskQueue* Queue)
	{
		return Queue->Memory + Queue->Middle.load(std::memory_order_acquire);
	}

	static void PopCompletedTask(TaskQueue* Queue)
	{
		u64 CurrentTail = Queue->Tail.load(std::memory_order_relaxed);
		Queue->Tail.store(Math::WrapIncrement(CurrentTail, Queue->Capacity), std::memory_order_relaxed);
	}

	static TransferTask* GetFirstCompletedTask(TaskQueue* Queue)
	{
		return Queue->Memory + Queue->Tail.load(std::memory_order_acquire);
	}

	static void ApplyStageBarrier(VkBufferMemoryBarrier2* Barrier, BmRender_PipelineSyncStage Stage)
	{
		switch (Stage)
		{
			case BmRender_PipelineSyncStage::VertexShader:
				Barrier->srcStageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
				Barrier->srcAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
				Barrier->dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				Barrier->dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				break;
			case BmRender_PipelineSyncStage::FragmentShader:
				Barrier->srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
				Barrier->srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_UNIFORM_READ_BIT;
				Barrier->dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				Barrier->dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				break;
			default:
				Barrier->srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				Barrier->srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
				Barrier->dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				Barrier->dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				break;
		}
	}

	static DataTransferState TransferState;

	u64 Transfer()
	{
		while (HasCompletedTasks(&TransferState.TransferTasksQueue))
		{
			TransferTask* Task = GetFirstCompletedTask(&TransferState.TransferTasksQueue);
			Memory::RingFree(&TransferState.TransferMemory.ControlBlock, Task->DataSize, 1);
			PopCompletedTask(&TransferState.TransferTasksQueue);
		}

		u64 TasksAdded = 0;

		if (!HasPendingTasks(&TransferState.TransferTasksQueue))
		{
			return 0;
		}

		const u32 CurrentFrame = TransferState.CurrentFrame;

		BmRender_WaitForFences(TransferState.Frames.Fences[CurrentFrame], VK_TRUE, UINT64_MAX);
		BmRender_ResetFences(TransferState.Frames.Fences[CurrentFrame]);
		BmRender_BeginCommandBuffer(TransferState.Frames.CommandBuffers[CurrentFrame]);

		VkCommandBuffer TransferCommandBuffer = (VkCommandBuffer)TransferState.Frames.CommandBuffers[CurrentFrame];

		while (HasPendingTasks(&TransferState.TransferTasksQueue))
		{
			TransferTask* Task = GetFirstPendingTask(&TransferState.TransferTasksQueue);

			const u64 AlignedSize = Math::AlignNumber(Task->DataSize, (u64)Task->Alignment);
			const u64 Head = CurrentFrame * TransferState.MaxTransferSizePerFrame;
			const u64 Offset = Head + TransferState.TransferStagingPool.AllocatedForFrame[CurrentFrame];
			const u64 AlignedOffset = Math::AlignNumber(Offset, (u64)Task->Alignment);
			const u64 RequestSize = AlignedSize + AlignedOffset - Offset;

			const u64 NewTotal = TransferState.TransferStagingPool.AllocatedForFrame[CurrentFrame] + RequestSize;
			if (NewTotal >= TransferState.MaxTransferSizePerFrame)
			{
				// TODO: End command buffers? Increase MaxTransferSizePerFrame? Remove current task?
				assert(TasksAdded != 0 && "Task->DataSize > then MaxTransferSizePerFrame");
				break;
			}

			TransferState.TransferStagingPool.AllocatedForFrame[CurrentFrame] = NewTotal;

			BmRender_UpdateHostCompatibleBuffer(TransferState.TransferStagingPool.Buffer, AlignedOffset, Task->DataSize, Task->RawData);

			switch (Task->Type)
			{
				case TaskType::Data:
				{
					const BmRender_GPUBufferBinding& Entry = Task->DataDescr.Handle;
					BmRender_GPUBufferData BufferData;
					BmRender_GetGPUBufferData(Entry.GPUBufferHandle, &BufferData);

					//BufferData.ReadyValue = TransferState.CompletedTransfer + 1; // TODO: fix

					VkBufferMemoryBarrier2 Barrier = { };
					Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
					ApplyStageBarrier(&Barrier, Task->DataDescr.StageBarrier);
					Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					Barrier.buffer = (VkBuffer)Entry.GPUBufferHandle;
					Barrier.offset = Entry.BufferOffset;
					Barrier.size = Task->DataSize;

					VkDependencyInfo DepInfo = { };
					DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
					DepInfo.bufferMemoryBarrierCount = 1;
					DepInfo.pBufferMemoryBarriers = &Barrier;

					VkBufferCopy IndexBufferCopyRegion = { };
					IndexBufferCopyRegion.srcOffset = AlignedOffset;
					IndexBufferCopyRegion.dstOffset = Entry.BufferOffset;
					IndexBufferCopyRegion.size = Task->DataSize;

					vkCmdPipelineBarrier2(TransferCommandBuffer, &DepInfo);
					vkCmdCopyBuffer(TransferCommandBuffer, (VkBuffer)TransferState.TransferStagingPool.Buffer, (VkBuffer)Entry.GPUBufferHandle, 1, &IndexBufferCopyRegion);
					
					break;
				}
				case TaskType::Image:
				{
					BmRender_ImageResource Image;
					BmRender_GetImageData(Task->TextureDescr.Handle, &Image);
					//Image.ReadyValue = TransferState.CompletedTransfer + 1; // TODO: fix

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
					TransferImageBarrier.image = (VkImage)Task->TextureDescr.Handle;
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
					ImageRegion.bufferOffset = AlignedOffset;
					ImageRegion.bufferRowLength = 0;
					ImageRegion.bufferImageHeight = 0;
					ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					ImageRegion.imageSubresource.mipLevel = 0;
					ImageRegion.imageSubresource.baseArrayLayer = 0;
					ImageRegion.imageSubresource.layerCount = 1;
					ImageRegion.imageOffset = { 0, 0, 0 };
					ImageRegion.imageExtent = { Image.Width, Image.Height, 1 };

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
					PresentationBarrier.image = (VkImage)Task->TextureDescr.Handle;
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
					vkCmdCopyBufferToImage(TransferCommandBuffer, (VkBuffer)TransferState.TransferStagingPool.Buffer,
						(VkImage)Task->TextureDescr.Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);
					vkCmdPipelineBarrier2(TransferCommandBuffer, &PresentDepInfo);

					break;
				}
			}

			PopPendingTask(&TransferState.TransferTasksQueue);
			++TasksAdded;
		}

		BmRender_EndCommandBuffer(TransferState.Frames.CommandBuffers[CurrentFrame]);

		++TransferState.CompletedTransfer;

		BmRender_TimelineSemaphoreSubmit SignalTimelineSemaphore;
		SignalTimelineSemaphore.Semaphore = TransferState.TransferSemaphore;
		SignalTimelineSemaphore.Value = TransferState.CompletedTransfer;

		BmRender_SubmitInfo SubmitInfo = { };
		SubmitInfo.CommandBuffers = &TransferState.Frames.CommandBuffers[CurrentFrame];
		SubmitInfo.CommandBufferCount = 1;
		SubmitInfo.SignalSemaphores = nullptr;
		SubmitInfo.SignalSemaphoreCount = 0;
		SubmitInfo.SignalTimelineSemaphores = &SignalTimelineSemaphore;
		SubmitInfo.SignalTimelineSemaphoreCount = 1;
		SubmitInfo.WaitDstStageFlags = nullptr;
		SubmitInfo.WaitSemaphores = nullptr;
		SubmitInfo.WaitSemaphoreCount = 0;
		SubmitInfo.WaitTimelineSemaphores = nullptr;
		SubmitInfo.WaitTimelineSemaphoreCount = 0;

		// Todo submit using queue system
		std::unique_lock SubmitLock(GetCommandSystemData()->QueueSubmitMutex);
		BmRender_QueueSubmit(GetCommandSystemData()->GraphicsQueue, 1, &SubmitInfo, TransferState.Frames.Fences[CurrentFrame]);
		SubmitLock.unlock();

		assert(TransferState.TransferStagingPool.AllocatedForFrame[CurrentFrame] <= TransferState.MaxTransferSizePerFrame);
		TransferState.TransferStagingPool.AllocatedForFrame[CurrentFrame] = 0;
		TransferState.CurrentFrame = Math::WrapIncrement(CurrentFrame, MAX_DRAW_FRAMES);

		return 1;
	}

	void Init()
	{
		TransferState.CurrentFrame = 0;

		TransferState.TransferCommandPool = BmRender_CreateCommandPool(BmRender_QueueType::Graphic);

		for (u32 i = 0; i < MAX_DRAW_FRAMES; ++i)
		{
			TransferState.Frames.Fences[i] = BmRender_CreateFence();
			TransferState.Frames.CommandBuffers[i] = BmRender_AllocateCommandBuffer(TransferState.TransferCommandPool);
		}

		TransferState.TransferSemaphore = BmRender_CreateTimelineSemaphore(0);

		TransferState.CompletedTransfer = 0;

		TransferState.TransferTasksQueue.Capacity = 1024 * 2 * 40;
		TransferState.TransferTasksQueue.Memory = (TransferTask*)calloc(TransferState.TransferTasksQueue.Capacity, sizeof(TransferTask));
		TransferState.TransferTasksQueue.Tail.store(0, std::memory_order_relaxed);
		TransferState.TransferTasksQueue.Tail.store(0, std::memory_order_relaxed);
		TransferState.TransferTasksQueue.Head.store(0, std::memory_order_relaxed);

		TransferState.TransferStagingPool = { };

		TransferState.TransferStagingPool.Buffer = BmRender_CreateStagingBuffer(TransferState.MaxTransferSizePerFrame * MAX_DRAW_FRAMES);

		TransferState.TransferMemory = Memory::AllocateRingBuffer<u8>(MB128);
	}

	void DeInit()
	{
		for (u32 i = 0; i < MAX_DRAW_FRAMES; ++i)
		{
			BmRender_FreeCommandBuffer(TransferState.Frames.CommandBuffers[i]);
			BmRender_DestroyFence(TransferState.Frames.Fences[i]);
		}

		BmRender_DestroyCommandPool(TransferState.TransferCommandPool);
		BmRender_DestroyGPUBuffer(TransferState.TransferStagingPool.Buffer);
		BmRender_DestroySemaphore(TransferState.TransferSemaphore);

		free(TransferState.TransferTasksQueue.Memory);
		Memory::FreeRingBuffer(&TransferState.TransferMemory);
	}

	TransferMemory RequestTransferMemory(u64 Size)
	{
		return TransferState.TransferMemory.DataArray + Memory::RingAlloc(&TransferState.TransferMemory.ControlBlock, Size, 1);
	}

	void AddTask(TransferTask* Task)
	{
		AddPendingTask(&TransferState.TransferTasksQueue, Task);
	}

	bool IsBufferLocked(BmRender_GPUBuffer Handle)
	{
		u64 CompletedValue = 0;
		BmRender_GetSemaphoreCounterValue(TransferState.TransferSemaphore, &CompletedValue);

		BmRender_GPUBufferData BufferData;
		BmRender_GetGPUBufferData(Handle, &BufferData);
		return false; // TODO: fix
		//return CompletedValue < BufferData.ReadyValue;
	}

	bool IsImageLocked(BmRender_Image Handle)
	{
		u64 CompletedValue = 0;
		BmRender_GetSemaphoreCounterValue(TransferState.TransferSemaphore, &CompletedValue);

		BmRender_ImageResource ImageData;
		BmRender_GetImageData(Handle, &ImageData);
		return false; // TODO: fix
		//return CompletedValue < ImageData.ReadyValue;
	}
}