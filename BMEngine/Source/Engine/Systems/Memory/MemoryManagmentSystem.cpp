#include "MemoryManagmentSystem.h"

namespace Memory
{
	FrameMemory CreateFrameMemory(u64 SpaceToAllocate)
	{
		FrameMemory Memory;
		Memory.AllocatedSpace = SpaceToAllocate;
		Memory.Base = MemoryManagementSystem::CAllocate<u8>(Memory.AllocatedSpace);
		Memory.Head = Memory.Base;

		return Memory;
	}

	void DestroyFrameMemory(FrameMemory* Memory)
	{
		MemoryManagementSystem::Free(Memory->Base);
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