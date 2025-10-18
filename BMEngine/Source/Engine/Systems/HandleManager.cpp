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

	System_HandleManager_OnClearManagerDelegate OnClearDelegate;
};

bool System_HandleManager_CompareHandles(System_HandleManager_Handle a, System_HandleManager_Handle b)
{
	return a == b;
}

System_HandleManager System_HandleManager_InitData(u32 InitialCapacity, u32 DataSize, u16 HandleType, System_HandleManager_OnClearManagerDelegate OnClearDelegate)
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
	Manager->OnClearDelegate = OnClearDelegate;

	return Manager;
}

void System_HandleManager_ClearData(System_HandleManager Manager)
{
	assert(Manager);

	if (Manager->OnClearDelegate)
	{
		for (u32 i = 0; i < Manager->StorageCount; ++i)
		{
			if (Manager->Entries[i].IsUsed)
			{
				void* DataPtr = (u8*)(Manager->StorageData) + (i * Manager->DataSize);
				Manager->OnClearDelegate(DataPtr);
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