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
	struct DynamicArray
	{
		T* Data;
		u64 Count;
		u64 Capacity;
	};

	template<typename T>
	struct RingBuffer
	{
		T* DataArray;
		u64 Head;
		u64 Tail;
		u64 Capacity;
		u64 Leftover;
		bool Wrapped;
	};

	template <typename T>
	static DynamicArray<T> AllocateArray(u64 count)
	{
		DynamicArray<T> Arr = { };
		Arr.Count = 0;
		Arr.Capacity = count;
		Arr.Data = MemoryManagementSystem::Allocate<T>(count);

		return Arr;
	}

	template <typename T>
	static void FreeArray(DynamicArray<T>* Array)
	{
		assert(Array->Capacity != 0);

		MemoryManagementSystem::Free(Array->Data);
		Array->Capacity = 0;
		Array->Count = 0;
	}

	template <typename T>
	static void ClearArray(DynamicArray<T>* Array)
	{
		Array->Count = 0;
	}

	template <typename T>
	static void ArrayIncreaseCapacity(DynamicArray<T>* Array)
	{
		assert(Array->Capacity != 0);

		const u64 NewCapacity = Array->Capacity + Array->Capacity / 2;
		T* newData = static_cast<T*>(realloc(Array->Data, NewCapacity * sizeof(T)));
		Array->Data = newData;
		Array->Capacity = NewCapacity;
	}

	template <typename T>
	static void PushBackToArray(DynamicArray<T>* Array, T* NewElement)
	{
		assert(Array->Capacity != 0);

		if (Array->Capacity <= Array->Count)
		{
			ArrayIncreaseCapacity(Array);
		}
		Array->Data[Array->Count++] = *NewElement;
	}

	template <typename T>
	static T* ArrayGetNew(DynamicArray<T>* Array)
	{
		assert(Array->Capacity != 0);

		if (Array->Capacity <= Array->Count)
		{
			ArrayIncreaseCapacity(Array);
		}

		return &Array->Data[Array->Count++];
	}

	template <typename T>
	static RingBuffer<T> AllocateRingBuffer(u64 Capacity)
	{
		assert(Capacity != 0);

		RingBuffer<T> Buffer = { };
		Buffer.DataArray = MemoryManagementSystem::Allocate<T>(Capacity);
		Buffer.Capacity = Capacity;
		return Buffer;
	};

	template <typename T>
	static void FreeRingBuffer(RingBuffer<T>* Buffer)
	{
		assert(Buffer->Capacity != 0);
		MemoryManagementSystem::Free(Buffer->DataArray);
		*Buffer = { };
	}

	template<typename T>
	static bool IsRingBufferEmpty(const RingBuffer<T>* Buffer)
	{
		assert(Buffer->Capacity != 0);
		return (Buffer->Head == Buffer->Tail && !Buffer->Wrapped);
	}

	template<typename T>
	static bool IsRingBufferFull(const RingBuffer<T>* Buffer)
	{
		assert(Buffer->Capacity != 0);
		return Buffer->Head == Buffer->Tail && Buffer->Wrapped;
	}

	template<typename T>
	static void PushToRingBuffer(RingBuffer<T>* Buffer, const T* NewItem)
	{
		assert(Buffer->Capacity != 0);
		assert(!IsRingBufferFull(Buffer));

		Buffer->DataArray[Buffer->Head] = *NewItem;
		Buffer->Head = (Buffer->Head + 1) % Buffer->Capacity;

		if (Buffer->Head == 0)
		{
			Buffer->Wrapped = true;
		}
	}

	template<typename T>
	static T* RingBufferGetFirst(const RingBuffer<T>* Buffer)
	{
		assert(Buffer->Capacity != 0);
		assert(!IsRingBufferEmpty(Buffer));
		return &Buffer->DataArray[Buffer->Tail];
	}

	template<typename T>
	static void RingBufferPopFirst(RingBuffer<T>* Buffer)
	{
		assert(Buffer->Capacity != 0);
		assert(!IsRingBufferEmpty(Buffer));

		Buffer->Tail = (Buffer->Tail + 1) % Buffer->Capacity;

		if (Buffer->Tail == Buffer->Head)
		{
			Buffer->Wrapped = false;
		}
	}

	template<typename T>
	static bool RingIsFit(RingBuffer<T>* Buffer, u64 Count)
	{
		assert(Buffer->Capacity != 0);
		assert(Count != 0);

		const u64 Used = !Buffer->Wrapped
			? Buffer->Head - Buffer->Tail
			: Buffer->Capacity - Buffer->Tail + Buffer->Head;

		const u64 Free = Buffer->Capacity - Used;
		if (Free < Count)
		{
			return false;
		}

		if (!Buffer->Wrapped && Buffer->Head + Count <= Buffer->Capacity)
		{
			return true;
		}

		return Count <= Buffer->Tail;
	}

	template<typename T>
	static u64 RingUsed(RingBuffer<T>* Buffer)
	{
		assert(Buffer->Capacity != 0);

		if (!Buffer->Wrapped)
		{
			return Buffer->Head - Buffer->Tail;
		}

		return Buffer->Capacity - Buffer->Tail + Buffer->Head;
	}

	template<typename T>
	static T* RingAlloc(RingBuffer<T>* Buffer, u64 Count)
	{
		assert(Buffer->Capacity != 0);
		assert(Count != 0);
		assert(RingIsFit(Buffer, Count));

		if ((!Buffer->Wrapped && Buffer->Head + Count <= Buffer->Capacity) ||
			(Buffer->Wrapped && Buffer->Head >= Buffer->Tail && Buffer->Head + Count <= Buffer->Capacity))
		{
			T* ptr = &Buffer->DataArray[Buffer->Head];
			Buffer->Head = (Buffer->Head + Count) % Buffer->Capacity;
			if (Buffer->Head == Buffer->Tail)
			{
				Buffer->Wrapped = true;
			}

			return ptr;
		}

		if (!Buffer->Wrapped && Count <= Buffer->Tail)
		{
			Buffer->Leftover = Buffer->Capacity - Buffer->Head;
			Buffer->Wrapped = true;
			Buffer->Head = Count;
			return &Buffer->DataArray[0];
		}

		assert(false);
		return &Buffer->DataArray[0];
	}

	template<typename T>
	static void  RingFree(RingBuffer<T>* Buffer, u64 Count)
	{
		assert(Buffer->Capacity != 0);
		assert(Count != 0);
		assert(Count <= RingUsed(Buffer));

		if (Buffer->Tail + Count <= Buffer->Capacity)
		{
			Buffer->Tail = (Buffer->Tail + Count) % Buffer->Capacity;
		}
		else
		{
			Buffer->Tail = (Buffer->Tail + Count + Buffer->Leftover) % Buffer->Capacity;
			Buffer->Leftover = 0;
		}

		if (Buffer->Tail == Buffer->Head)
		{
			Buffer->Tail = 0;
			Buffer->Head = 0;
			if (Buffer->Wrapped)
			Buffer->Wrapped = false;
		}
	}
}
