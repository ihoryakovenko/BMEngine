#pragma once

#include <cstdint>

#include "Util/EngineTypes.h"

namespace Memory
{
	struct BmMemoryManagementSystem
	{
	public:
		static BmMemoryManagementSystem* Get()
		{
			static BmMemoryManagementSystem Instance;
			return &Instance;
		}

		static inline int AllocateCounter = 0;

		// TODO: HANDLE CONSTRUCTORS AND DESTRUCTORS!!!!!!!!!!!!!!
		// for (u64 i = 0; i < Count; ++i)
		// {
		//	new (memory + i) T(); // Placement new
		// }
		// 
		//for (u64 i = 0; i < Count; ++i)
		//{
		//	memory[i].~T();
		//}

		template <typename T>
		static T* Allocate(u64 Count = 1)
		{
#ifndef NDEBUG
			++AllocateCounter;
#endif
			return static_cast<T*>(std::malloc(Count * sizeof(T)));
		}

		template <typename T>
		static T* CAllocate(u64 Count = 1)
		{
#ifndef NDEBUG
			++AllocateCounter;
#endif

			return static_cast<T*>(std::calloc(Count, sizeof(T)));
		}

		static void Free(void* Ptr)
		{
#ifndef NDEBUG
			if (Ptr != nullptr)
			{
				--AllocateCounter;
			}
#endif
			std::free(Ptr);
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

		// TODO: HANDLE CONSTRUCTORS AND DESTRUCTORS!!!!!!!!!!!!!!
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
			Pointer.Data = BmMemoryManagementSystem::Get()->FrameAlloc<T>(InCount);
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

		T* Data = nullptr;
	};

	template <typename T>
	struct FrameArray
	{
		static FrameArray<T> Create(u32 InCount = 1)
		{
			FrameArray Array;
			Array.Count = InCount;
			Array.Pointer.Data = BmMemoryManagementSystem::Get()->FrameAlloc<T>(InCount);
			return Array;
		}

		T& operator[](u64 index)
		{
			return Pointer[index];
		}

		u32 Count = 0;
		FramePointer<T> Pointer;
	};
}
