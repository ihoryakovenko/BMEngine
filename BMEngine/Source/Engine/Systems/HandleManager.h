#pragma once

#include <Util/EngineTypes.h>

typedef u64 System_HandleManager_Handle;
typedef struct System_HandleManager_T* System_HandleManager;
typedef void (*System_HandleManager_OnClearManagerDelegate)(void* data);

System_HandleManager System_HandleManager_InitData(u32 InitialCapacity, u32 DataSize, u16 HandleType, System_HandleManager_OnClearManagerDelegate OnClearDelegate = nullptr);
void System_HandleManager_ClearData(System_HandleManager Manager);
System_HandleManager_Handle System_HandleManager_CreateHandle(System_HandleManager Manager, const void* Data);
void System_HandleManager_DestroyHandle(System_HandleManager Manager, System_HandleManager_Handle Handle);
void* System_HandleManager_GetHandleData(System_HandleManager Manager, System_HandleManager_Handle Handle);
bool System_HandleManager_IsHandleValid(System_HandleManager Manager, System_HandleManager_Handle Handle);
bool System_HandleManager_CompareHandles(System_HandleManager_Handle a, System_HandleManager_Handle b);