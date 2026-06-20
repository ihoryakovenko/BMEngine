#include "SharedLib.h"

#include <cstring>
#include <cassert>
#include <cstddef>

#include <forge_memory_debugger.h>

// Memory_PoolAllocator
static void* AlignPointer(void* Ptr, u32 Alignment)
{
	uintptr_t Addr = (uintptr_t)Ptr;
	uintptr_t AlignedAddr = (Addr + Alignment - 1) & ~(uintptr_t)(Alignment - 1);
	return (void*)AlignedAddr;
}

void Memory_PoolAllocator_Init(Memory_PoolAllocator* Allocator, u64 InitialCapacity, u32 DataSize, u32 Alignment)
{
	Allocator->FreeList = nullptr;
	Allocator->FreeCount = 0;
	Allocator->FreeCapacity = 0;
	Allocator->Count = 0;
	Allocator->capacity = InitialCapacity;
	Allocator->DataSize = DataSize;
	Allocator->Alignment = Alignment;

	u32 Stride = SHARED_LIB_ALIGN_UP(DataSize, Allocator->Alignment);
	u64 TotalSize = InitialCapacity * Stride + Allocator->Alignment - 1;
	Allocator->RawData = calloc(1, TotalSize);
	Allocator->Data = AlignPointer(Allocator->RawData, Allocator->Alignment);
}

void Memory_PoolAllocator_Free(Memory_PoolAllocator* Allocator)
{
	free(Allocator->RawData);
	if (Allocator->FreeList)
	{
		free(Allocator->FreeList);
	}
}

u32 Memory_PoolAllocator_PushData(Memory_PoolAllocator* Allocator, const void* Data)
{
	u32 Stride = SHARED_LIB_ALIGN_UP(Allocator->DataSize, Allocator->Alignment);

	if (Allocator->FreeCount > 0)
	{
		u32 Index = Allocator->FreeList[--Allocator->FreeCount];
		memcpy((char*)Allocator->Data + Index * Stride, Data, Allocator->DataSize);
		return Index;
	}

	if (Allocator->Count >= Allocator->capacity)
	{
		Allocator->capacity *= 2;
		u64 TotalSize = Allocator->capacity * Stride + Allocator->Alignment - 1;
		void* NewRawData = calloc(1, TotalSize);
		void* NewData = AlignPointer(NewRawData, Allocator->Alignment);

		for (u64 i = 0; i < Allocator->Count; ++i)
		{
			memcpy((char*)NewData + i * Stride, (char*)Allocator->Data + i * Stride, Allocator->DataSize);
		}

		free(Allocator->RawData);
		Allocator->RawData = NewRawData;
		Allocator->Data = NewData;
	}

	memcpy((char*)Allocator->Data + Allocator->Count * Stride, Data, Allocator->DataSize);
	return (u32)(Allocator->Count++);
}

void Memory_PoolAllocator_GetData(Memory_PoolAllocator* Allocator, u32 Index, void* OutData)
{
	u32 Stride = SHARED_LIB_ALIGN_UP(Allocator->DataSize, Allocator->Alignment);
	memcpy(OutData, (char*)Allocator->Data + Index * Stride, Allocator->DataSize);
}

void Memory_PoolAllocator_FreeData(Memory_PoolAllocator* Allocator, u32 Index)
{
	if (Allocator->FreeCount >= Allocator->FreeCapacity)
	{
		Allocator->FreeCapacity = Allocator->FreeCapacity ? Allocator->FreeCapacity * 2 : 8;
		Allocator->FreeList = (u32*)realloc(Allocator->FreeList, Allocator->FreeCapacity * sizeof(u32));
	}
	Allocator->FreeList[Allocator->FreeCount++] = Index;
}

// SparceHashMap
static u64 HashVkHandle(u64 VkHandle)
{
	VkHandle >>= 3;
	VkHandle ^= VkHandle >> 30; VkHandle *= UINT64_C(0xBF58476D1CE4E5B9);
	VkHandle ^= VkHandle >> 27; VkHandle *= UINT64_C(0x94D049BB133111EB);
	VkHandle ^= VkHandle >> 31;
	return VkHandle;
}

static inline u64 GetIndexFromHandleMask(u64 h, u64 Capacity)
{
	assert((Capacity & (Capacity - 1)) == 0);
	u64 Mask = Capacity - 1;
	return HashVkHandle(h) & Mask;
}

static inline u64 NextIndex(u64 i, u64 Mask)
{
	return (i + 1) & Mask;
}

static void SparceHashMap_Resize(Container_SparceHashMap* Map)
{
	u64 OldCapacity = Map->Capacity;
	u64* OldKeys = Map->Keys;
	u32* OldIndices = Map->Indices;
	u8* OldProbe = Map->ProbeDist;
	bool* OldOccupied = Map->Occupied;

	u64 NewCapacity = OldCapacity * 2;
	Container_SparceHashMap_Init(Map, NewCapacity);

	for (u64 i = 0; i < OldCapacity; ++i)
	{
		if (OldOccupied[i])
		{
			Container_SparceHashMap_Insert(Map, OldKeys[i], OldIndices[i]);
		}
	}

	free(OldKeys);
	free(OldIndices);
	free(OldProbe);
	free(OldOccupied);
}

void Container_SparceHashMap_Init(Container_SparceHashMap* Map, u64 InitialCapacity)
{
	assert((InitialCapacity & (InitialCapacity - 1)) == 0);
	Map->Capacity = InitialCapacity;
	Map->Count = 0;
	Map->Keys = (u64*)calloc(InitialCapacity, sizeof(u64));
	Map->Indices = (u32*)calloc(InitialCapacity, sizeof(u32));
	Map->ProbeDist = (u8*)calloc(InitialCapacity, sizeof(u8));
	Map->Occupied = (bool*)calloc(InitialCapacity, sizeof(bool));
}

void Container_SparceHashMap_Free(Container_SparceHashMap* Map)
{
	free(Map->Keys);
	free(Map->Indices);
	free(Map->ProbeDist);
	free(Map->Occupied);
}

void Container_SparceHashMap_Insert(Container_SparceHashMap* Map, u64 Key, u32 Index)
{
	const float LoadFactor = 0.8f;
	if (Map->Count + 1 > (u64)(Map->Capacity * LoadFactor))
	{
		SparceHashMap_Resize(Map);
	}

	u64 Mask = Map->Capacity - 1;
	u64 i = GetIndexFromHandleMask(Key, Map->Capacity);
	u8 Dist = 0;

	while (true)
	{
		if (!Map->Occupied[i])
		{
			Map->Keys[i] = Key;
			Map->Indices[i] = Index;
			Map->ProbeDist[i] = Dist;
			Map->Occupied[i] = true;
			Map->Count++;
			return;
		}

		if (Map->Keys[i] == Key)
		{
			Map->Indices[i] = Index;
			return;
		}

		if (Map->ProbeDist[i] < Dist)
		{
			u64 tmp_Key = Map->Keys[i];
			u32 tmp_Index = Map->Indices[i];
			u8 tmp_Dist = Map->ProbeDist[i];

			Map->Keys[i] = Key;
			Map->Indices[i] = Index;
			Map->ProbeDist[i] = Dist;

			Key = tmp_Key;
			Index = tmp_Index;
			Dist = tmp_Dist;
		}

		i = NextIndex(i, Mask);
		Dist++;
	}
}

bool Container_SparceHashMap_Get(const Container_SparceHashMap* Map, u64 Key, u32* OutIndex)
{
	u64 Mask = Map->Capacity - 1;
	u64 i = GetIndexFromHandleMask(Key, Map->Capacity);
	u8 Dist = 0;

	while (true)
	{
		if (!Map->Occupied[i] || Map->ProbeDist[i] < Dist)
		{
			return false;
		}

		if (Map->Keys[i] == Key)
		{
			*OutIndex = Map->Indices[i];
			return true;
		}

		i = NextIndex(i, Mask);
		Dist++;
	}
}

bool Container_SparceHashMap_Remove(Container_SparceHashMap* Map, u64 Key, u32* OutIndex)
{
	u64 Mask = Map->Capacity - 1;
	u64 i = GetIndexFromHandleMask(Key, Map->Capacity);
	u8 Dist = 0;

	while (true)
	{
		if (!Map->Occupied[i] || Map->ProbeDist[i] < Dist)
		{
			return false;
		}

		if (Map->Keys[i] == Key)
		{
			*OutIndex = Map->Indices[i];
			Map->Occupied[i] = false;

			u64 j = NextIndex(i, Mask);
			while (Map->Occupied[j] && Map->ProbeDist[j] > 0)
			{
				Map->Keys[i] = Map->Keys[j];
				Map->Indices[i] = Map->Indices[j];
				Map->ProbeDist[i] = Map->ProbeDist[j] - 1;
				Map->Occupied[i] = true;

				Map->Occupied[j] = false;
				i = j;
				j = NextIndex(j, Mask);
			}

			Map->Count--;
			return true;
		}

		i = NextIndex(i, Mask);
		Dist++;
	}
}

//DynamicArray


//HandleManager
static u16 GetType(u64 Handle)
{
	return (Handle >> 48) & 0xFFFF;
}

static u32 GetIndex(u64 Handle)
{
	return (Handle >> 16) & 0xFFFFFFFF;
}

static u16 GetGeneration(u64 Handle)
{
	return Handle & 0xFFFF;
}

static u64 Create(u16 Type, u32 Index, u16 Generation)
{
	return ((u64)Type << 48) | ((u64)Index << 16) | (u64)Generation;
}

bool System_HandleManager_CompareHandles(System_HandleManager_Handle a, System_HandleManager_Handle b)
{
	return a == b;
}

void System_HandleManager_InitData(System_HandleManager* Manager, u32 InitialCapacity, u32 DataSize, u16 HandleType)
{
	assert(Manager);

	Manager->Entries = (System_HandleManager_Entry*)(calloc(InitialCapacity, sizeof(System_HandleManager_Entry)));
	Memory_PoolAllocator_Init(&Manager->Storage, InitialCapacity, DataSize, 1);
	Manager->HandleType = HandleType;
}

void System_HandleManager_ClearData(System_HandleManager* Manager)
{
	assert(Manager);

	free(Manager->Entries);
	Memory_PoolAllocator_Free(&Manager->Storage);
}

System_HandleManager_Handle System_HandleManager_CreateHandle(System_HandleManager* Manager, const void* Data)
{
	assert(Manager);

	u32 OldCapacity = (u32)Manager->Storage.capacity;
	u32 OldCount = (u32)Manager->Storage.Count;
	u32 Index = Memory_PoolAllocator_PushData(&Manager->Storage, Data);

	if ((u32)Manager->Storage.capacity > OldCapacity)
	{
		const u32 NewCapacity = (u32)Manager->Storage.capacity;
		System_HandleManager_Entry* NewEntries = (System_HandleManager_Entry*)(calloc(NewCapacity, sizeof(System_HandleManager_Entry)));

		memcpy(NewEntries, Manager->Entries, OldCapacity * sizeof(System_HandleManager_Entry));
		free(Manager->Entries);

		Manager->Entries = NewEntries;
	}

	bool IsReused = Index < OldCount;

	if (IsReused)
	{
		Manager->Entries[Index].IsUsed = true;
		Manager->Entries[Index].Generation++;
	}
	else
	{
		Manager->Entries[Index].IsUsed = true;
		Manager->Entries[Index].Generation = 1;
	}

	System_HandleManager_Handle DataHandle = Create(Manager->HandleType, Index, Manager->Entries[Index].Generation);

	return DataHandle;
}

void System_HandleManager_DestroyHandle(System_HandleManager* Manager, System_HandleManager_Handle DataHandle)
{
	const u32 Index = GetIndex(DataHandle);
	const u16 Generation = GetGeneration(DataHandle);

	assert(System_HandleManager_IsHandleValid(Manager, DataHandle));

	Manager->Entries[Index].IsUsed = false;
	Memory_PoolAllocator_FreeData(&Manager->Storage, Index);
}

void System_HandleManager_GetHandleData(System_HandleManager* Manager, System_HandleManager_Handle DataHandle, void* OutData)
{
	const u32 Index = GetIndex(DataHandle);
	const u16 Generation = GetGeneration(DataHandle);

	assert(System_HandleManager_IsHandleValid(Manager, DataHandle));

	Memory_PoolAllocator_GetData(&Manager->Storage, Index, OutData);
}

bool System_HandleManager_IsHandleValid(System_HandleManager* Manager, System_HandleManager_Handle DataHandle)
{
	const u32 Index = GetIndex(DataHandle);
	const u16 Type = GetType(DataHandle);
	const u16 Generation = GetGeneration(DataHandle);

	assert(Index < (u32)Manager->Storage.Count);

	return Type == Manager->HandleType && Manager->Entries[Index].IsUsed && Manager->Entries[Index].Generation == Generation;
}

// Memory_LinearAllocator
void Memory_LinearAllocator_Init(Memory_LinearAllocator* Memory, u64 SpaceToAllocate)
{
	Memory->AllocatedSpace = SpaceToAllocate;
	Memory->Base = (u8*)calloc(Memory->AllocatedSpace, sizeof(u8));
	Memory->Head = Memory->Base;
}

void Memory_LinearAllocator_Free(Memory_LinearAllocator* Memory)
{
	free(Memory->Base);
}

void* Memory_LinearAllocator_Alloc(Memory_LinearAllocator* Memory, u64 Size)
{
	assert(Memory->Head + Size <= Memory->Base + Memory->AllocatedSpace);

	if (Size == 0)
	{
		return nullptr;
	}

	void* ReturnPointer = Memory->Head;
	Memory->Head += Size;

	return ReturnPointer;
}

void Memory_LinearAllocator_FreeMemory(Memory_LinearAllocator* Memory)
{
	Memory->Head = Memory->Base;
}

void* Memory_LinearAllocator_GetHead(Memory_LinearAllocator* Memory)
{
	return Memory->Head;
}
