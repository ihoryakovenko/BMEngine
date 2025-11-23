#pragma once

#include "ShortTypes.h"

struct Memory_PoolAllocator
{
	void* Data;
	void* RawData;
	u32* FreeList;
	u64 FreeCount;
	u64 FreeCapacity;
	u64 Count;
	u64 capacity;
	u32 DataSize;
	u32 Alignment;
};

void Memory_PoolAllocator_Init(Memory_PoolAllocator* Allocator, u64 InitialCapacity, u32 DataSize, u32 Alignment = 1);
void Memory_PoolAllocator_Free(Memory_PoolAllocator* Allocator);
u32 Memory_PoolAllocator_PushData(Memory_PoolAllocator* Allocator, const void* Data);
void Memory_PoolAllocator_GetData(Memory_PoolAllocator* Allocator, u32 Index, void* OutData);
void Memory_PoolAllocator_FreeData(Memory_PoolAllocator* Allocator, u32 Index);

struct Memory_LinearAllocator
{
	u32 AllocatedSpace;
	u8* Head;
	u8* Base;
};

void Memory_LinearAllocator_Init(Memory_LinearAllocator* Memory, u64 SpaceToAllocate);
void Memory_LinearAllocator_Free(Memory_LinearAllocator* Memory);
void* Memory_LinearAllocator_Alloc(Memory_LinearAllocator* Memory, u64 Size);
void Memory_LinearAllocator_FreeMemory(Memory_LinearAllocator* Memory);
void* Memory_LinearAllocator_GetHead(Memory_LinearAllocator* Memory);

struct SparceHashMap
{
	u64* Keys;
	u32* Indices;
	u8* ProbeDist;
	bool* Occupied;
	u64 Capacity;
	u64 Count;
};

void Systems_SparceHashMap_Init(SparceHashMap* Map, u64 InitialCapacity);
void Systems_SparceHashMap_Free(SparceHashMap* Map);
void Systems_SparceHashMap_Insert(SparceHashMap* Map, u64 Key, u32 Index);
bool Systems_SparceHashMap_Get(const SparceHashMap* Map, u64 Key, u32* OutIndex);
bool Systems_SparceHashMap_Remove(SparceHashMap* Map, u64 Key, u32* OutIndex);

struct System_HandleManager_Entry
{
	u32 IsUsed : 1;
	u32 Generation : 16;
};

struct System_HandleManager
{
	System_HandleManager_Entry* Entries;
	Memory_PoolAllocator Storage;
	u16 HandleType;
};

typedef u64 System_HandleManager_Handle;

void System_HandleManager_InitData(System_HandleManager* Manager, u32 InitialCapacity, u32 DataSize, u16 HandleType);
void System_HandleManager_ClearData(System_HandleManager* Manager);
System_HandleManager_Handle System_HandleManager_CreateHandle(System_HandleManager* Manager, const void* Data);
void System_HandleManager_DestroyHandle(System_HandleManager* Manager, System_HandleManager_Handle Handle);
void System_HandleManager_GetHandleData(System_HandleManager* Manager, System_HandleManager_Handle Handle, void* OutData);
bool System_HandleManager_IsHandleValid(System_HandleManager* Manager, System_HandleManager_Handle Handle);
bool System_HandleManager_CompareHandles(System_HandleManager_Handle a, System_HandleManager_Handle b);
