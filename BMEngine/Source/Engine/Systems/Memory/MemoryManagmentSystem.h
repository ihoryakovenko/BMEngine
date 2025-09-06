#pragma once

#include <cstdint>
#include <memory>

#ifndef CUSTOM_ASSERT
#include <cassert>
#endif

#include "Util/EngineTypes.h"
#include "Util/Math.h"

#define F_MEMORY_DEBUG
#include "forge.h"

namespace Memory
{
	struct FrameMemory
	{
		u32 AllocatedSpace;
		u8* Head;
		u8* Base;
	};

	void Init(bool EnableMemoryDebugging);
	void DeInit();
	void Update();

	void AllowMemoryDump(bool Allow);

	FrameMemory CreateFrameMemory(u64 SpaceToallocate);
	void DestroyFrameMemory(FrameMemory* Memory);

	void* FrameAlloc(FrameMemory* Memory, u64 Size);
	void FrameFree(FrameMemory* Memory);

	template <typename T>
	struct DynamicHeapArray
	{
		T* Data;
		u64 Count;
		u64 Capacity;
	};

	struct RingBufferControl
	{
		u64 Head;
		u64 Tail;
		u64 Capacity;
		u64 Leftover;
		bool Wrapped;
	};

	template <typename T>
	struct HeapRingBuffer
	{
		T* DataArray;
		RingBufferControl ControlBlock;
	};

	template <typename T>
	static DynamicHeapArray<T> AllocateArray(u64 Count)
	{
		DynamicHeapArray<T> Arr = { };
		Arr.Count = 0;
		Arr.Capacity = Count;
		Arr.Data = (T*)malloc(Count * sizeof(T));

		return Arr;
	}

	template <typename T>
	static void FreeArray(DynamicHeapArray<T>* Array)
	{
		assert(Array->Capacity != 0);

		free(Array->Data);
		Array->Capacity = 0;
		Array->Count = 0;
	}

	template <typename T>
	static void ClearArray(DynamicHeapArray<T>* Array)
	{
		Array->Count = 0;
	}

	template <typename T>
	static void ArrayIncreaseCapacity(DynamicHeapArray<T>* Array)
	{
		assert(Array->Capacity != 0);

		const u64 NewCapacity = Array->Capacity * 2;
		T* NewData = static_cast<T*>(realloc(Array->Data, NewCapacity * sizeof(T)));
		Array->Data = NewData;
		Array->Capacity = NewCapacity;
	}

	template <typename T>
	static void PushBackToArray(DynamicHeapArray<T>* Array, const T* NewElement)
	{
		assert(Array->Capacity != 0);

		if (Array->Count >= Array->Capacity)
		{
			ArrayIncreaseCapacity(Array);
		}
		Array->Data[Array->Count++] = *NewElement;
	}

	template <typename T>
	static T* ArrayGetNew(DynamicHeapArray<T>* Array)
	{
		assert(Array->Capacity != 0);

		if (Array->Count >= Array->Capacity)
		{
			ArrayIncreaseCapacity(Array);
		}

		return &Array->Data[Array->Count++];
	}

	template <typename T>
	static HeapRingBuffer<T> AllocateRingBuffer(u64 Capacity)
	{
		assert(Capacity != 0);

		HeapRingBuffer<T> Buffer = { };
		Buffer.DataArray = (T*)malloc(Capacity * sizeof(T));
		Buffer.ControlBlock.Capacity = Capacity;
		//Buffer.ControlBlock.Alignment = 1;
		return Buffer;
	};

	template <typename T>
	static void FreeRingBuffer(HeapRingBuffer<T>* Buffer)
	{
		assert(Buffer->ControlBlock.Capacity != 0);
		free(Buffer->DataArray);
		*Buffer = { };
	}

	template<typename T>
	static bool IsRingBufferEmpty(const HeapRingBuffer<T>* Buffer)
	{
		assert(Buffer->ControlBlock.Capacity != 0);
		return (Buffer->ControlBlock.Head == Buffer->ControlBlock.Tail && !Buffer->ControlBlock.Wrapped);
	}

	template<typename T>
	static bool IsRingBufferFull(const HeapRingBuffer<T>* Buffer)
	{
		assert(Buffer->ControlBlock.Capacity != 0);
		return Buffer->ControlBlock.Head == Buffer->ControlBlock.Tail && Buffer->ControlBlock.Wrapped;
	}

	template<typename T>
	static void PushToRingBuffer(HeapRingBuffer<T>* Buffer, const T* NewItem)
	{
		assert(Buffer->ControlBlock.Capacity != 0);
		assert(!IsRingBufferFull(Buffer));

		Buffer->DataArray[Buffer->ControlBlock.Head] = *NewItem;
		Buffer->ControlBlock.Head = Math::WrapIncrement(Buffer->ControlBlock.Head, Buffer->ControlBlock.Capacity);

		if (Buffer->ControlBlock.Head == 0)
		{
			Buffer->ControlBlock.Wrapped = true;
		}
	}

	template<typename T>
	static T* RingBufferGetFirst(const HeapRingBuffer<T>* Buffer)
	{
		assert(Buffer->ControlBlock.Capacity != 0);
		assert(!IsRingBufferEmpty(Buffer));
		return &Buffer->DataArray[Buffer->ControlBlock.Tail];
	}

	template<typename T>
	static void RingBufferPopFirst(HeapRingBuffer<T>* Buffer)
	{
		assert(Buffer->ControlBlock.Capacity != 0);
		assert(!IsRingBufferEmpty(Buffer));

		Buffer->ControlBlock.Tail = Math::WrapIncrement(Buffer->ControlBlock.Tail, Buffer->ControlBlock.Capacity);

		if (Buffer->ControlBlock.Tail == Buffer->ControlBlock.Head)
		{
			Buffer->ControlBlock.Wrapped = false;
		}
	}

	static bool RingIsFit(u64 Capacity, u64 Head, u64 Tail, bool Wrapped, u64 Count)
	{
		assert(Capacity != 0);
		assert(Count != 0);

		const u64 Used = !Wrapped ? Head - Tail : Capacity - Tail + Head;

		const u64 Free = Capacity - Used;
		if (Free < Count)
		{
			return false;
		}

		if (!Wrapped && Head + Count <= Capacity)
		{
			return true;
		}

		return Count <= Tail;
	}

	static u64 RingUsed(u64 Capacity, u64 Head, u64 Tail, bool Wrapped)
	{
		assert(Capacity != 0);

		if (!Wrapped)
		{
			return Head - Tail;
		}

		return Capacity - Tail + Head;
	}

	static u64 RingAlloc(RingBufferControl* ControlBlock, u64 Count, u64 HeadAlignment)
	{
		assert(ControlBlock->Capacity != 0);
		assert(Count != 0);

		const u64 AlignedHead = Math::AlignNumber(ControlBlock->Head, HeadAlignment);
		assert(RingIsFit(ControlBlock->Capacity, AlignedHead, ControlBlock->Tail, ControlBlock->Wrapped, Count));

		if ((!ControlBlock->Wrapped && AlignedHead + Count <= ControlBlock->Capacity) ||
			(ControlBlock->Wrapped && AlignedHead >= ControlBlock->Tail && ControlBlock->Head + Count <= ControlBlock->Capacity))
		{
			ControlBlock->Head = (AlignedHead + Count) % ControlBlock->Capacity;
			if (ControlBlock->Head == ControlBlock->Tail)
			{
				ControlBlock->Wrapped = true;
			}

			return AlignedHead;
		}

		ControlBlock->Leftover = ControlBlock->Capacity - ControlBlock->Head;
		ControlBlock->Wrapped = true;
		ControlBlock->Head = Count;
		return 0;
	}

	static void RingFree(RingBufferControl* ControlBlock, u64 Count, u64 HeadAlignment)
	{
		assert(ControlBlock->Capacity != 0);
		assert(Count != 0);

		const u64 AlignedTail = Math::AlignNumber(ControlBlock->Tail, HeadAlignment);
		Count += AlignedTail - ControlBlock->Tail;

		assert(Count <= RingUsed(ControlBlock->Capacity, ControlBlock->Head, ControlBlock->Tail, ControlBlock->Wrapped));

		if (AlignedTail + Count <= ControlBlock->Capacity)
		{
			ControlBlock->Tail = (ControlBlock->Tail + Count) % ControlBlock->Capacity;
		}
		else
		{
			ControlBlock->Tail = (ControlBlock->Tail + Count + ControlBlock->Leftover) % ControlBlock->Capacity;
			ControlBlock->Leftover = 0;
		}

		if (ControlBlock->Tail == ControlBlock->Head)
		{
			ControlBlock->Tail = 0;
			ControlBlock->Head = 0;
			if (ControlBlock->Wrapped)
			{
				ControlBlock->Wrapped = false;
			}
		}
	}
}
