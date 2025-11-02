#include "Systems.h"

#include "Handles.h"

#include "Util/Util.h"

static CommandSystemData SubmitSystem;
static DrawSystemData DrawSystem;

void InitCommandSystem(u32 WorkerCount)
{
	VulkanCoreContext::VulkanCoreContext* Context = GetCoreContext();
	CommandSystemData* CommandSystem = GetCommandSystemData();

	vkGetDeviceQueue(Context->LogicalDevice, (u32)Context->Indices.GraphicsFamily, 0, &CommandSystem->GraphicsQueue);

	CommandSystem->WorkerCount = WorkerCount;
	CommandSystem->FreeWorkerCount = WorkerCount;
	CommandSystem->OldestWorkerIndex.store(0);

	VkDevice Device = Context->LogicalDevice;
	u32 GraphicsFamily = Context->Indices.GraphicsFamily;

	VkCommandPoolCreateInfo PoolInfo = { };
	PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	PoolInfo.queueFamilyIndex = GraphicsFamily;

	VkCommandBufferAllocateInfo AllocateInfo = { };
	AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = 1;

	VkFenceCreateInfo FenceCreateInfo = { };
	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (u32 i = 0; i < WorkerCount; ++i)
	{
		CommandWorkerData WorkerData = { };
		WorkerData.IsLocked = false;

		VULKAN_CHECK_RESULT(vkCreateCommandPool(Device, &PoolInfo, GetVulkanAllocator(), &WorkerData.CommandPool));

		AllocateInfo.commandPool = WorkerData.CommandPool;
		VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device, &AllocateInfo, &WorkerData.CommandBuffer));

		VULKAN_CHECK_RESULT(vkCreateFence(Device, &FenceCreateInfo, GetVulkanAllocator(), &WorkerData.Fence));

		CommandSystem->Workers[i] = CreateCommandWorkerHandle(&WorkerData);
	}
}

void InitDrawSystem(u32 InMaxFramesInFly)
{
	VulkanCoreContext::VulkanCoreContext* Context = GetCoreContext();
	VkDevice Device = Context->LogicalDevice;

	DrawSystem.CurrentFrame = 0;
	DrawSystem.MaxFramesInFly = InMaxFramesInFly;

	VkSemaphoreCreateInfo SemaphoreCreateInfo = { };
	SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo FenceCreateInfo = { };
	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (u64 i = 0; i < BmRender_GetMaxFramesInFly(); i++)
	{
		VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreCreateInfo, GetVulkanAllocator(), &DrawSystem.ImagesAvailable[i]));
		VULKAN_CHECK_RESULT(vkCreateSemaphore(Device, &SemaphoreCreateInfo, GetVulkanAllocator(), &DrawSystem.RenderFinished[i]));
	}
}

void DeInitDrawSystem()
{
	VulkanCoreContext::VulkanCoreContext* Context = GetCoreContext();
	VkDevice Device = Context->LogicalDevice;

	for (u64 i = 0; i < BmRender_GetMaxFramesInFly(); i++)
	{
		vkDestroySemaphore(Device, DrawSystem.ImagesAvailable[i], GetVulkanAllocator());
		vkDestroySemaphore(Device, DrawSystem.RenderFinished[i], GetVulkanAllocator());
	}
}

u32 BmRender_GetMaxFramesInFly()
{
	return DrawSystem.MaxFramesInFly;
}

CommandSystemData* GetCommandSystemData()
{
	return &SubmitSystem;
}

u32 BmRender_AcquireNextSwapchainImage(u32 CurrentFrame)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	VkSwapchainKHR Swapchain = GetCoreContext()->VulkanSwapchain;

	DrawSystemData* DrawSystem = GetDrawSystemData();
	VkSemaphore ImagesAvailable = DrawSystem->ImagesAvailable[CurrentFrame];

	u32 ImageIndex;
	VULKAN_CHECK_RESULT(vkAcquireNextImageKHR(Device, Swapchain, UINT64_MAX, ImagesAvailable, nullptr, &ImageIndex));

	return ImageIndex;
}


void BmRender_StartRecording(BmRender_CommandWorker Handle)
{
	VulkanCoreContext::VulkanCoreContext* Context = GetCoreContext();
	CommandWorkerData* SubmitPool = GetSubmitPoolData(Handle);

	VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
	CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VULKAN_CHECK_RESULT(vkBeginCommandBuffer(SubmitPool->CommandBuffer, &CommandBufferBeginInfo));
}

BmRender_CommandWorker BmRender_AcquireWorker(u64 Timeout)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	CommandSystemData* CommandSystem = GetCommandSystemData();

	for (u32 i = 0; i < CommandSystem->WorkerCount; ++i)
	{
		CommandWorkerData* WorkerData = GetSubmitPoolData(CommandSystem->Workers[i]);

		const VkResult FenceStatus = vkGetFenceStatus(Device, WorkerData->Fence);
		if (FenceStatus == VK_SUCCESS && WorkerData->IsLocked.load())
		{
			WorkerData->IsLocked.store(false, std::memory_order_release);
		}

		bool Expected = false;
		if (WorkerData->IsLocked.compare_exchange_weak(Expected, true))
		{
			if (FenceStatus == VK_SUCCESS)
			{
				VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &WorkerData->Fence));
				return CommandSystem->Workers[i];
			}
			else
			{
				WorkerData->IsLocked.store(false, std::memory_order_release);
			}
		}
	}

	const u32 OldestIndex = CommandSystem->OldestWorkerIndex.load(std::memory_order_acquire);
	CommandWorkerData* OldestWorker = GetSubmitPoolData(CommandSystem->Workers[OldestIndex]);

	const VkResult WaitResult = vkWaitForFences(Device, 1, &OldestWorker->Fence, VK_TRUE, Timeout);

	if (WaitResult == VK_TIMEOUT)
	{
		return BmRender_CommandWorker{ 0 };
	}

	if (OldestWorker->IsLocked.load())
	{
		OldestWorker->IsLocked.store(false, std::memory_order_release);
	}

	bool Expected = false;
	if (!OldestWorker->IsLocked.compare_exchange_weak(Expected, true))
	{
		return BmRender_CommandWorker{ 0 };
	}

	VULKAN_CHECK_RESULT(vkResetFences(Device, 1, &OldestWorker->Fence));

	// n = Simultaneous threads
	// T(n) = Max total iterations
	// T(n) = n(n+1)/2
	u32 Current = CommandSystem->OldestWorkerIndex.load(std::memory_order_relaxed);
	u32 Next;
	do
	{
		Next = (Current + 1) % CommandSystem->WorkerCount;
	}
	while (!CommandSystem->OldestWorkerIndex.compare_exchange_weak(Current, Next, std::memory_order_release));

	return CommandSystem->Workers[OldestIndex];
}

DrawSystemData* GetDrawSystemData()
{
	return &DrawSystem;
}

u32 BmRender_GetCurrentFrameIndex()
{
	DrawSystemData* DrawSystem = GetDrawSystemData();
	return DrawSystem->CurrentFrame;
}
