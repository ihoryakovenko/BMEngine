#include "HandleManager.h"
#include <cstring>

#include <Engine/Systems/Memory/forge_memory_debugger.h>

namespace HandleManager
{

    bool CompareHandles(Handle a, Handle b)
    {
        return a.Index == b.Index && a.Generation == b.Generation;
    }

    size_t GetHandleHash(Handle DataHandle)
    {
        return ((size_t)(DataHandle.Index) << 8) | DataHandle.Generation;
    }

    void InitHandleManagerData(HandleManagerData* Manager, u32 InitialCapacity, u32 DataSize)
    {
        if (!Manager) return;
        
        Manager->Entries = (HandleEntry*)(calloc(InitialCapacity, sizeof(HandleEntry)));
        Manager->StorageData = malloc(InitialCapacity * DataSize);
        Manager->FreeIndices.reserve(InitialCapacity);
    
        Manager->StorageCapacity = InitialCapacity;
        Manager->FreeIndicesCapacity = InitialCapacity;
        Manager->StorageCount = 0;
        Manager->FreeIndicesCount = 0;
        Manager->DataSize = DataSize;
        
        for (u32 i = 0; i < InitialCapacity; i++)
        {
            Manager->Entries[i].IsUsed = false;
            Manager->Entries[i].Generation = 0;
        }
    }

    void ClearHandleManager(HandleManagerData* Manager)
    {
        if (!Manager) return;
    
        if (Manager->Entries)
        {
            free(Manager->Entries);
            Manager->Entries = nullptr;
        }
        
        if (Manager->StorageData)
        {
            free(Manager->StorageData);
            Manager->StorageData = nullptr;
        }
    }

    Handle CreateHandle(HandleManagerData* Manager, const void* Data)
    {        
        u32 Index;
        
        if (!Manager->FreeIndices.empty())
        {
            Index = Manager->FreeIndices.back();
            Manager->FreeIndices.pop_back();
        
            void* DataPtr = (u8*)(Manager->StorageData) + (Index * Manager->DataSize);
            memcpy(DataPtr, Data, Manager->DataSize);
            Manager->Entries[Index].IsUsed = true;
            Manager->Entries[Index].Generation++;
        }
        else
        {
            Index = Manager->StorageCount;
            void* DataPtr = (u8*)(Manager->StorageData) + (Index * Manager->DataSize);
            memcpy(DataPtr, Data, Manager->DataSize);
            Manager->Entries[Index].IsUsed = true;
            Manager->Entries[Index].Generation = 1;
            
            Manager->StorageCount++;
        }

        Handle DataHandle;
        DataHandle.Index = Index;
        DataHandle.Generation = Manager->Entries[Index].Generation;
        return DataHandle;
    
        return DataHandle;
    }

    void DestroyHandle(HandleManagerData* Manager, Handle DataHandle)
    {
        if (DataHandle.Index >= Manager->StorageCount) return;
        if (!Manager->Entries[DataHandle.Index].IsUsed) return;
        if (Manager->Entries[DataHandle.Index].Generation != DataHandle.Generation) return;
    
        void* DataPtr = (u8*)(Manager->StorageData) + (DataHandle.Index * Manager->DataSize);
        memset(DataPtr, 0, Manager->DataSize);
        Manager->Entries[DataHandle.Index].IsUsed = false;
        
        Manager->FreeIndices.push_back(DataHandle.Index);
        Manager->FreeIndicesCount = (u32)(Manager->FreeIndices.size());
    }

    void* GetHandleData(HandleManagerData* Manager, Handle DataHandle)
    {
        if (DataHandle.Index >= Manager->StorageCount) return nullptr;
        if (!Manager->Entries[DataHandle.Index].IsUsed) return nullptr;
        if (Manager->Entries[DataHandle.Index].Generation != DataHandle.Generation) return nullptr;
    
        return (u8*)(Manager->StorageData) + (DataHandle.Index * Manager->DataSize);
    }

    bool IsHandleValid(HandleManagerData* Manager, Handle Handle)
    {
        if (Handle.Index >= Manager->StorageCount) return false;
        
        return Manager->Entries[Handle.Index].IsUsed && 
               Manager->Entries[Handle.Index].Generation == Handle.Generation;
    }
}