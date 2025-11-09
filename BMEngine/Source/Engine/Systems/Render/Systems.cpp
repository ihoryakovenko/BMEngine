#include "Systems.h"

#include "Handles.h"

#include "Util/Util.h"

static CommandSystemData SubmitSystem;
static DrawSystemData DrawSystem;

static System_HandleManager CommandWorkerManager;

static void OnCommandWorkerClear(CommandWorkerData* PoolData)
{
	//BmRender_DestroyFence(PoolData->Fence);
	//BmRender_DestroyCommandPool(PoolData->CommandPool);
}

void InitCommandSystem(u32 WorkerCount)
{
	InitCommandWorkerManager(WorkerCount);

	VulkanCoreContext::VulkanCoreContext* Context = GetCoreContext();
	CommandSystemData* CommandSystem = GetCommandSystemData();

	vkGetDeviceQueue(Context->LogicalDevice, (u32)Context->Indices.GraphicsFamily, 0, &CommandSystem->GraphicsQueue);

	CommandSystem->WorkerCount = WorkerCount;
	CommandSystem->FreeWorkerCount = WorkerCount;
	CommandSystem->OldestWorkerIndex.store(0);

	VkDevice Device = Context->LogicalDevice;
	u32 GraphicsFamily = Context->Indices.GraphicsFamily;

	BmRender_CommandPool CommandPool = BmRender_CreateCommandPool(GraphicsFamily);

	CommandPoolData* PoolData = GetCommandPoolData(CommandPool);


	for (u32 i = 0; i < WorkerCount; ++i)
	{
		CommandWorkerData WorkerData = { };
		WorkerData.IsLocked = false;
		WorkerData.CommandPool = CommandPool;

		WorkerData.CommandBuffer = BmRender_AllocateCommandBuffer(CommandPool);

		WorkerData.Fence = BmRender_CreateFence();

		CommandSystem->Workers[i] = CreateCommandWorkerHandle(&WorkerData);
	}
}

void InitDrawSystem(u32 InMaxFramesInFly)
{
	DrawSystem.CurrentFrame = 0;
	DrawSystem.MaxFramesInFly = InMaxFramesInFly;

	for (u64 i = 0; i < GetMaxFramesInFly(); i++)
	{
		DrawSystem.ImagesAvailable[i] = BmRender_CreateSemaphore();
		DrawSystem.RenderFinished[i] = BmRender_CreateSemaphore();
	}
}

void DeInitDrawSystem()
{
	for (u64 i = 0; i < GetMaxFramesInFly(); i++)
	{
		BmRender_DestroySemaphore(DrawSystem.ImagesAvailable[i]);
		BmRender_DestroySemaphore(DrawSystem.RenderFinished[i]);
	}
}

void DeInitCommandSystem()
{
	DeinitCommandWorkerManager(OnCommandWorkerClear);
}

u32 GetMaxFramesInFly()
{
	return DrawSystem.MaxFramesInFly;
}

CommandSystemData* GetCommandSystemData()
{
	return &SubmitSystem;
}

u32 AcquireNextSwapchainImage(u32 CurrentFrame)
{
	DrawSystemData* DrawSystem = GetDrawSystemData();

	u32 ImageIndex;
	BmRender_AcquireNextSwapchainImage(UINT64_MAX, DrawSystem->ImagesAvailable[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

	return ImageIndex;
}


void StartRecording(BmRender_CommandWorker Handle)
{
	VulkanCoreContext::VulkanCoreContext* Context = GetCoreContext();
	CommandWorkerData* SubmitPool = GetSubmitPoolData(Handle);

	BmRender_BeginCommandBuffer(SubmitPool->CommandBuffer);
}

void EndRecording(BmRender_CommandWorker Handle)
{
	CommandWorkerData* SubmitPool = GetSubmitPoolData(Handle);
	BmRender_EndCommandBuffer(SubmitPool->CommandBuffer);
}

void SubmitWorker(BmRender_CommandWorker Handle, u64 WaitSemaphoreValue)
{

}

void PresentFrame(u32 FrameIndex)
{
}

BmRender_CommandWorker AcquireWorker(u64 Timeout)
{
	VkDevice Device = GetCoreContext()->LogicalDevice;
	CommandSystemData* CommandSystem = GetCommandSystemData();

	for (u32 i = 0; i < CommandSystem->WorkerCount; ++i)
	{
		CommandWorkerData* WorkerData = GetSubmitPoolData(CommandSystem->Workers[i]);

		const BmRender_FenceStatus FenceStatus = BmRender_GetFenceStatus(WorkerData->Fence);
		if (FenceStatus == BmRender_FenceStatus::Signaled && WorkerData->IsLocked.load())
		{
			WorkerData->IsLocked.store(false, std::memory_order_release);
		}

		bool Expected = false;
		if (WorkerData->IsLocked.compare_exchange_weak(Expected, true))
		{
			if (FenceStatus == BmRender_FenceStatus::Signaled)
			{
				BmRender_ResetFences(WorkerData->Fence);
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

	const BmRender_WaitResult WaitResult = BmRender_WaitForFences(OldestWorker->Fence, VK_TRUE, Timeout);

	if (WaitResult == BmRender_WaitResult::Timeout)
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

	BmRender_ResetFences(OldestWorker->Fence);

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

u32 GetCurrentFrameIndex()
{
	DrawSystemData* DrawSystem = GetDrawSystemData();
	return DrawSystem->CurrentFrame;
}

void InitCommandWorkerManager(u32 Size)
{
	CommandWorkerManager = System_HandleManager_InitData(Size, sizeof(CommandWorkerData), 0);
}

void DeinitCommandWorkerManager(void(*CleanUpFunc)(CommandWorkerData*))
{
	System_HandleManager_ClearData(CommandWorkerManager, (void(*)(void*))CleanUpFunc);
}

BmRender_CommandWorker CreateCommandWorkerHandle(const CommandWorkerData* Data)
{
	BmRender_CommandWorker Handle;
	Handle.Private = System_HandleManager_CreateHandle(CommandWorkerManager, Data);
	return Handle;
}

void DestroyCommandWorkerHandle(BmRender_CommandWorker Handle)
{
	System_HandleManager_DestroyHandle(CommandWorkerManager, Handle.Private);
}

CommandWorkerData* GetSubmitPoolData(BmRender_CommandWorker Handle)
{
	return (CommandWorkerData*)System_HandleManager_GetHandleData(CommandWorkerManager, Handle.Private);
}
