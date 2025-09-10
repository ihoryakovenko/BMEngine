#pragma once

#include <atomic>
#include <semaphore>

#include "Util/EngineTypes.h"

namespace TaskSystem
{
	typedef void (*TaskFunction)();

	struct TaskGroup
	{
		std::counting_semaphore<> Semaphore{0};
		u32 TasksInGroup;
	};

	void Init();
	void DeInit();

	void SetConcurencyEnabled(bool Enabled);
	
	void AddTask(TaskFunction Function, TaskGroup* Group);
	void WaitForGroup(TaskGroup* Group);
}