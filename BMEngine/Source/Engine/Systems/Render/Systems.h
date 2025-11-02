#pragma once

#include "RenderInterface.h"
#include "RenderTypes.h"

struct CommandSystemData
{
	VkQueue GraphicsQueue;
	std::mutex QueueSubmitMutex;
	BmRender_CommandWorker Workers[16];
	u32 WorkerCount;
	u32 FreeWorkerCount;
	std::atomic<u32> OldestWorkerIndex;
};

struct DrawSystemData
{
	VkSemaphore ImagesAvailable[VulkanHelper::MAX_DRAW_FRAMES];
	VkSemaphore RenderFinished[VulkanHelper::MAX_DRAW_FRAMES];
	u32 CurrentFrame;
	u32 MaxFramesInFly;
	u64 WaitSemaphoreValueCount;

};

void InitCommandSystem(u32 WorkerCount);
void InitDrawSystem(u32 MaxFramesInFly);

void DeInitDrawSystem();

CommandSystemData* GetCommandSystemData();
DrawSystemData* GetDrawSystemData();