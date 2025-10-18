#include "RenderTypes.h"

#include <Engine/Systems/HandleManager.h>

System_HandleManager SamplerManager;
System_HandleManager PipelineManager;
System_HandleManager PipelineLayoutManager;
System_HandleManager DescriptorSetLayoutManager;
System_HandleManager DescriptorPoolManager;
System_HandleManager ShaderManager;
System_HandleManager ImageManager;

static u16 GetNextHandleType()
{
	static u16 HandleType = 0;
	return HandleType++;
}

// INIT
void InitializeSamplerManager(u32 Size)
{
	SamplerManager = System_HandleManager_InitData(Size, sizeof(SamplerData), GetNextHandleType());
}

void InitializePipelineManager(u32 Size)
{
	PipelineManager = System_HandleManager_InitData(Size, sizeof(PipelineData), GetNextHandleType());
}

void InitializePipelineLayoutManager(u32 Size)
{
	PipelineLayoutManager = System_HandleManager_InitData(Size, sizeof(PipelineLayoutData), GetNextHandleType());
}

void InitializeDescriptorSetLayoutManager(u32 Size)
{
	DescriptorSetLayoutManager = System_HandleManager_InitData(Size, sizeof(DescriptorSetLayoutData), GetNextHandleType());
}

void InitializeDescriptorPoolManager(u32 Size)
{
	DescriptorPoolManager = System_HandleManager_InitData(Size, sizeof(DescriptorPoolData), GetNextHandleType());
}

void InitializeShaderManager(u32 Size)
{
	ShaderManager = System_HandleManager_InitData(Size, sizeof(ShaderData), GetNextHandleType());
}

void InitializeImageManager(u32 Size)
{
	ImageManager = System_HandleManager_InitData(Size, sizeof(ImageResource), GetNextHandleType());
}
// INIT

// DEINIT
void DeinitSamplerManager(void(*CleanupFunc)(SamplerData*))
{
	System_HandleManager_ClearData(SamplerManager, (void(*)(void*))CleanupFunc);
}

void DeinitPipelineManager(void(*CleanupFunc)(PipelineData*))
{
	System_HandleManager_ClearData(PipelineManager, (void(*)(void*))CleanupFunc);
}

void DeinitPipelineLayoutManager(void(*CleanupFunc)(PipelineLayoutData*))
{
	System_HandleManager_ClearData(PipelineLayoutManager, (void(*)(void*))CleanupFunc);
}

void DeinitDescriptorSetLayoutManager(void(*CleanupFunc)(DescriptorSetLayoutData*))
{
	System_HandleManager_ClearData(DescriptorSetLayoutManager, (void(*)(void*))CleanupFunc);
}

void DeinitDescriptorPoolManager(void(*CleanupFunc)(DescriptorPoolData*))
{
	System_HandleManager_ClearData(DescriptorPoolManager, (void(*)(void*))CleanupFunc);
}

void DeinitShaderManager(void(*CleanupFunc)(ShaderData*))
{
	System_HandleManager_ClearData(ShaderManager, (void(*)(void*))CleanupFunc);
}

void DeinitImageManager(void(*CleanupFunc)(ImageResource*))
{
	System_HandleManager_ClearData(ImageManager, (void(*)(void*))CleanupFunc);
}
// DEINIT

// CREATE
BmRender_Sampler CreateSamplerHandle(const SamplerData* Data)
{
	BmRender_Sampler Handle;
	Handle.Private = System_HandleManager_CreateHandle(SamplerManager, Data);
	return Handle;
}

BmRender_Pipeline CreatePipelineHandle(const PipelineData* Data)
{
	BmRender_Pipeline Handle;
	Handle.Private = System_HandleManager_CreateHandle(PipelineManager, Data);
	return Handle;
}

BmRender_PipelineLayout CreatePipelineLayoutHandle(const PipelineLayoutData* Data)
{
	BmRender_PipelineLayout Handle;
	Handle.Private = System_HandleManager_CreateHandle(PipelineLayoutManager, Data);
	return Handle;
}

BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(const DescriptorSetLayoutData* Data)
{
	BmRender_DescriptorSetLayout Handle;
	Handle.Private = System_HandleManager_CreateHandle(DescriptorSetLayoutManager, Data);
	return Handle;
}

BmRender_DescriptorPool CreateDescriptorPoolHandle(const DescriptorPoolData* Data)
{
	BmRender_DescriptorPool Handle;
	Handle.Private = System_HandleManager_CreateHandle(DescriptorPoolManager, Data);
	return Handle;
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
// CREATE

// DESTROY
void DestroySamplerHandle(BmRender_Sampler Handle)
{
	System_HandleManager_DestroyHandle(SamplerManager, Handle.Private);
}

void DestroyPipelineHandle(BmRender_Pipeline Handle)
{
	System_HandleManager_DestroyHandle(PipelineManager, Handle.Private);
}

void DestroyPipelineLayoutHandle(BmRender_PipelineLayout Handle)
{
	System_HandleManager_DestroyHandle(PipelineLayoutManager, Handle.Private);
}

void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle)
{
	System_HandleManager_DestroyHandle(DescriptorSetLayoutManager, Handle.Private);
}

void DestroyDescriptorPoolHandle(BmRender_DescriptorPool Handle)
{
	System_HandleManager_DestroyHandle(DescriptorPoolManager, Handle.Private);
}

void DestroyShaderHandle(BmRender_Shader Handle)
{
	System_HandleManager_DestroyHandle(ShaderManager, Handle.Private);
}

void DestroyImageHandle(BmRender_Image Handle)
{
	System_HandleManager_DestroyHandle(ImageManager, Handle.Private);
}
// DESTROY

// GET
SamplerData* GetSamplerData(BmRender_Sampler Handle)
{
	return (SamplerData*)System_HandleManager_GetHandleData(SamplerManager, Handle.Private);
}

PipelineData* GetPipelineData(BmRender_Pipeline Handle)
{
	return (PipelineData*)System_HandleManager_GetHandleData(PipelineManager, Handle.Private);
}

PipelineLayoutData* GetPipelineLayoutData(BmRender_PipelineLayout Handle)
{
	return (PipelineLayoutData*)System_HandleManager_GetHandleData(PipelineLayoutManager, Handle.Private);
}

DescriptorSetLayoutData* GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle)
{
	return (DescriptorSetLayoutData*)System_HandleManager_GetHandleData(DescriptorSetLayoutManager, Handle.Private);
}

DescriptorPoolData* GetDescriptorPoolData(BmRender_DescriptorPool Handle)
{
	return (DescriptorPoolData*)System_HandleManager_GetHandleData(DescriptorPoolManager, Handle.Private);
}

ShaderData* GetShaderData(BmRender_Shader Handle)
{
	return (ShaderData*)System_HandleManager_GetHandleData(ShaderManager, Handle.Private);
}

ImageResource* GetImageData(BmRender_Image Handle)
{
	return (ImageResource*)System_HandleManager_GetHandleData(ImageManager, Handle.Private);
}
// GET
