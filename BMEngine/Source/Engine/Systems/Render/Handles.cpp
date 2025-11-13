#include "Handles.h"

#include <SharedLib.h>

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
	Memory_PoolAllocator Allocator;
	SparceHashMap HashMap;
};

static StoragePair DescriptorSetLayoutStorage;
static StoragePair ShaderStorage;
static StoragePair ImageStorage;
static StoragePair GPUBufferStorage;
static StoragePair DescriptorSetStorage;
static StoragePair SemaphoreStorage;
static StoragePair CommandPoolStorage;
static StoragePair CommandBufferStorage;

// INIT
void InitializeDescriptorSetLayoutManager(u32 Size)
{
	Memory_PoolAllocator_Init(&DescriptorSetLayoutStorage.Allocator, Size, sizeof(DescriptorSetLayoutData));
	Systems_SparceHashMap_Init(&DescriptorSetLayoutStorage.HashMap, Size);
}

void InitializeShaderManager(u32 Size)
{
	Memory_PoolAllocator_Init(&ShaderStorage.Allocator, Size, sizeof(ShaderData));
	Systems_SparceHashMap_Init(&ShaderStorage.HashMap, Size);
}

void InitializeImageManager(u32 Size)
{
	Memory_PoolAllocator_Init(&ImageStorage.Allocator, Size, sizeof(ImageResource));
	Systems_SparceHashMap_Init(&ImageStorage.HashMap, Size);
}

void InitializeGPUBufferManager(u32 Size)
{
	Memory_PoolAllocator_Init(&GPUBufferStorage.Allocator, Size, sizeof(GPUBufferData));
	Systems_SparceHashMap_Init(&GPUBufferStorage.HashMap, Size);
}

void InitializeDescriptorSetManager(u32 Size)
{
	Memory_PoolAllocator_Init(&DescriptorSetStorage.Allocator, Size, sizeof(DescriptorSetData));
	Systems_SparceHashMap_Init(&DescriptorSetStorage.HashMap, Size);
}

void InitializeSemaphoreManager(u32 Size)
{
	Memory_PoolAllocator_Init(&SemaphoreStorage.Allocator, Size, sizeof(SemaphoreData));
	Systems_SparceHashMap_Init(&SemaphoreStorage.HashMap, Size);
}

void InitializeCommandPoolManager(u32 Size)
{
	Memory_PoolAllocator_Init(&CommandPoolStorage.Allocator, Size, sizeof(CommandPoolData));
	Systems_SparceHashMap_Init(&CommandPoolStorage.HashMap, Size);
}

void InitializeCommandBufferManager(u32 Size)
{
	Memory_PoolAllocator_Init(&CommandBufferStorage.Allocator, Size, sizeof(CommandBufferData));
	Systems_SparceHashMap_Init(&CommandBufferStorage.HashMap, Size);
}
// INIT

// DEINIT
void DeinitDescriptorSetLayoutManager()
{
	Systems_SparceHashMap_Free(&DescriptorSetLayoutStorage.HashMap);
	Memory_PoolAllocator_Free(&DescriptorSetLayoutStorage.Allocator);
}

void DeinitShaderManager()
{
	Systems_SparceHashMap_Free(&ShaderStorage.HashMap);
	Memory_PoolAllocator_Free(&ShaderStorage.Allocator);
}

void DeinitImageManager()
{
	Systems_SparceHashMap_Free(&ImageStorage.HashMap);
	Memory_PoolAllocator_Free(&ImageStorage.Allocator);
}

void DeinitGPUBufferManager()
{
	Systems_SparceHashMap_Free(&GPUBufferStorage.HashMap);
	Memory_PoolAllocator_Free(&GPUBufferStorage.Allocator);
}

void DeinitDescriptorSetManager()
{
	Systems_SparceHashMap_Free(&DescriptorSetStorage.HashMap);
	Memory_PoolAllocator_Free(&DescriptorSetStorage.Allocator);
}

void DeinitSemaphoreManager()
{
	Systems_SparceHashMap_Free(&SemaphoreStorage.HashMap);
	Memory_PoolAllocator_Free(&SemaphoreStorage.Allocator);
}

void DeinitCommandPoolManager()
{
	Systems_SparceHashMap_Free(&CommandPoolStorage.HashMap);
	Memory_PoolAllocator_Free(&CommandPoolStorage.Allocator);
}

void DeinitCommandBufferManager()
{
	Systems_SparceHashMap_Free(&CommandBufferStorage.HashMap);
	Memory_PoolAllocator_Free(&CommandBufferStorage.Allocator);
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
	const u32 Index = Memory_PoolAllocator_PushData(&DescriptorSetLayoutStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&DescriptorSetLayoutStorage.HashMap, (u64)Layout, Index);

	return (BmRender_DescriptorSetLayout)Layout;
}

BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool)
{
	return (BmRender_DescriptorPool)DescriptorPool;
}

BmRender_Shader CreateShaderHandle(VkShaderModule VulkanShaderModule, const ShaderData* Data)
{
	const u32 Index = Memory_PoolAllocator_PushData(&ShaderStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&ShaderStorage.HashMap, (u64)VulkanShaderModule, Index);
	
	return (BmRender_Shader)VulkanShaderModule;
}

BmRender_Image CreateImageHandle(VkImage Image, const ImageResource* Data)
{
	const u32 Index = Memory_PoolAllocator_PushData(&ImageStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&ImageStorage.HashMap, (u64)Image, Index);
	
	return (BmRender_Image)Image;
}

BmRender_ImageView CreateImageViewHandle(VkImageView ImageView)
{
	return (BmRender_ImageView)ImageView;
}

BmRender_GPUBuffer CreateGPUBufferHandle(VkBuffer Buffer, const GPUBufferData* Data)
{
	const u32 Index = Memory_PoolAllocator_PushData(&GPUBufferStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&GPUBufferStorage.HashMap, (u64)Buffer, Index);
	
	return (BmRender_GPUBuffer)Buffer;
}

BmRender_DescriptorSet CreateDescriptorSetHandle(VkDescriptorSet Set, const DescriptorSetData* Data)
{
	const u32 Index = Memory_PoolAllocator_PushData(&DescriptorSetStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&DescriptorSetStorage.HashMap, (u64)Set, Index);
	
	return (BmRender_DescriptorSet)Set;
}

BmRender_Fence CreateFenceHandle(VkFence Fence)
{
	return (BmRender_Fence)Fence;
}

BmRender_Queue CreateQueueHandle(VkQueue Queue)
{
	return (BmRender_Queue)Queue;
}

BmRender_Semaphore CreateSemaphoreHandle(VkSemaphore VulkanSemaphore, const SemaphoreData* Data)
{
	const u32 Index = Memory_PoolAllocator_PushData(&SemaphoreStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&SemaphoreStorage.HashMap, (u64)VulkanSemaphore, Index);
	
	return (BmRender_Semaphore)VulkanSemaphore;
}

BmRender_CommandPool CreateCommandPoolHandle(VkCommandPool VulkanCommandPool, const CommandPoolData* Data)
{
	const u32 Index = Memory_PoolAllocator_PushData(&CommandPoolStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&CommandPoolStorage.HashMap, (u64)VulkanCommandPool, Index);
	
	return (BmRender_CommandPool)VulkanCommandPool;
}

BmRender_CommandBuffer CreateCommandBufferHandle(VkCommandBuffer VulkanCommandBuffer, const CommandBufferData* Data)
{
	const u32 Index = Memory_PoolAllocator_PushData(&CommandBufferStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&CommandBufferStorage.HashMap, (u64)VulkanCommandBuffer, Index);
	
	return (BmRender_CommandBuffer)VulkanCommandBuffer;
}
// CREATE

// DESTROY
void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle)
{
	VkDescriptorSetLayout Layout = (VkDescriptorSetLayout)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&DescriptorSetLayoutStorage.HashMap, (u64)Layout, &Index))
	{
		Memory_PoolAllocator_FreeData(&DescriptorSetLayoutStorage.Allocator, Index);
	}
}

void DestroyShaderHandle(BmRender_Shader Handle)
{
	VkShaderModule ShaderModule = (VkShaderModule)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&ShaderStorage.HashMap, (u64)ShaderModule, &Index))
	{
		Memory_PoolAllocator_FreeData(&ShaderStorage.Allocator, Index);
	}
}

void DestroyImageHandle(BmRender_Image Handle)
{
	VkImage Image = (VkImage)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&ImageStorage.HashMap, (u64)Image, &Index))
	{
		Memory_PoolAllocator_FreeData(&ImageStorage.Allocator, Index);
	}
}

void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle)
{
	VkBuffer Buffer = (VkBuffer)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&GPUBufferStorage.HashMap, (u64)Buffer, &Index))
	{
		Memory_PoolAllocator_FreeData(&GPUBufferStorage.Allocator, Index);
	}
}

void DestroySemaphoreHandle(BmRender_Semaphore Handle)
{
	VkSemaphore Semaphore = (VkSemaphore)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&SemaphoreStorage.HashMap, (u64)Semaphore, &Index))
	{
		Memory_PoolAllocator_FreeData(&SemaphoreStorage.Allocator, Index);
	}
}

void DestroyCommandPoolHandle(BmRender_CommandPool Handle)
{
	VkCommandPool CommandPool = (VkCommandPool)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&CommandPoolStorage.HashMap, (u64)CommandPool, &Index))
	{
		Memory_PoolAllocator_FreeData(&CommandPoolStorage.Allocator, Index);
	}
}

void DestroyCommandBufferHandle(BmRender_CommandBuffer Handle)
{
	VkCommandBuffer CommandBuffer = (VkCommandBuffer)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&CommandBufferStorage.HashMap, (u64)CommandBuffer, &Index))
	{
		Memory_PoolAllocator_FreeData(&CommandBufferStorage.Allocator, Index);
	}
}
// DESTROY

// GET
void GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle, DescriptorSetLayoutData* OutData)
{
	VkDescriptorSetLayout Layout = (VkDescriptorSetLayout)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Get(&DescriptorSetLayoutStorage.HashMap, (u64)Layout, &Index))
	{
		Memory_PoolAllocator_GetData(&DescriptorSetLayoutStorage.Allocator, Index, OutData);
	}
}

bool GetShaderData(BmRender_Shader Handle, ShaderData* OutData)
{
	u32 Index;
	if (Systems_SparceHashMap_Get(&ShaderStorage.HashMap, (u64)Handle, &Index))
	{
		Memory_PoolAllocator_GetData(&ShaderStorage.Allocator, Index, OutData);
		return true;
	}
	return false;
}

bool GetImageData(BmRender_Image Handle, ImageResource* OutData)
{
	u32 Index;
	if (Systems_SparceHashMap_Get(&ImageStorage.HashMap, (u64)Handle, &Index))
	{
		Memory_PoolAllocator_GetData(&ImageStorage.Allocator, Index, OutData);
		return true;
	}
	return false;
}

bool GetGPUBufferData(BmRender_GPUBuffer Handle, GPUBufferData* OutData)
{
	u32 Index;
	if (Systems_SparceHashMap_Get(&GPUBufferStorage.HashMap, (u64)Handle, &Index))
	{
		Memory_PoolAllocator_GetData(&GPUBufferStorage.Allocator, Index, OutData);
		return true;
	}
	return false;
}

bool GetDescriptorSetData(BmRender_DescriptorSet Handle, DescriptorSetData* OutData)
{
	u32 Index;
	if (Systems_SparceHashMap_Get(&DescriptorSetStorage.HashMap, (u64)Handle, &Index))
	{
		Memory_PoolAllocator_GetData(&DescriptorSetStorage.Allocator, Index, OutData);
		return true;
	}
	return false;
}

bool GetSemaphoreData(BmRender_Semaphore Handle, SemaphoreData* OutData)
{
	u32 Index;
	if (Systems_SparceHashMap_Get(&SemaphoreStorage.HashMap, (u64)Handle, &Index))
	{
		Memory_PoolAllocator_GetData(&SemaphoreStorage.Allocator, Index, OutData);
		return true;
	}
	return false;
}

bool GetCommandPoolData(BmRender_CommandPool Handle, CommandPoolData* OutData)
{
	u32 Index;
	if (Systems_SparceHashMap_Get(&CommandPoolStorage.HashMap, (u64)Handle, &Index))
	{
		Memory_PoolAllocator_GetData(&CommandPoolStorage.Allocator, Index, OutData);
		return true;
	}
	return false;
}

bool GetCommandBufferData(BmRender_CommandBuffer Handle, CommandBufferData* OutData)
{
	u32 Index;
	if (Systems_SparceHashMap_Get(&CommandBufferStorage.HashMap, (u64)Handle, &Index))
	{
		Memory_PoolAllocator_GetData(&CommandBufferStorage.Allocator, Index, OutData);
		return true;
	}
	return false;
}
// GET