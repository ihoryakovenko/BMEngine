#include "MemoryManagmentSystem.h"

#include <mutex>

#include <SharedLib.h>

namespace Memory
{
	static Memory_LinearAllocator GeneralFrameMemory;

	static std::recursive_mutex MemoryDebugMutex;
	static bool IsMemoryDebuggingEnabled;
	static bool IsMemoryDumpAllowed;
	static bool AreFrameMemoryChecksEnabled;

	static int Lock(std::mutex* Mutex)
	{
		Mutex->lock();
		return 0;
	}

	static int Unlock(std::mutex* Mutex)
	{
		Mutex->unlock();
		return 0;
	}

	void Init(bool EnableMemoryDebugging)
	{
		IsMemoryDebuggingEnabled = EnableMemoryDebugging;

		if (IsMemoryDebuggingEnabled)
		{
			f_debug_mem_thread_safe_init((int(*)(void*))Lock, (int(*)(void*))Unlock, &MemoryDebugMutex);
		}

		Memory_LinearAllocator_Init(&GeneralFrameMemory, 1024 * 1024);
	}

	void DeInit()
	{
		Memory_LinearAllocator_Free(&GeneralFrameMemory);

		if (IsMemoryDebuggingEnabled)
		{
			f_debug_mem_print(0);
			f_debug_mem_check_bounds();
			f_debug_mem_check_stack_reference();
			f_debug_mem_check_heap_reference(0);
		}
	}

	void Update()
	{
		if (IsMemoryDebuggingEnabled)
		{
			if (IsMemoryDumpAllowed)
			{
				f_debug_mem_print(0);
			}

			if (AreFrameMemoryChecksEnabled)
			{
				f_debug_mem_check_bounds();
				f_debug_mem_check_stack_reference();
				f_debug_mem_check_heap_reference(0);
			}
		}
	}

	void AllowFrameMemoryDump(bool Allow)
	{
		IsMemoryDumpAllowed = Allow;
	}

	void AllowFrameMemoryChecks(bool Allow)
	{
		AreFrameMemoryChecksEnabled = Allow;
	}

	Memory_LinearAllocator* GetGeneralFrameMemory()
	{
		return &GeneralFrameMemory;
	}
}