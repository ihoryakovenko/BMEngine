#include "HandleManager.h"
#include <cstring>
#include <cassert>

#include <Engine/Systems/Memory/forge_memory_debugger.h>

static u16 GetType(u64 Handle) { 
	return (Handle >> 48) & 0xFFFF; 
}

static u32 GetIndex(u64 Handle) { 
	return (Handle >> 16) & 0xFFFFFFFF; 
}

static u16 GetGeneration(u64 Handle) { 
	return Handle & 0xFFFF; 
}

static u64 Create(u16 Type, u32 Index, u16 Generation) {
	return ((u64)Type << 48) | ((u64)Index << 16) | (u64)Generation;
}

struct System_HandleManager_Entry
{
	u32 IsUsed : 1;
	u32 Generation : 16;
};

struct System_HandleManager_T
{
	System_HandleManager_Entry* Entries;
	void* StorageData;
	u32* FreeIndices;
	u32 StorageCapacity;
	u32 FreeIndicesCapacity;
	u32 StorageCount;
	u32 FreeIndicesCount;
	u32 DataSize;
	u16 HandleType;
};

bool System_HandleManager_CompareHandles(System_HandleManager_Handle a, System_HandleManager_Handle b)
{
	return a == b;
}

System_HandleManager System_HandleManager_InitData(u32 InitialCapacity, u32 DataSize, u16 HandleType)
{
	auto Manager = (System_HandleManager)malloc(sizeof(System_HandleManager_T));

	Manager->Entries = (System_HandleManager_Entry*)(calloc(InitialCapacity, sizeof(System_HandleManager_Entry)));
	Manager->StorageData = malloc(InitialCapacity * DataSize);
	Manager->FreeIndices = (u32*)(malloc(InitialCapacity * sizeof(u32)));

	Manager->StorageCapacity = InitialCapacity;
	Manager->FreeIndicesCapacity = InitialCapacity;
	Manager->StorageCount = 0;
	Manager->FreeIndicesCount = 0;
	Manager->DataSize = DataSize;
	Manager->HandleType = HandleType;

	return Manager;
}

void System_HandleManager_ClearData(System_HandleManager Manager, System_HandleManager_OnClearManagerDelegate OnClearDelegate)
{
	assert(Manager);

	if (OnClearDelegate)
	{
		for (u32 i = 0; i < Manager->StorageCount; ++i)
		{
			if (Manager->Entries[i].IsUsed)
			{
				void* DataPtr = (u8*)(Manager->StorageData) + (i * Manager->DataSize);
				OnClearDelegate(DataPtr);
			}
		}
	}

	free(Manager->Entries);
	free(Manager->StorageData);
	free(Manager->FreeIndices);
	free(Manager);
}

System_HandleManager_Handle System_HandleManager_CreateHandle(System_HandleManager Manager, const void* Data)
{        
	assert(Manager);

	u32 Index;

	if (Manager->FreeIndicesCount > 0)
	{
		Index = Manager->FreeIndices[Manager->FreeIndicesCount - 1];
		--Manager->FreeIndicesCount;
	
		void* DataPtr = (u8*)(Manager->StorageData) + (Index * Manager->DataSize);
		memcpy(DataPtr, Data, Manager->DataSize);

		Manager->Entries[Index].IsUsed = true;
		Manager->Entries[Index].Generation++;
	}
	else
	{
		if (Manager->StorageCount >= Manager->StorageCapacity)
		{
			const u32 NewCapacity = Manager->StorageCapacity * 2;
			System_HandleManager_Entry* NewEntries = (System_HandleManager_Entry*)(calloc(NewCapacity, sizeof(System_HandleManager_Entry)));

			memcpy(NewEntries, Manager->Entries, Manager->StorageCapacity * sizeof(System_HandleManager_Entry));
			free(Manager->Entries);

			Manager->Entries = NewEntries;
			Manager->StorageData = realloc(Manager->StorageData, NewCapacity * Manager->DataSize);
			Manager->StorageCapacity = NewCapacity;
		}
		
		Index = Manager->StorageCount;
		void* DataPtr = (u8*)(Manager->StorageData) + (Index * Manager->DataSize);
		memcpy(DataPtr, Data, Manager->DataSize);
		Manager->Entries[Index].IsUsed = true;
		Manager->Entries[Index].Generation = 1;
		
		++Manager->StorageCount;
	}

	System_HandleManager_Handle DataHandle = Create(Manager->HandleType, Index, Manager->Entries[Index].Generation);

	return DataHandle;
}

void System_HandleManager_DestroyHandle(System_HandleManager Manager, System_HandleManager_Handle DataHandle)
{
	const u32 Index = GetIndex(DataHandle);
	const u16 Generation = GetGeneration(DataHandle);

	assert(System_HandleManager_IsHandleValid(Manager, DataHandle));

	if (Manager->FreeIndicesCount >= Manager->FreeIndicesCapacity)
	{
		Manager->FreeIndicesCapacity = Manager->FreeIndicesCapacity * 2;
		Manager->FreeIndices = (u32*)(realloc(Manager->FreeIndices, Manager->FreeIndicesCapacity * sizeof(u32)));
	}

	Manager->Entries[Index].IsUsed = false;
	Manager->FreeIndices[Manager->FreeIndicesCount] = Index;
	++Manager->FreeIndicesCount;
}

void* System_HandleManager_GetHandleData(System_HandleManager Manager, System_HandleManager_Handle DataHandle)
{
	const u32 Index = GetIndex(DataHandle);
	const u16 Generation = GetGeneration(DataHandle);

	assert(System_HandleManager_IsHandleValid(Manager, DataHandle));

	return (u8*)(Manager->StorageData) + (Index * Manager->DataSize);
}

bool System_HandleManager_IsHandleValid(System_HandleManager Manager, System_HandleManager_Handle DataHandle)
{
	const u32 Index = GetIndex(DataHandle);
	const u16 Type = GetType(DataHandle);
	const u16 Generation = GetGeneration(DataHandle);

	assert(Index < Manager->StorageCount);
	
	return Type == Manager->HandleType && Manager->Entries[Index].IsUsed && Manager->Entries[Index].Generation == Generation;
}

// Hash
static u32 AlignUp(u32 Value, u32 Alignment)
{
	return (Value + Alignment - 1) & ~(Alignment - 1);
}

static void* AlignPointer(void* Ptr, u32 Alignment)
{
	uintptr_t Addr = (uintptr_t)Ptr;
	uintptr_t AlignedAddr = (Addr + Alignment - 1) & ~(uintptr_t)(Alignment - 1);
	return (void*)AlignedAddr;
}

void Systems_PoolAllocator_Init(PoolAllocator* Allocator, u64 InitialCapacity, u32 DataSize, u32 Alignment)
{
	Allocator->FreeList = nullptr;
	Allocator->FreeCount = 0;
	Allocator->FreeCapacity = 0;
	Allocator->Count = 0;
	Allocator->capacity = InitialCapacity;
	Allocator->DataSize = DataSize;
	Allocator->Alignment = Alignment;

	u32 Stride = AlignUp(DataSize, Allocator->Alignment);
	u64 TotalSize = InitialCapacity * Stride + Allocator->Alignment - 1;
	Allocator->RawData = calloc(1, TotalSize);
	Allocator->Data = AlignPointer(Allocator->RawData, Allocator->Alignment);
}

void Systems_PoolAllocator_Free(PoolAllocator* Allocator)
{
	free(Allocator->RawData);
	if (Allocator->FreeList)
	{
		free(Allocator->FreeList);
	}
}

u32 Systems_PoolAllocator_PushData(PoolAllocator* Allocator, const void* Data)
{
	u32 Stride = AlignUp(Allocator->DataSize, Allocator->Alignment);

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

		// Copy existing data
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

void Systems_PoolAllocator_GetData(PoolAllocator* Allocator, u32 Index, void* OutData)
{
	u32 Stride = AlignUp(Allocator->DataSize, Allocator->Alignment);
	memcpy(OutData, (char*)Allocator->Data + Index * Stride, Allocator->DataSize);
}

void Systems_PoolAllocator_FreeData(PoolAllocator* Allocator, u32 Index)
{
	if (Allocator->FreeCount >= Allocator->FreeCapacity)
	{
		Allocator->FreeCapacity = Allocator->FreeCapacity ? Allocator->FreeCapacity * 2 : 8;
		Allocator->FreeList = (u32*)realloc(Allocator->FreeList, Allocator->FreeCapacity * sizeof(u32));
	}
	Allocator->FreeList[Allocator->FreeCount++] = Index;
}

// -----------------------------------------------------------------------------
// BufferMap implementation

static u64 HashVkHandle(u64 VkHandle)
{
	VkHandle >>= 3; // Drop low bits if handles are aligned
	VkHandle ^= VkHandle >> 30; VkHandle *= UINT64_C(0xBF58476D1CE4E5B9);
	VkHandle ^= VkHandle >> 27; VkHandle *= UINT64_C(0x94D049BB133111EB);
	VkHandle ^= VkHandle >> 31;
	return VkHandle;
}

static inline u64 GetIndexFromHandleMask(u64 h, u64 Capacity)
{
	assert((Capacity & (Capacity - 1)) == 0); // Power of two required
	u64 Mask = Capacity - 1;
	return HashVkHandle(h) & Mask;
}

static inline u64 NextIndex(u64 i, u64 Mask)
{
	return (i + 1) & Mask;
}

static void SparceHashMap_Resize(SparceHashMap* Map)
{
	u64 OldCapacity = Map->Capacity;
	u64* OldKeys = Map->Keys;
	u32* OldIndices = Map->Indices;
	u8* OldProbe = Map->ProbeDist;
	bool* OldOccupied = Map->Occupied;

	u64 NewCapacity = OldCapacity * 2;
	Systems_SparceHashMap_Init(Map, NewCapacity);

	for (u64 i = 0; i < OldCapacity; ++i)
	{
		if (OldOccupied[i])
		{
			Systems_SparceHashMap_Insert(Map, OldKeys[i], OldIndices[i]);
		}
	}

	free(OldKeys);
	free(OldIndices);
	free(OldProbe);
	free(OldOccupied);
}

void Systems_SparceHashMap_Init(SparceHashMap* Map, u64 InitialCapacity)
{
	assert((InitialCapacity & (InitialCapacity - 1)) == 0);
	Map->Capacity = InitialCapacity;
	Map->Count = 0;
	Map->Keys = (u64*)calloc(InitialCapacity, sizeof(u64));
	Map->Indices = (u32*)calloc(InitialCapacity, sizeof(u32));
	Map->ProbeDist = (u8*)calloc(InitialCapacity, sizeof(u8));
	Map->Occupied = (bool*)calloc(InitialCapacity, sizeof(bool));
}

void Systems_SparceHashMap_Free(SparceHashMap* Map)
{
	free(Map->Keys);
	free(Map->Indices);
	free(Map->ProbeDist);
	free(Map->Occupied);
}

void Systems_SparceHashMap_Insert(SparceHashMap* Map, u64 Key, u32 Index)
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

bool Systems_SparceHashMap_Get(const SparceHashMap* Map, u64 Key, u32* OutIndex)
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

bool Systems_SparceHashMap_Remove(SparceHashMap* Map, u64 Key, u32* OutIndex)
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

			// Backward-shift deletion
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