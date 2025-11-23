#pragma once

#include <atomic>
#include <semaphore>
#include <functional>

#include "Util/EngineTypes.h"

namespace TaskSystem
{
	typedef std::function<void()> TaskLambda;

	struct TaskGroup
	{
		std::counting_semaphore<> Semaphore{0};
		u32 TasksInGroup;
	};

	void Init();
	void DeInit();

	void SetConcurencyEnabled(bool Enabled);
	
	void AddTask(TaskLambda* Function, TaskGroup* Group);
	void WaitForGroup(TaskGroup* Group);
}