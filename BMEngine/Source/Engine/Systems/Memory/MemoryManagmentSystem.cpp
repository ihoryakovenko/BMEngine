#include "MemoryManagmentSystem.h"

#include <mutex>

namespace Memory
{
	static std::mutex MemoryDebugMutex;
	static bool IsMemoryDebuggingEnabled;
	static bool IsMemoryDumpAllowed;

	static void Lock(std::mutex* Mutex)
	{
		Mutex->lock();
	}

	static void Unlock(std::mutex* Mutex)
	{
		Mutex->unlock();
	}

	void Init(bool EnableMemoryDebugging)
	{
		IsMemoryDebuggingEnabled = EnableMemoryDebugging;

		if (IsMemoryDebuggingEnabled)
		{
			f_debug_memory_init((void(*)(void*))Lock, (void(*)(void*))Unlock, &MemoryDebugMutex);
		}
	}

	void DeInit()
	{
		if (IsMemoryDebuggingEnabled)
		{
			f_debug_mem_print(0);
			f_debug_memory();
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

			f_debug_memory();
		}
	}

	void AllowMemoryDump(bool Allow)
	{
		IsMemoryDumpAllowed = Allow;
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