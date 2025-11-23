#pragma once

#include <atomic>
#include <mutex>

#include "RenderInterface.h"

struct CommandWorkerData
{
	BmRender_CommandPool CommandPool;
	BmRender_CommandBuffer CommandBuffer;
	BmRender_Fence Fence;
	std::atomic_bool IsLocked;
};

struct CommandSystemData
{
	BmRender_Queue GraphicsQueue;
	std::mutex QueueSubmitMutex;
	BmRender_CommandWorker Workers[16];
	u32 WorkerCount;
	u32 FreeWorkerCount;
	std::atomic<u32> OldestWorkerIndex;
};

struct DrawSystemData
{
	BmRender_Semaphore ImagesAvailable[MAX_DRAW_FRAMES];
	BmRender_Semaphore RenderFinished[MAX_DRAW_FRAMES];
	u32 CurrentFrame;
	u32 MaxFramesInFly;
	u64 WaitSemaphoreValueCount;
};

void InitCommandWorkerManager(u32 Size);
void DeinitCommandWorkerManager();
BmRender_CommandWorker CreateCommandWorkerHandle(CommandWorkerData&& Data);
void DestroyCommandWorkerHandle(BmRender_CommandWorker Handle);
CommandWorkerData* GetSubmitPoolData(BmRender_CommandWorker Handle);

void InitCommandSystem(u32 WorkerCount);
void InitDrawSystem(u32 MaxFramesInFly);

void DeInitDrawSystem();
void DeInitCommandSystem();

CommandSystemData* GetCommandSystemData();
DrawSystemData* GetDrawSystemData();

u32 GetCurrentFrameIndex();

u32 AcquireNextSwapchainImage(u32 CurrentFrame);
BmRender_CommandWorker AcquireWorker(u64 Timeout);
void StartRecording(BmRender_CommandWorker Handle);
void EndRecording(BmRender_CommandWorker Handle);
void SubmitWorker(BmRender_CommandWorker Handle, u64 WaitSemaphoreValue);
void PresentFrame(u32 FrameIndex);
u32 GetMaxFramesInFly();