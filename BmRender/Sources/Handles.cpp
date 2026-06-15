#include "Handles.h"

#include <SharedLib.h>

struct StoragePair
{
	Memory_PoolAllocator Allocator;
	SparceHashMap HashMap;
};

static StoragePair PipelineStorage;

// INIT
void InitializePipelineManager(u32 Size)
{
	Memory_PoolAllocator_Init(&PipelineStorage.Allocator, Size, sizeof(PipelineData));
	Systems_SparceHashMap_Init(&PipelineStorage.HashMap, Size);
}
// INIT

// DEINIT
void DeinitPipelineManager()
{
	Systems_SparceHashMap_Free(&PipelineStorage.HashMap);
	Memory_PoolAllocator_Free(&PipelineStorage.Allocator);
}
// DEINIT

// CREATE
BmRender_Sampler CreateSamplerHandle(VkSampler Sampler)
{
	return (BmRender_Sampler)Sampler;
}

BmRender_Pipeline CreatePipelineHandle(VkPipeline Pipeline, const PipelineData* Data)
{
	const u32 Index = Memory_PoolAllocator_PushData(&PipelineStorage.Allocator, Data);
	Systems_SparceHashMap_Insert(&PipelineStorage.HashMap, (u64)Pipeline, Index);

	return (BmRender_Pipeline)Pipeline;
}

BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool)
{
	return (BmRender_DescriptorPool)DescriptorPool;
}

BmRender_Shader CreateShaderHandle(VkShaderModule VulkanShaderModule)
{
	return (BmRender_Shader)VulkanShaderModule;
}



BmRender_Fence CreateFenceHandle(VkFence Fence)
{
	return (BmRender_Fence)Fence;
}

BmRender_DeviceMemory CreateDeviceMemoryHandle(VkDeviceMemory Memory)
{
	return (BmRender_DeviceMemory)Memory;
}
// CREATE

// DESTROY
void DestroyPipelineData(BmRender_Pipeline Handle)
{
	VkPipeline Pipeline = (VkPipeline)Handle;
	u32 Index;
	if (Systems_SparceHashMap_Remove(&PipelineStorage.HashMap, (u64)Pipeline, &Index))
	{
		Memory_PoolAllocator_FreeData(&PipelineStorage.Allocator, Index);
	}
}
// DESTROY

// GET

bool BmRender_GetPipelineData(BmRender_Pipeline Handle, PipelineData* OutData)
{
	u32 Index;
	if (Systems_SparceHashMap_Get(&PipelineStorage.HashMap, (u64)Handle, &Index))
	{
		Memory_PoolAllocator_GetData(&PipelineStorage.Allocator, Index, OutData);
		return true;
	}
	return false;
}
// GET