#pragma once

#include <atomic>
#include <semaphore>

#include "Util/EngineTypes.h"

namespace TaskSystem
{
	typedef void (*TaskFunction)();

	struct TaskGroup
	{
		std::atomic<int> Counter{0};
		std::counting_semaphore<> Semaphore{0};
	};

	void Init();
	void DeInit();

	void SetConcurencyEnabled(bool Enabled);
	
	void AddTask(TaskFunction Function, TaskGroup* Groups, u32 GroupsCounter);
	void WaitForGroup(TaskGroup* Groups, u32 GroupsCounter);
}