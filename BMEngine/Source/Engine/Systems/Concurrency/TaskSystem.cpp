#include "TaskSystem.h"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <queue>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

namespace TaskSystem
{
	struct Task
	{
		TaskFunction Function;
		TaskGroup* Groups;
		u32 GroupsCounter;
	};

	static std::vector<std::thread> WorkerThreads;
	static std::queue<Task> TaskQueue;
	static std::mutex QueueMutex;
	static std::condition_variable QueueCondition;

	static std::atomic<bool> ConcurencyEnabled = true;
	static std::atomic<bool> ShutdownFlag = false;
	
	static u32 ThreadPoolSize = 1;
	
	static void WorkerThread()
	{
		while (!ShutdownFlag.load())
		{
			Task CurrentTask;

			{
				std::unique_lock<std::mutex> Lock(QueueMutex);
				QueueCondition.wait(Lock, [] { return !TaskQueue.empty() || ShutdownFlag; });

				if (ShutdownFlag)
				{
					break;
				}

				CurrentTask = TaskQueue.front();
				TaskQueue.pop();
			}

			CurrentTask.Function();

			for (u32 i = 0; i < CurrentTask.GroupsCounter; ++i)
			{
				if (CurrentTask.Groups[i].Counter.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					CurrentTask.Groups[i].Semaphore.release();
				}
			}
		}
	}
	
	void Init()
	{
		ThreadPoolSize = std::thread::hardware_concurrency();
		
		for (u32 i = 0; i < ThreadPoolSize; ++i)
		{
			WorkerThreads.emplace_back(WorkerThread);
		}
	}
	
	void DeInit()
	{
		ShutdownFlag.store(true);
		QueueCondition.notify_all();
		
		for (auto& Thread : WorkerThreads)
		{
			if (Thread.joinable())
			{
				Thread.join();
			}
		}
		
		WorkerThreads.clear();
		
		{
			std::lock_guard<std::mutex> QueueLock(QueueMutex);
			while (!TaskQueue.empty())
			{
				TaskQueue.pop();
			}
		}
	}

	void SetConcurencyEnabled(bool Enabled)
	{
		ConcurencyEnabled.store(Enabled, std::memory_order_relaxed);
	}
	
	void AddTask(TaskFunction Function, TaskGroup* Groups, u32 GroupsCounter)
	{
		if (!ConcurencyEnabled.load(std::memory_order_relaxed))
		{
			Function();
			return;
		}

		Task NewTask;
		NewTask.Groups = Groups;
		NewTask.Function = Function;
		NewTask.GroupsCounter = GroupsCounter;

		for (u32 i = 0; i < NewTask.GroupsCounter; ++i)
		{
			NewTask.Groups[i].Counter.fetch_add(1, std::memory_order_relaxed);
		}
				
		{
			std::lock_guard<std::mutex> Lock(QueueMutex);
			TaskQueue.push(NewTask);
		}

		QueueCondition.notify_one();
	}
	
	void WaitForGroup(TaskGroup* Groups, u32 GroupsCounter)
	{
		for (u32 i = 0; i < GroupsCounter; ++i)
		{
			if (Groups[i].Counter.load(std::memory_order_acquire) > 0)
			{
				Groups[i].Semaphore.acquire();
			}
		}
	}
}