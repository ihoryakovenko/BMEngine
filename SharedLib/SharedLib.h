#pragma once

#include "ShortTypes.h"

#define SHARED_LIB_ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define SHARED_LIB_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define SHARED_LIB_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

template<typename F>
struct Helpers_Defer
{
	Helpers_Defer(F func) : m_func(func) {}
	~Helpers_Defer() { m_func(); }
	F m_func;
};

template<typename F>
Helpers_Defer<F> Helpers_MakeDefer(F func)
{
	return Helpers_Defer<F>(func);
}

#define DEFER(code) const auto CONCAT(_helpers_defer_, __LINE__) = Helpers_MakeDefer([&](){ code; })

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
	u8* Head;
	u8* Base;
	u32 AllocatedSpace;
};

void Memory_LinearAllocator_Init(Memory_LinearAllocator* Memory, u64 SpaceToAllocate);
void Memory_LinearAllocator_Free(Memory_LinearAllocator* Memory);
void* Memory_LinearAllocator_Alloc(Memory_LinearAllocator* Memory, u64 Size);
void Memory_LinearAllocator_FreeMemory(Memory_LinearAllocator* Memory);
void* Memory_LinearAllocator_GetHead(Memory_LinearAllocator* Memory);

#define Memory_LinearAllocator_AllocT(Memory, Type) (Type*)Memory_LinearAllocator_Alloc(Memory, sizeof(Type))
#define Memory_LinearAllocator_AllocTC(Memory, Type, Count) (Type*)Memory_LinearAllocator_Alloc(Memory, sizeof(Type) * Count)

struct Memory_ScopeAllocator
{
	Memory_LinearAllocator Allocator;
};

void Memory_ScopeAllocator_Init(Memory_ScopeAllocator* Memory, u64 SpaceToAllocate);
void Memory_ScopeAllocator_Free(Memory_ScopeAllocator* Memory);
void* Memory_ScopeAllocator_Allocate(Memory_ScopeAllocator* Memory, u64 Size);
void Memory_ScopeAllocator_FreeSpace(Memory_ScopeAllocator* Memory, u64 SpaceToFree);

#define Memory_ScopeAllocator_Alloc(Memory, Size) \
	Memory_ScopeAllocator_Allocate(Memory, Size); const auto CONCAT(_helpers_defer_, __LINE__) = Helpers_MakeDefer([&]() { Memory_ScopeAllocator_FreeSpace(Memory, Size); })

#define Memory_ScopeAllocator_AllocT(Memory, Type) \
	(Type*)Memory_ScopeAllocator_Allocate(Memory, sizeof(Type)); const auto CONCAT(_helpers_defer_, __LINE__) = Helpers_MakeDefer([&]() { Memory_ScopeAllocator_FreeSpace(Memory, sizeof(Type)); })

#define Memory_ScopeAllocator_AllocTC(Memory, Type, Count) \
	(Type*)Memory_ScopeAllocator_Allocate(Memory, sizeof(Type) * Count); const auto CONCAT(_helpers_defer_, __LINE__) = Helpers_MakeDefer([&]() { Memory_ScopeAllocator_FreeSpace(Memory, sizeof(Type) * Count); })

struct Container_SparceHashMap
{
	u64* Keys;
	u32* Indices;
	u8* ProbeDist;
	bool* Occupied;
	u64 Capacity;
	u64 Count;
};

void Container_SparceHashMap_Init(Container_SparceHashMap* Map, u64 InitialCapacity);
void Container_SparceHashMap_Free(Container_SparceHashMap* Map);
void Container_SparceHashMap_Insert(Container_SparceHashMap* Map, u64 Key, u32 Index);
bool Container_SparceHashMap_Get(const Container_SparceHashMap* Map, u64 Key, u32* OutIndex);
bool Container_SparceHashMap_Remove(Container_SparceHashMap* Map, u64 Key, u32* OutIndex);

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
