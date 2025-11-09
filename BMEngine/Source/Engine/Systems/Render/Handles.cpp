#include "Handles.h"

static PoolAllocator GeneralHandleStorage;
static SparceHashMap GeneralHashMap;

static System_HandleManager DescriptorSetLayoutManager;
static System_HandleManager ShaderManager;
static System_HandleManager ImageManager;
static System_HandleManager GPUBufferManager;
static System_HandleManager DescriptorSetManager;
static System_HandleManager SemaphoreManager;
static System_HandleManager CommandPoolManager;
static System_HandleManager CommandBufferManager;

static u16 GetNextHandleType()
{
	static u16 HandleType = 0;
	return HandleType++;
}

static u64 MixVkObjectWithVkType(u64 Handle, VkObjectType Type)
{
	return Handle ^ (static_cast<uint64_t>(Type) << 48);
}

// INIT
void InitializeGeneralHandleStorage(u32 Size)
{
	Systems_PoolAllocator_Init(&GeneralHandleStorage, Size, sizeof(TrackedData));
	Systems_SparceHashMap_Init(&GeneralHashMap, Size);
}

void InitializeDescriptorSetLayoutManager(u32 Size)
{
	DescriptorSetLayoutManager = System_HandleManager_InitData(Size, sizeof(DescriptorSetLayoutData), GetNextHandleType());
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
	System_HandleManager_ClearData(DescriptorSetLayoutManager, (void(*)(void*))CleanupFunc);
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

void DeinitGeneralHandleStorage(void(*CleanUpFunc)(TrackedData*))
{
	Systems_SparceHashMap_Free(&GeneralHashMap);
	Systems_PoolAllocator_Free(&GeneralHandleStorage, (void(*)(void*))CleanUpFunc);
}
// DEINIT

// CREATE
BmRender_Sampler CreateSamplerHandle(VkSampler Sampler)
{
	TrackedData Handle;
	Handle.InternalData = Sampler;
	Handle.Type = TrackedDataType::Sampler;

	const u32 Index = Systems_PoolAllocator_PushData(&GeneralHandleStorage, &Handle);
	Systems_SparceHashMap_Insert(&GeneralHashMap, MixVkObjectWithVkType((u64)Sampler, VkObjectType::VK_OBJECT_TYPE_SAMPLER), Index);

	return (BmRender_Sampler)Sampler;
}

BmRender_Pipeline CreatePipelineHandle(VkPipeline Pipeline)
{
	TrackedData Handle;
	Handle.InternalData = Pipeline;
	Handle.Type = TrackedDataType::Pipeline;

	const u32 Index = Systems_PoolAllocator_PushData(&GeneralHandleStorage, &Handle);
	Systems_SparceHashMap_Insert(&GeneralHashMap, MixVkObjectWithVkType((u64)Pipeline, VkObjectType::VK_OBJECT_TYPE_PIPELINE), Index);

	return (BmRender_Pipeline)Pipeline;
}

BmRender_PipelineLayout CreatePipelineLayoutHandle(VkPipelineLayout PipelineLayout)
{
	TrackedData Handle;
	Handle.InternalData = PipelineLayout;
	Handle.Type = TrackedDataType::PipelineLayout;

	const u32 Index = Systems_PoolAllocator_PushData(&GeneralHandleStorage, &Handle);
	Systems_SparceHashMap_Insert(&GeneralHashMap, MixVkObjectWithVkType((u64)PipelineLayout, VkObjectType::VK_OBJECT_TYPE_PIPELINE_LAYOUT), Index);

	return (BmRender_PipelineLayout)PipelineLayout;
}

BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(const DescriptorSetLayoutData* Data)
{
	BmRender_DescriptorSetLayout Handle;
	Handle.Private = System_HandleManager_CreateHandle(DescriptorSetLayoutManager, Data);
	return Handle;
}

BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool)
{
	TrackedData Handle;
	Handle.InternalData = DescriptorPool;
	Handle.Type = TrackedDataType::DescriptorPool;

	const u32 Index = Systems_PoolAllocator_PushData(&GeneralHandleStorage, &Handle);
	Systems_SparceHashMap_Insert(&GeneralHashMap, MixVkObjectWithVkType((u64)DescriptorPool, VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_POOL), Index);

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
	TrackedData Handle;
	Handle.InternalData = ImageView;
	Handle.Type = TrackedDataType::ImageVIew;

	const u32 Index = Systems_PoolAllocator_PushData(&GeneralHandleStorage, &Handle);
	Systems_SparceHashMap_Insert(&GeneralHashMap, MixVkObjectWithVkType((u64)ImageView, VkObjectType::VK_OBJECT_TYPE_IMAGE_VIEW), Index);

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
	TrackedData Handle;
	Handle.InternalData = Fence;
	Handle.Type = TrackedDataType::Fence;

	const u32 Index = Systems_PoolAllocator_PushData(&GeneralHandleStorage, &Handle);
	Systems_SparceHashMap_Insert(&GeneralHashMap, MixVkObjectWithVkType((u64)Fence, VkObjectType::VK_OBJECT_TYPE_FENCE), Index);

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
void DestroySamplerHandle(BmRender_Sampler Handle)
{
	u32 Index;
	if (Systems_SparceHashMap_Remove(&GeneralHashMap, (u64)Handle, &Index))
	{
		Systems_PoolAllocator_FreeData(&GeneralHandleStorage, Index);
	}
}

void DestroyPipelineHandle(BmRender_Pipeline Handle)
{
	u32 Index;
	if (Systems_SparceHashMap_Remove(&GeneralHashMap, (u64)Handle, &Index))
	{
		Systems_PoolAllocator_FreeData(&GeneralHandleStorage, Index);
	}
}

void DestroyPipelineLayoutHandle(BmRender_PipelineLayout Handle)
{
	u32 Index;
	if (Systems_SparceHashMap_Remove(&GeneralHashMap, (u64)Handle, &Index))
	{
		Systems_PoolAllocator_FreeData(&GeneralHandleStorage, Index);
	}
}

void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle)
{
	System_HandleManager_DestroyHandle(DescriptorSetLayoutManager, Handle.Private);
}

void DestroyDescriptorPoolHandle(BmRender_DescriptorPool Handle)
{
	u32 Index;
	if (Systems_SparceHashMap_Remove(&GeneralHashMap, (u64)Handle, &Index))
	{
		Systems_PoolAllocator_FreeData(&GeneralHandleStorage, Index);
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

void DestroyImageViewHandle(BmRender_ImageView Handle)
{
	u32 Index;
	if (Systems_SparceHashMap_Remove(&GeneralHashMap, (u64)Handle, &Index))
	{
		Systems_PoolAllocator_FreeData(&GeneralHandleStorage, Index);
	}
}

void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle)
{
	System_HandleManager_DestroyHandle(GPUBufferManager, Handle.Private);
}



void DestroyFenceHandle(BmRender_Fence Handle)
{
	u32 Index;
	if (Systems_SparceHashMap_Remove(&GeneralHashMap, (u64)Handle, &Index))
	{
		Systems_PoolAllocator_FreeData(&GeneralHandleStorage, Index);
	}
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
DescriptorSetLayoutData* GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle)
{
	return (DescriptorSetLayoutData*)System_HandleManager_GetHandleData(DescriptorSetLayoutManager, Handle.Private);
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