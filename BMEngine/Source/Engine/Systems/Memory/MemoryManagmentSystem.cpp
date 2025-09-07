#include "MemoryManagmentSystem.h"

#include <mutex>

namespace Memory
{
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
	}

	void DeInit()
	{
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

	FrameMemory CreateFrameMemory(u64 SpaceToAllocate)
	{
		FrameMemory Memory;
		Memory.AllocatedSpace = SpaceToAllocate;
		Memory.Base = (u8*)calloc(Memory.AllocatedSpace, sizeof(u8));
		Memory.Head = Memory.Base;

		return Memory;
	}

	void DestroyFrameMemory(FrameMemory* Memory)
	{
		free(Memory->Base);
	}

	void* FrameAlloc(FrameMemory* Memory, u64 Size)
	{
		assert(Memory->Head + Size <= Memory->Base + Memory->AllocatedSpace);

		void* ReturnPointer = Memory->Head;
		Memory->Head += Size;

		return ReturnPointer;
	}

	void FrameFree(FrameMemory* Memory)
	{
		Memory->Head = Memory->Base;
	}
}