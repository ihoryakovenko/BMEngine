#pragma once

#include <cstdint>
#include <cassert>
#include <memory>

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
		MemoryManagementSystem::Free(Array->Data);
		Array->Capacity = 0;
		Array->Count = 0;
	}

	template <typename T>
	static DynamicArray<T> ClearArray(DynamicArray<T>* Array)
	{
		Array->Count = 0;
	}

	template <typename T>
	static void ArrayIncreaseCapacity(DynamicArray<T>* Array)
	{
		const u64 NewCapacity = Array->Capacity + Array->Capacity / 2;
		T* newData = static_cast<T*>(realloc(Array->Data, NewCapacity * sizeof(T)));
		Array->Data = newData;
		Array->Capacity = NewCapacity;
	}

	template <typename T>
	static void PushBackToArray(DynamicArray<T>* Array, T* NewElement)
	{
		if (Array->Capacity <= Array->Count)
		{
			ArrayIncreaseCapacity(Array);
		}
		Array->Data[Array->Count++] = *NewElement;
	}
}
