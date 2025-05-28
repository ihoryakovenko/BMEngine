#pragma once

#include <cstdint>
#include <memory>

#ifndef CUSTOM_ASSERT
#include <cassert>
#endif

#include "Util/EngineTypes.h"

namespace Memory
{
	struct MemoryManagementSystem
	{
	public:
		static MemoryManagementSystem* Get()
		{
			static MemoryManagementSystem Instance;
			return &Instance;
		}

		static inline int AllocateCounter = 0;

		template <typename T>
		static T* Allocate(u64 Count = 1)
		{
#ifndef NDEBUG
			++AllocateCounter;
#endif
			return static_cast<T*>(malloc(Count * sizeof(T)));
		}

		template <typename T>
		static T* CAllocate(u64 Count = 1)
		{
#ifndef NDEBUG
			++AllocateCounter;
#endif

			return static_cast<T*>(calloc(Count, sizeof(T)));
		}

		static void Free(void* Ptr)
		{
#ifndef NDEBUG
			if (Ptr != nullptr)
			{
				--AllocateCounter;
			}
#endif
			free(Ptr);
		}

		static void Init(u32 InByteCount)
		{
			ByteCount = InByteCount;
			MemoryPool = CAllocate<char>(ByteCount);
			NextMemory = MemoryPool;
		}

		static void DeInit()
		{
			Free(MemoryPool);
		}

		template<typename T>
		static T* FrameAlloc(u32 Count = 1)
		{
			assert(MemoryPool != nullptr);
			assert(NextMemory + sizeof(T) * Count <= MemoryPool + ByteCount);

			void* ReturnPointer = NextMemory;
			NextMemory += sizeof(T) * Count;
			return static_cast<T*>(ReturnPointer);
		}

		static void FrameFree()
		{
			NextMemory = MemoryPool;
		}

		static inline u32 ByteCount = 0;
		static inline char* NextMemory = nullptr;
		static inline char* MemoryPool = nullptr;
	};

	template <typename T>
	struct FramePointer
	{
		static FramePointer<T> Create(u32 InCount = 1)
		{
			FramePointer Pointer;
			Pointer.Data = MemoryManagementSystem::Get()->FrameAlloc<T>(InCount);
			return Pointer;
		}

		T* operator->()
		{
			return Data;
		}

		T& operator*()
		{
			return *Data;
		}

		T& operator[](u64 index)
		{
			return Data[index];
		}

		T* Data;
	};

	template <typename T>
	struct FrameArray
	{
		static FrameArray<T> Create(u32 InCount = 1)
		{
			FrameArray Array;
			Array.Count = InCount;
			Array.Pointer.Data = MemoryManagementSystem::Get()->FrameAlloc<T>(InCount);
			return Array;
		}

		T& operator[](u64 index)
		{
			return Pointer[index];
		}

		u32 Count;
		FramePointer<T> Pointer;
	};

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
	static DynamicHeapArray<T> AllocateArray(u64 count)
	{
		DynamicHeapArray<T> Arr = { };
		Arr.Count = 0;
		Arr.Capacity = count;
		Arr.Data = MemoryManagementSystem::Allocate<T>(count);

		return Arr;
	}

	template <typename T>
	static void FreeArray(DynamicHeapArray<T>* Array)
	{
		assert(Array->Capacity != 0);

		MemoryManagementSystem::Free(Array->Data);
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

		const u64 NewCapacity = Array->Capacity + Array->Capacity / 2;
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
		Buffer.DataArray = MemoryManagementSystem::Allocate<T>(Capacity);
		Buffer.ControlBlock.Capacity = Capacity;
		//Buffer.ControlBlock.Alignment = 1;
		return Buffer;
	};

	template <typename T>
	static void FreeRingBuffer(HeapRingBuffer<T>* Buffer)
	{
		assert(Buffer->ControlBlock.Capacity != 0);
		MemoryManagementSystem::Free(Buffer->DataArray);
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
		Buffer->ControlBlock.Head = (Buffer->ControlBlock.Head + 1) % Buffer->ControlBlock.Capacity;

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

		Buffer->ControlBlock.Tail = (Buffer->ControlBlock.Tail + 1) % Buffer->ControlBlock.Capacity;

		if (Buffer->ControlBlock.Tail == Buffer->ControlBlock.Head)
		{
			Buffer->ControlBlock.Wrapped = false;
		}
	}

	static bool RingIsFit(const RingBufferControl* ControlBlock, u64 Count)
	{
		assert(ControlBlock->Capacity != 0);
		assert(Count != 0);

		const u64 Used = !ControlBlock->Wrapped
			? ControlBlock->Head - ControlBlock->Tail
			: ControlBlock->Capacity - ControlBlock->Tail + ControlBlock->Head;

		const u64 Free = ControlBlock->Capacity - Used;
		if (Free < Count)
		{
			return false;
		}

		if (!ControlBlock->Wrapped && ControlBlock->Head + Count <= ControlBlock->Capacity)
		{
			return true;
		}

		return Count <= ControlBlock->Tail;
	}

	static u64 RingUsed(const RingBufferControl* ControlBlock)
	{
		assert(ControlBlock->Capacity != 0);

		if (!ControlBlock->Wrapped)
		{
			return ControlBlock->Head - ControlBlock->Tail;
		}

		return ControlBlock->Capacity - ControlBlock->Tail + ControlBlock->Head;
	}

	static u64 RingAlloc(RingBufferControl* ControlBlock, u64 Count)
	{
		//Count = (Count + ControlBlock->Alignment - 1) & ~(ControlBlock->Alignment - 1);

		assert(ControlBlock->Capacity != 0);
		assert(Count != 0);
		assert(RingIsFit(ControlBlock, Count));

		if ((!ControlBlock->Wrapped && ControlBlock->Head + Count <= ControlBlock->Capacity) ||
			(ControlBlock->Wrapped && ControlBlock->Head >= ControlBlock->Tail && ControlBlock->Head + Count <= ControlBlock->Capacity))
		{
			const u64 Offset = ControlBlock->Head;
			ControlBlock->Head = (ControlBlock->Head + Count) % ControlBlock->Capacity;
			if (ControlBlock->Head == ControlBlock->Tail)
			{
				ControlBlock->Wrapped = true;
			}

			return Offset;
		}

		if (!ControlBlock->Wrapped && Count <= ControlBlock->Tail)
		{
			ControlBlock->Leftover = ControlBlock->Capacity - ControlBlock->Head;
			ControlBlock->Wrapped = true;
			ControlBlock->Head = Count;
			return 0;
		}

		assert(false);
		return 0;
	}

	static void RingFree(RingBufferControl* ControlBlock, u64 Count)
	{
		//Count = (Count + ControlBlock->Alignment - 1) & ~(ControlBlock->Alignment - 1);

		assert(ControlBlock->Capacity != 0);
		assert(Count != 0);
		assert(Count <= RingUsed(ControlBlock));

		if (ControlBlock->Tail + Count <= ControlBlock->Capacity)
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
			ControlBlock->Wrapped = false;
		}
	}
}
