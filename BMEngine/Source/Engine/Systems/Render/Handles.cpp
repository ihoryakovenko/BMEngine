#include "Handles.h"

static u16 GetNextHandleType()
{
	static u16 HandleType = 0;
	return HandleType++;
}

static u64 MixVkObjectWithVkType(u64 Handle, VkObjectType Type)
{
	return Handle ^ (static_cast<uint64_t>(Type) << 48);
}

struct StoragePair
{
	PoolAllocator Allocator;
	SparceHashMap HashMap;
};

static void OnStorageClear(StoragePair* Storage, void(*CleanUpFunc)(void* CleanupData))
{
	for (u64 i = 0; i < Storage->HashMap.Capacity; ++i)
	{
		if (Storage->HashMap.Occupied[i])
		{
			DescriptorSetLayoutData Data;
			Systems_PoolAllocator_GetData(&Storage->Allocator, Storage->HashMap.Indices[i], &Data);
			CleanUpFunc(&Data);
		}
	}

	Systems_SparceHashMap_Free(&Storage->HashMap);
	Systems_PoolAllocator_Free(&Storage->Allocator);
}

static StoragePair DescriptorSetLayoutStorage;

static System_HandleManager ShaderManager;
static System_HandleManager ImageManager;
static System_HandleManager GPUBufferManager;
static System_HandleManager DescriptorSetManager;
static System_HandleManager SemaphoreManager;
static System_HandleManager CommandPoolManager;
static System_HandleManager CommandBufferManager;

// INIT
void InitializeDescriptorSetLayoutManager(u32 Size)
{
	Systems_PoolAllocator_Init(&DescriptorSetLayoutStorage.Allocator, Size, sizeof(DescriptorSetLayoutData));
	Systems_SparceHashMap_Init(&DescriptorSetLayoutStorage.HashMap, Size);
}

void InitializeShaderManager(u32 Size)
{
	ShaderManager = System_HandleManager_InitData(Size, sizeof(ShaderData), GetNextHandleType());
}

void InitializeImageManager(u32 Size)
{
	ImageManager = System_HandleManager_InitData(Size, sizeof(ImageResource), GetNextHandleType());
}

void InitializeGPUBufferManager(u32 Size)
{
	GPUBufferManager = System_HandleManager_InitData(Size, sizeof(GPUBufferData), GetNextHandleType());
}

void InitializeDescriptorSetManager(u32 Size)
{
	DescriptorSetManager = System_HandleManager_InitData(Size, sizeof(DescriptorSetData), GetNextHandleType());
}

void InitializeSemaphoreManager(u32 Size)
{
	SemaphoreManager = System_HandleManager_InitData(Size, sizeof(SemaphoreData), GetNextHandleType());
}

void InitializeCommandPoolManager(u32 Size)
{
	CommandPoolManager = System_HandleManager_InitData(Size, sizeof(CommandPoolData), GetNextHandleType());
}

void InitializeCommandBufferManager(u32 Size)
{
	CommandBufferManager = System_HandleManager_InitData(Size, sizeof(CommandBufferData), GetNextHandleType());
}
// INIT

// DEINIT
void DeinitDescriptorSetLayoutManager(void(*CleanupFunc)(DescriptorSetLayoutData*))
{
	OnStorageClear(&DescriptorSetLayoutStorage, (void(*)(void*))CleanupFunc);
}

void DeinitShaderManager(void(*CleanupFunc)(ShaderData*))
{
	System_HandleManager_ClearData(ShaderManager, (void(*)(void*))CleanupFunc);
}

void DeinitImageManager(void(*CleanupFunc)(ImageResource*))
{
	System_HandleManager_ClearData(ImageManager, (void(*)(void*))CleanupFunc);
}

void DeinitGPUBufferManager(void(*CleanUpFunc)(GPUBufferData*))
{
	System_HandleManager_ClearData(GPUBufferManager, (void(*)(void*))CleanUpFunc);
}

void DeinitDescriptorSetManager()
{
	System_HandleManager_ClearData(DescriptorSetManager);
}

void DeinitSemaphoreManager(void(*CleanUpFunc)(SemaphoreData*))
{
	System_HandleManager_ClearData(SemaphoreManager, (void(*)(void*))CleanUpFunc);
}

void DeinitCommandPoolManager(void(*CleanUpFunc)(CommandPoolData*))
{
	System_HandleManager_ClearData(CommandPoolManager, (void(*)(void*))CleanUpFunc);
}

void DeinitCommandBufferManager()
{
	System_HandleManager_ClearData(CommandBufferManager);
}
// DEINIT

// CREATE
BmRender_Sampler CreateSamplerHandle(VkSampler Sampler)
{
	return (BmRender_Sampler)Sampler;
}

BmRender_Pipeline CreatePipelineHandle(VkPipeline Pipeline)
{
	return (BmRender_Pipeline)Pipeline;
}

BmRender_PipelineLayout CreatePipelineLayoutHandle(VkPipelineLayout PipelineLayout)
{
	return (BmRender_PipelineLayout)PipelineLayout;
}

BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(VkDescriptorSetLayout Layout, const DescriptorSetLayoutData* Data)
{
	const u32 Index = Systems_PoolAllocator_PushData(&DescriptorSetLayoutStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&DescriptorSetLayoutStorage.HashMap, (u64)Layout, Index);

	return (BmRender_DescriptorSetLayout)Layout;
}

BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool)
{
	return (BmRender_DescriptorPool)DescriptorPool;
}

BmRender_Shader CreateShaderHandle(const ShaderData* Data)
{
	BmRender_Shader Handle;
	Handle.Private = System_HandleManager_CreateHandle(ShaderManager, Data);
	return Handle;
}

BmRender_Image CreateImageHandle(const ImageResource* Data)
{
	BmRender_Image Handle;
	Handle.Private = System_HandleManager_CreateHandle(ImageManager, Data);
	return Handle;
}

BmRender_ImageView CreateImageViewHandle(VkImageView ImageView)
{
	return (BmRender_ImageView)ImageView;
}

BmRender_GPUBuffer CreateGPUBufferHandle(const GPUBufferData* Data)
{
	BmRender_GPUBuffer Handle;
	Handle.Private = System_HandleManager_CreateHandle(GPUBufferManager, Data);
	return Handle;
}


BmRender_DescriptorSet CreateDescriptorSetHandle(const DescriptorSetData* Data)
{
	BmRender_DescriptorSet Handle;
	Handle.Private = System_HandleManager_CreateHandle(DescriptorSetManager, Data);
	return Handle;
}

BmRender_Fence CreateFenceHandle(VkFence Fence)
{
	return (BmRender_Fence)Fence;
}

BmRender_Semaphore CreateSemaphoreHandle(const SemaphoreData* Data)
{
	BmRender_Semaphore Handle;
	Handle.Private = System_HandleManager_CreateHandle(SemaphoreManager, Data);
	return Handle;
}

BmRender_CommandPool CreateCommandPoolHandle(const CommandPoolData* Data)
{
	BmRender_CommandPool Handle;
	Handle.Private = System_HandleManager_CreateHandle(CommandPoolManager, Data);
	return Handle;
}

BmRender_CommandBuffer CreateCommandBufferHandle(const CommandBufferData* Data)
{
	BmRender_CommandBuffer Handle;
	Handle.Private = System_HandleManager_CreateHandle(CommandBufferManager, Data);
	return Handle;
}
// CREATE

// DESTROY
void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle)
{
	VkDescriptorSetLayout Layout = (VkDescriptorSetLayout)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&DescriptorSetLayoutStorage.HashMap, (u64)Layout, &Index))
	{
		Systems_PoolAllocator_FreeData(&DescriptorSetLayoutStorage.Allocator, Index);
	}
}

void DestroyShaderHandle(BmRender_Shader Handle)
{
	System_HandleManager_DestroyHandle(ShaderManager, Handle.Private);
}

void DestroyImageHandle(BmRender_Image Handle)
{
	System_HandleManager_DestroyHandle(ImageManager, Handle.Private);
}

void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle)
{
	System_HandleManager_DestroyHandle(GPUBufferManager, Handle.Private);
}




void DestroySemaphoreHandle(BmRender_Semaphore Handle)
{
	System_HandleManager_DestroyHandle(SemaphoreManager, Handle.Private);
}

void DestroyCommandPoolHandle(BmRender_CommandPool Handle)
{
	System_HandleManager_DestroyHandle(CommandPoolManager, Handle.Private);
}

void DestroyCommandBufferHandle(BmRender_CommandBuffer Handle)
{
	System_HandleManager_DestroyHandle(CommandBufferManager, Handle.Private);
}
// DESTROY

// GET
void GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle, DescriptorSetLayoutData* OutData)
{
	VkDescriptorSetLayout Layout = (VkDescriptorSetLayout)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Get(&DescriptorSetLayoutStorage.HashMap, (u64)Layout, &Index))
	{
		Systems_PoolAllocator_GetData(&DescriptorSetLayoutStorage.Allocator, Index, OutData);
	}
}

ShaderData* GetShaderData(BmRender_Shader Handle)
{
	return (ShaderData*)System_HandleManager_GetHandleData(ShaderManager, Handle.Private);
}

ImageResource* GetImageData(BmRender_Image Handle)
{
	return (ImageResource*)System_HandleManager_GetHandleData(ImageManager, Handle.Private);
}

GPUBufferData* GetGPUBufferData(BmRender_GPUBuffer Handle)
{
	return (GPUBufferData*)System_HandleManager_GetHandleData(GPUBufferManager, Handle.Private);
}

DescriptorSetData* GetDescriptorSetData(BmRender_DescriptorSet Handle)
{
	return (DescriptorSetData*)System_HandleManager_GetHandleData(DescriptorSetManager, Handle.Private);
}

SemaphoreData* GetSemaphoreData(BmRender_Semaphore Handle)
{
	return (SemaphoreData*)System_HandleManager_GetHandleData(SemaphoreManager, Handle.Private);
}

CommandPoolData* GetCommandPoolData(BmRender_CommandPool Handle)
{
	return (CommandPoolData*)System_HandleManager_GetHandleData(CommandPoolManager, Handle.Private);
}

CommandBufferData* GetCommandBufferData(BmRender_CommandBuffer Handle)
{
	return (CommandBufferData*)System_HandleManager_GetHandleData(CommandBufferManager, Handle.Private);
}
// GET