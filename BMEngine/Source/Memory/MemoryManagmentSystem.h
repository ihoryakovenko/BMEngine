#pragma once

#include <cstdint>

#include "Util/Util.h"
#include "Util/EngineTypes.h"

namespace Memory
{
	class MemoryManagementSystem
	{
	public:
		static MemoryManagementSystem* Get()
		{
			static MemoryManagementSystem Instance;
			return &Instance;
		}

		static inline int AllocateCounter = 0;

		template <typename T>
		static T* Allocate(size_t Count = 1)
		{
#ifndef NDEBUG
			++AllocateCounter;
#endif
			return static_cast<T*>(std::malloc(Count * sizeof(T)));
		}

		template <typename T>
		static T* CAllocate(size_t Count = 1)
		{
#ifndef NDEBUG
			++AllocateCounter;
#endif

			return static_cast<T*>(std::calloc(Count, sizeof(T)));
		}

		static void Deallocate(void* Ptr)
		{
#ifndef NDEBUG
			if (Ptr != nullptr)
			{
				--AllocateCounter;
			}
#endif
			std::free(Ptr);
		}

		void Init(u32 InByteCount)
		{
			ByteCount = InByteCount;
			MemoryPool = CAllocate<char>(ByteCount);
			NextMemory = MemoryPool;
		}

		void DeInit()
		{
			Deallocate(MemoryPool);
		}

		template<typename T>
		inline T* FrameAlloc(u32 Count = 1)
		{
			assert(MemoryPool != nullptr);
			assert(NextMemory + sizeof(T) * Count <= MemoryPool + ByteCount);

			void* ReturnPointer = NextMemory;
			NextMemory += sizeof(T) * Count;
			return static_cast<T*>(ReturnPointer);
		}

		void FrameDealloc()
		{
			NextMemory = MemoryPool;
		}

	private:
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

		T& operator[](size_t index)
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
			Array.Pointer.Data = MemoryManagementSystem::Get()->FrameAlloc<T>(InCount);
			return Array;
		}

		T& operator[](size_t index)
		{
			return Pointer[index];
		}

		u32 Count = 0;
		FramePointer<T> Pointer;
	};
}
