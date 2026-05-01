#include "Systems.h"

#include "Util/Util.h"
#include <unordered_map>

static CommandSystemData SubmitSystem;
static DrawSystemData DrawSystem;

static std::unordered_map<u64, CommandWorkerData> CommandWorkerStorage;
static u64 NextCommandWorkerId = 1;
static BmRender_CommandPool CommandSystemCommandPool = nullptr;

void InitCommandSystem(u32 WorkerCount)
{
	InitCommandWorkerManager(WorkerCount);

	CommandSystemData* CommandSystem = GetCommandSystemData();

	CommandSystem->GraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);

	CommandSystem->WorkerCount = WorkerCount;
	CommandSystem->FreeWorkerCount = WorkerCount;
	CommandSystem->OldestWorkerIndex.store(0);

	CommandSystemCommandPool = BmRender_CreateCommandPool(BmRender_QueueType::Graphic);

	for (u32 i = 0; i < WorkerCount; ++i)
	{
		CommandWorkerData WorkerData = { };
		WorkerData.IsLocked = false;
		WorkerData.CommandPool = CommandSystemCommandPool;

		WorkerData.CommandBuffer = BmRender_AllocateCommandBuffer(CommandSystemCommandPool);

		WorkerData.Fence = BmRender_CreateFence();

		CommandSystem->Workers[i] = CreateCommandWorkerHandle(std::move(WorkerData));
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
	// Clean up all command workers
	for (auto& [Key, WorkerData] : CommandWorkerStorage)
	{
		BmRender_FreeCommandBuffer(WorkerData.CommandBuffer);
		BmRender_DestroyFence(WorkerData.Fence);
	}
	
	// Destroy command pool
	if (CommandSystemCommandPool != nullptr)
	{
		BmRender_DestroyCommandPool(CommandSystemCommandPool);
		CommandSystemCommandPool = nullptr;
	}
	
	DeinitCommandWorkerManager();
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
	BmRender_AcquireNextSwapchainImage(UINT64_MAX, DrawSystem->ImagesAvailable[CurrentFrame], nullptr, &ImageIndex);

	return ImageIndex;
}


void StartRecording(BmRender_CommandWorker Handle)
{
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
		return nullptr;
	}

	if (OldestWorker->IsLocked.load())
	{
		OldestWorker->IsLocked.store(false, std::memory_order_release);
	}

	bool Expected = false;
	if (!OldestWorker->IsLocked.compare_exchange_weak(Expected, true))
	{
		return nullptr;
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
	CommandWorkerStorage.reserve(Size);
	NextCommandWorkerId = 1;
}

void DeinitCommandWorkerManager()
{
	CommandWorkerStorage.clear();
}

BmRender_CommandWorker CreateCommandWorkerHandle(CommandWorkerData&& Data)
{
	u64 WorkerId = NextCommandWorkerId++;
	CommandWorkerData& StoredData = CommandWorkerStorage[WorkerId];
	StoredData.CommandPool = Data.CommandPool;
	StoredData.CommandBuffer = Data.CommandBuffer;
	StoredData.Fence = Data.Fence;
	StoredData.IsLocked.store(Data.IsLocked.load());
	return (BmRender_CommandWorker)(uintptr_t)WorkerId;
}

void DestroyCommandWorkerHandle(BmRender_CommandWorker Handle)
{
	u64 WorkerId = (u64)(uintptr_t)Handle;
	CommandWorkerStorage.erase(WorkerId);
}

CommandWorkerData* GetSubmitPoolData(BmRender_CommandWorker Handle)
{
	u64 WorkerId = (u64)(uintptr_t)Handle;
	auto It = CommandWorkerStorage.find(WorkerId);
	if (It != CommandWorkerStorage.end())
	{
		return &It->second;
	}
	return nullptr;
}
