#pragma once

#include <Util/EngineTypes.h>

struct Systems_HandleManager_Handle
{
	u64 Type : 16;       // 65,536 handle types (0-65,535)
	u64 Index : 24;      // 16.7 million indices (0-16,777,215)
	u64 Generation : 24; // 16.7 million generations (0-16,777,215)
};

struct Systems_HandleManager_Entry
{
	bool IsUsed;
	u32 Generation;
};

struct Systems_HandleManager_Data
{
	Systems_HandleManager_Entry* Entries;
	void* StorageData;
	u32* FreeIndices;
	u32 StorageCapacity;
	u32 FreeIndicesCapacity;
	u32 StorageCount;
	u32 FreeIndicesCount;
	u32 DataSize;
	u32 HandleType;
};

bool Systems_HandleManager_CompareHandles(Systems_HandleManager_Handle a, Systems_HandleManager_Handle b);

void Systems_HandleManager_InitData(Systems_HandleManager_Data* Manager, u32 InitialCapacity, u32 DataSize, u32 HandleType);
void Systems_HandleManager_ClearData(Systems_HandleManager_Data* Manager);
Systems_HandleManager_Handle Systems_HandleManager_CreateHandle(Systems_HandleManager_Data* Manager, const void* Data);
void Systems_HandleManager_DestroyHandle(Systems_HandleManager_Data* Manager, Systems_HandleManager_Handle Handle);
void* Systems_HandleManager_GetHandleData(Systems_HandleManager_Data* Manager, Systems_HandleManager_Handle Handle);
bool Systems_HandleManager_IsHandleValid(Systems_HandleManager_Data* Manager, Systems_HandleManager_Handle Handle);