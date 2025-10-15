#include "HandleManager.h"
#include <cstring>
#include <cassert>

#include <Engine/Systems/Memory/forge_memory_debugger.h>

bool Systems_HandleManager_CompareHandles(Systems_HandleManager_Handle a, Systems_HandleManager_Handle b)
{
	return a.Type == b.Type && a.Index == b.Index && a.Generation == b.Generation;
}

void Systems_HandleManager_InitData(Systems_HandleManager_Data* Manager, u32 InitialCapacity, u32 DataSize, u32 HandleType)
{
	assert(Manager);
	
	Manager->Entries = (Systems_HandleManager_Entry*)(calloc(InitialCapacity, sizeof(Systems_HandleManager_Entry)));
	Manager->StorageData = malloc(InitialCapacity * DataSize);
	Manager->FreeIndices = (u32*)(malloc(InitialCapacity * sizeof(u32)));

	Manager->StorageCapacity = InitialCapacity;
	Manager->FreeIndicesCapacity = InitialCapacity;
	Manager->StorageCount = 0;
	Manager->FreeIndicesCount = 0;
	Manager->DataSize = DataSize;
	Manager->HandleType = HandleType;
	
	for (u32 i = 0; i < InitialCapacity; i++)
	{
		Manager->Entries[i].IsUsed = false;
		Manager->Entries[i].Generation = 0;
	}
}

void Systems_HandleManager_ClearData(Systems_HandleManager_Data* Manager)
{
	assert(Manager);
	free(Manager->Entries);
	free(Manager->StorageData);
	free(Manager->FreeIndices);
}

Systems_HandleManager_Handle Systems_HandleManager_CreateHandle(Systems_HandleManager_Data* Manager, const void* Data)
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
			Systems_HandleManager_Entry* NewEntries = (Systems_HandleManager_Entry*)(calloc(NewCapacity, sizeof(Systems_HandleManager_Entry)));

			memcpy(NewEntries, Manager->Entries, Manager->StorageCapacity * sizeof(Systems_HandleManager_Entry));
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

	Systems_HandleManager_Handle DataHandle;
	DataHandle.Index = Index;
	DataHandle.Generation = Manager->Entries[Index].Generation;
	DataHandle.Type = Manager->HandleType;
	return DataHandle;

	return DataHandle;
}

void Systems_HandleManager_DestroyHandle(Systems_HandleManager_Data* Manager, Systems_HandleManager_Handle DataHandle)
{
	assert(DataHandle.Index < Manager->StorageCount);
	assert(Manager->Entries[DataHandle.Index].IsUsed);
	assert(Manager->Entries[DataHandle.Index].Generation == DataHandle.Generation);

	if (Manager->FreeIndicesCount >= Manager->FreeIndicesCapacity)
	{
		Manager->FreeIndicesCapacity = Manager->FreeIndicesCapacity * 2;
		Manager->FreeIndices = (u32*)(realloc(Manager->FreeIndices, Manager->FreeIndicesCapacity * sizeof(u32)));
	}

	Manager->Entries[DataHandle.Index].IsUsed = false;
	Manager->FreeIndices[Manager->FreeIndicesCount] = DataHandle.Index;
	++Manager->FreeIndicesCount;
}

void* Systems_HandleManager_GetHandleData(Systems_HandleManager_Data* Manager, Systems_HandleManager_Handle DataHandle)
{
	assert(DataHandle.Index < Manager->StorageCount);
	assert(Manager->Entries[DataHandle.Index].IsUsed);
	assert(Manager->Entries[DataHandle.Index].Generation == DataHandle.Generation);

	return (u8*)(Manager->StorageData) + (DataHandle.Index * Manager->DataSize);
}

bool Systems_HandleManager_IsHandleValid(Systems_HandleManager_Data* Manager, Systems_HandleManager_Handle DataHandle)
{
	assert(DataHandle.Index < Manager->StorageCount);
	assert(DataHandle.Type == Manager->HandleType);
	
	return Manager->Entries[DataHandle.Index].IsUsed && Manager->Entries[DataHandle.Index].Generation == DataHandle.Generation;
}