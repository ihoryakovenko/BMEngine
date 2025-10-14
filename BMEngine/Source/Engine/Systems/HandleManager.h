#pragma once

#include <vector>
#include <Util/EngineTypes.h>

namespace HandleManager
{
    struct Handle
    {
        u32 Index : 24;
        u32 Generation : 8;
    };

    struct HandleEntry
    {
        bool IsUsed;
        u32 Generation;
    };

    struct HandleManagerData
    {
        HandleEntry* Entries;
        void* StorageData;
        std::vector<u32> FreeIndices;
        u32 StorageCapacity;
        u32 FreeIndicesCapacity;
        u32 StorageCount;
        u32 FreeIndicesCount;
        u32 DataSize;
    };

    bool CompareHandles(Handle a, Handle b);
    size_t GetHandleHash(Handle handle);

    void InitHandleManagerData(HandleManagerData* Manager, u32 InitialCapacity, u32 DataSize);
    void ClearHandleManager(HandleManagerData* Manager);
    Handle CreateHandle(HandleManagerData* Manager, const void* Data);
    void DestroyHandle(HandleManagerData* Manager, Handle Handle);
    void* GetHandleData(HandleManagerData* Manager, Handle Handle);
    bool IsHandleValid(HandleManagerData* Manager, Handle Handle);
}