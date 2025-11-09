#pragma once

#include <Util/EngineTypes.h>

typedef u64 System_HandleManager_Handle;
typedef struct System_HandleManager_T* System_HandleManager;
typedef void (*System_HandleManager_OnClearManagerDelegate)(void* data);

System_HandleManager System_HandleManager_InitData(u32 InitialCapacity, u32 DataSize, u16 HandleType);
void System_HandleManager_ClearData(System_HandleManager Manager, System_HandleManager_OnClearManagerDelegate OnClearDelegate = nullptr);
System_HandleManager_Handle System_HandleManager_CreateHandle(System_HandleManager Manager, const void* Data);
void System_HandleManager_DestroyHandle(System_HandleManager Manager, System_HandleManager_Handle Handle);
void* System_HandleManager_GetHandleData(System_HandleManager Manager, System_HandleManager_Handle Handle);
bool System_HandleManager_IsHandleValid(System_HandleManager Manager, System_HandleManager_Handle Handle);
bool System_HandleManager_CompareHandles(System_HandleManager_Handle a, System_HandleManager_Handle b);

typedef void (*System_PoolAllocator_OnFreeDelegate)(void* data);

struct PoolAllocator
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

void Systems_PoolAllocator_Init(PoolAllocator* Allocator, u64 InitialCapacity, u32 DataSize, u32 Alignment = 1);
void Systems_PoolAllocator_Free(PoolAllocator* Allocator, System_PoolAllocator_OnFreeDelegate OnFreeDelegate);
u32 Systems_PoolAllocator_PushData(PoolAllocator* Allocator, const void* Data);
void Systems_PoolAllocator_GetData(PoolAllocator* Allocator, u32 Index, void* OutData);
void Systems_PoolAllocator_FreeData(PoolAllocator* Allocator, u32 Index);

struct SparceHashMap
{
    u64* Keys;
    u32* Indices;
    u8* ProbeDist;
    u8* Occupied;
    u64 Capacity;
    u64 Count;
};

void Systems_SparceHashMap_Init(SparceHashMap* Map, u64 InitialCapacity);
void Systems_SparceHashMap_Free(SparceHashMap* Map);
void Systems_SparceHashMap_Insert(SparceHashMap* Map, u64 Key, u32 Index);
bool Systems_SparceHashMap_Get(const SparceHashMap* Map, u64 Key, u32* OutIndex);
bool Systems_SparceHashMap_Remove(SparceHashMap* Map, u64 Key, u32* OutIndex);