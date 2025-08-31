#include "TransferSystem.h"

#include <mutex>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Util/Util.h"
#include "VulkanHelper.h"
#include "RenderResources.h"
#include "VulkanCoreContext.h"

namespace TransferSystem
{
	struct StagingPool
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		Memory::RingBufferControl ControlBlock;
	};

	struct TaskQueue
	{
		Memory::HeapRingBuffer<TransferTask> TasksBuffer;
		std::mutex Mutex;
	};

	struct TransferFrames
	{
		VkFence Fences[VulkanHelper::MAX_DRAW_FRAMES];
		VkCommandBuffer CommandBuffers[VulkanHelper::MAX_DRAW_FRAMES];
	};

	struct DataTransferState
	{
		const u64 MaxTransferSizePerFrame = MB4;

		Memory::HeapRingBuffer<u8> TransferMemory;

		VkCommandPool TransferCommandPool;
		StagingPool TransferStagingPool;

		VkSemaphore TransferSemaphore;
		u64 TasksInFly;
		u64 CompletedTransfer;

		TaskQueue PendingTransferTasksQueue;
		TaskQueue ActiveTransferTasksQueue;

		TransferFrames Frames;
		u32 CurrentFrame;
	};

	DataTransferState TransferState;

	void Transfer()
	{
		u64 FrameTotalTransferSize = 0;
		const u32 CurrentFrame = TransferState.CurrentFrame;
		VkDevice Device = VulkanInterface::GetDevice();

		VkFence TransferFence = TransferState.Frames.Fences[CurrentFrame];
		VkCommandBuffer TransferCommandBuffer = TransferState.Frames.CommandBuffers[CurrentFrame];

		{
			std::unique_lock<std::mutex> PendingLock(TransferState.PendingTransferTasksQueue.Mutex);
			if (Memory::IsRingBufferEmpty(&TransferState.PendingTransferTasksQueue.TasksBuffer))
			{
				return;
			}

			VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
			CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VULKAN_CHECK_RESULT(vkWaitForFences(Device, 1, &TransferFence, VK_TRUE, UINT64_MAX));
			VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &TransferFence));
			VULKAN_CHECK_RESULT(vkBeginCommandBuffer(TransferCommandBuffer, &CommandBufferBeginInfo));

			while (!Memory::IsRingBufferEmpty(&TransferState.PendingTransferTasksQueue.TasksBuffer))
			{
				TransferTask* Task = Memory::RingBufferGetFirst(&TransferState.PendingTransferTasksQueue.TasksBuffer);

				switch (Task->Type)
				{
					case RenderResources::ResourceType::Mesh:
					{
						const u64 TransferOffset = Memory::RingAlloc(&TransferState.TransferStagingPool.ControlBlock, Task->DataSize, 1);
						VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState.TransferStagingPool.Memory, Task->DataSize,
							TransferOffset, Task->RawData);

						VkBufferMemoryBarrier2 Barrier = { };
						Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
						Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
						Barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
						Barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
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

						vkCmdCopyBuffer(TransferCommandBuffer, TransferState.TransferStagingPool.Buffer, Task->DataDescr.DstBuffer, 1, &IndexBufferCopyRegion);
						vkCmdPipelineBarrier2(TransferCommandBuffer, &DepInfo);

						break;
					}
					case RenderResources::ResourceType::Material:
					case RenderResources::ResourceType::Instance:
					{
						const u64 TransferOffset = Memory::RingAlloc(&TransferState.TransferStagingPool.ControlBlock, Task->DataSize, 1);
						VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState.TransferStagingPool.Memory, Task->DataSize,
							TransferOffset, Task->RawData);

						VkBufferMemoryBarrier2 Barrier = { };
						Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
						Barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
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

						VkBufferCopy IndexBufferCopyRegion = { };
						IndexBufferCopyRegion.srcOffset = TransferOffset;
						IndexBufferCopyRegion.dstOffset = Task->DataDescr.DstOffset;
						IndexBufferCopyRegion.size = Task->DataSize;

						vkCmdCopyBuffer(TransferCommandBuffer, TransferState.TransferStagingPool.Buffer, Task->DataDescr.DstBuffer, 1, &IndexBufferCopyRegion);
						vkCmdPipelineBarrier2(TransferCommandBuffer, &DepInfo);

						break;
					}
					case RenderResources::ResourceType::Texture:
					{
						const u64 TransferOffset = Memory::RingAlloc(&TransferState.TransferStagingPool.ControlBlock, Task->DataSize, 16);
						VulkanHelper::UpdateHostCompatibleBufferMemory(Device, TransferState.TransferStagingPool.Memory, Task->DataSize,
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
						vkCmdCopyBufferToImage(TransferCommandBuffer, TransferState.TransferStagingPool.Buffer,
							Task->TextureDescr.DstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);
						vkCmdPipelineBarrier2(TransferCommandBuffer, &PresentDepInfo);

						break;
					}
				}

				{
					std::unique_lock ActiveTasksLock(TransferState.ActiveTransferTasksQueue.Mutex);
					Memory::PushToRingBuffer(&TransferState.ActiveTransferTasksQueue.TasksBuffer, Task);
					++TransferState.TasksInFly;
				}

				Memory::RingBufferPopFirst(&TransferState.PendingTransferTasksQueue.TasksBuffer);

				FrameTotalTransferSize += Task->DataSize;
				if (FrameTotalTransferSize >= TransferState.MaxTransferSizePerFrame)
				{
					break;
				}
			}
		}

		VULKAN_CHECK_RESULT(vkEndCommandBuffer(TransferCommandBuffer));

		const u64 TasksInFly = TransferState.TasksInFly;

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
		SubmitInfo.pSignalSemaphores = &TransferState.TransferSemaphore;

		VulkanCoreContext::VulkanCoreContext* CoreContext = RenderResources::GetCoreContext();

		// Todo submit using queue system?
		std::unique_lock SubmitLock(CoreContext->QueueSubmitMutex);
		vkQueueSubmit(VulkanInterface::GetTransferQueue(), 1, &SubmitInfo, TransferFence);
		SubmitLock.unlock();

		TransferState.CurrentFrame = (CurrentFrame + 1) % VulkanHelper::MAX_DRAW_FRAMES;
	}

	void Init()
	{
		VulkanCoreContext::VulkanCoreContext* Context = RenderResources::GetCoreContext();
		VkPhysicalDevice PhysicalDevice = Context->PhysicalDevice;
		VkDevice Device = Context->LogicalDevice;

		TransferState.CurrentFrame = 0;

		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = Context->Indices.GraphicsFamily;

		VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &PoolInfo, nullptr, &TransferState.TransferCommandPool));

		VkCommandBufferAllocateInfo TransferCommandBufferAllocateInfo = { };
		TransferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		TransferCommandBufferAllocateInfo.commandPool = TransferState.TransferCommandPool;
		TransferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		TransferCommandBufferAllocateInfo.commandBufferCount = VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT;

		VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &TransferCommandBufferAllocateInfo, TransferState.Frames.CommandBuffers));

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (u32 i = 0; i < VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, nullptr, TransferState.Frames.Fences + i));
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

		VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreInfo, nullptr, &TransferState.TransferSemaphore));
		TransferState.CompletedTransfer = 0;
		TransferState.TasksInFly = 0;

		TransferState.PendingTransferTasksQueue.TasksBuffer = Memory::AllocateRingBuffer<TransferTask>(1024 * 100);
		TransferState.ActiveTransferTasksQueue.TasksBuffer = Memory::AllocateRingBuffer<TransferTask>(1024 * 100);

		TransferState.TransferStagingPool = { };

		TransferState.TransferStagingPool.Buffer = VulkanHelper::CreateBuffer(Device, MB16, VulkanHelper::BufferUsageFlag::StagingFlag);
		VulkanHelper::DeviceMemoryAllocResult AllocResult = VulkanHelper::AllocateDeviceMemory(PhysicalDevice, Device,
			TransferState.TransferStagingPool.Buffer, VulkanHelper::MemoryPropertyFlag::HostCompatible);
		TransferState.TransferStagingPool.Memory = AllocResult.Memory;
		//TransferState.TransferStagingPool.ControlBlock.Alignment = AllocResult.Alignment;
		TransferState.TransferStagingPool.ControlBlock.Capacity = AllocResult.Size;
		VULKAN_CHECK_RESULT(vkBindBufferMemory(Device, TransferState.TransferStagingPool.Buffer, TransferState.TransferStagingPool.Memory, 0));

		TransferState.TransferMemory = Memory::AllocateRingBuffer<u8>(MB16);
	}

	void DeInit()
	{
		VulkanCoreContext::VulkanCoreContext* Context = RenderResources::GetCoreContext();
		VkDevice Device = Context->LogicalDevice;

		vkDestroyBuffer(Device, TransferState.TransferStagingPool.Buffer, nullptr);
		vkFreeMemory(Device, TransferState.TransferStagingPool.Memory, nullptr);

		vkDestroyCommandPool(Device, TransferState.TransferCommandPool, nullptr);

		for (u32 i = 0; i < VulkanCoreContext::MAX_SWAPCHAIN_IMAGES_COUNT; ++i)
		{
			vkDestroyFence(Device, TransferState.Frames.Fences[i], nullptr);
		}

		vkDestroySemaphore(Device, TransferState.TransferSemaphore, nullptr);

		Memory::FreeRingBuffer(&TransferState.PendingTransferTasksQueue.TasksBuffer);
		Memory::FreeRingBuffer(&TransferState.ActiveTransferTasksQueue.TasksBuffer);
		Memory::FreeRingBuffer(&TransferState.TransferMemory);
	}

	void ProcessTransferTasks()
	{
		VulkanCoreContext::VulkanCoreContext* Context = RenderResources::GetCoreContext();
		VkDevice Device = Context->LogicalDevice;

		u64 CompletedCounter = 0;
		vkGetSemaphoreCounterValue(Device, TransferState.TransferSemaphore, &CompletedCounter);

		u64 Counter = CompletedCounter - TransferState.CompletedTransfer;
		if (Counter > 0)
		{
			std::unique_lock Lock(TransferState.ActiveTransferTasksQueue.Mutex);

			while (Counter--)
			{
				TransferTask* Task = Memory::RingBufferGetFirst(&TransferState.ActiveTransferTasksQueue.TasksBuffer);
				RenderResources::SetResourceReadyToRender(Task->ResourceIndex);

				Memory::RingFree(&TransferState.TransferMemory.ControlBlock, Task->DataSize, 1);
				Memory::RingBufferPopFirst(&TransferState.ActiveTransferTasksQueue.TasksBuffer);
			}

			TransferState.CompletedTransfer = CompletedCounter;
		}
	}

	TransferMemory RequestTransferMemory(u64 Size)
	{
		return TransferState.TransferMemory.DataArray + Memory::RingAlloc(&TransferState.TransferMemory.ControlBlock, Size, 1);
	}

	ResourceTransferMemory RequestMemoryForResource(RenderResources::ResourceType Type, u32 ResourceCount)
	{
		//ResourceTransferMemory Memory;
		//Memory.Memory = nullptr;
		//Memory.Type = RenderResources::ResourceType::Invalid;

		//switch (Type)
		//{
		//	case RenderResources::ResourceType::Texture:
		//		break;
		//	case RenderResources::ResourceType::Mesh:
		//		break;
		//	case RenderResources::ResourceType::Material:
		//		break;
		//	case RenderResources::ResourceType::Instance:
		//		break;
		//}
		//
		//assert(Memory.Type != RenderResources::ResourceType::Invalid);

		return ResourceTransferMemory();
	}

	void AddTask(TransferTask* Task)
	{
		std::unique_lock<std::mutex> PendingLock(TransferState.PendingTransferTasksQueue.Mutex);
		Memory::PushToRingBuffer(&TransferState.PendingTransferTasksQueue.TasksBuffer, Task);
	}
}