#include "Handles.h"

static System_HandleManager SamplerManager;
static System_HandleManager PipelineManager;
static System_HandleManager PipelineLayoutManager;
static System_HandleManager DescriptorSetLayoutManager;
static System_HandleManager DescriptorPoolManager;
static System_HandleManager ShaderManager;
static System_HandleManager ImageManager;
static System_HandleManager ImageViewManager;
static System_HandleManager GPUBufferManager;
static System_HandleManager PushConstantsManager;
static System_HandleManager DescriptorSetManager;
static System_HandleManager CommandWorkerManager;

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

void InitializeImageViewManager(u32 Size)
{
	ImageViewManager = System_HandleManager_InitData(Size, sizeof(ImageViewData), GetNextHandleType());
}

void InitializeGPUBufferManager(u32 Size)
{
	GPUBufferManager = System_HandleManager_InitData(Size, sizeof(GPUBufferData), GetNextHandleType());
}

void InitializePushConstantManager(u32 Size)
{
	PushConstantsManager = System_HandleManager_InitData(Size, sizeof(PushConstantData), GetNextHandleType());
}

void InitializeDescriptorSetManager(u32 Size)
{
	DescriptorSetManager = System_HandleManager_InitData(Size, sizeof(DescriptorSetData), GetNextHandleType());
}

void InitCommandWorkerManager(u32 Size)
{
	CommandWorkerManager = System_HandleManager_InitData(Size, sizeof(CommandWorkerData), GetNextHandleType());
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

void DeinitImageViewManager(void(*CleanUpFunc)(ImageViewData*))
{
	System_HandleManager_ClearData(ImageViewManager, (void(*)(void*))CleanUpFunc);
}

void DeinitGPUBufferManager(void(*CleanUpFunc)(GPUBufferData*))
{
	System_HandleManager_ClearData(GPUBufferManager, (void(*)(void*))CleanUpFunc);
}

void DeinitPushConstantManager()
{
	System_HandleManager_ClearData(PushConstantsManager);
}

void DeinitDescriptorSetManager()
{
	System_HandleManager_ClearData(DescriptorSetManager);
}

void DeinitCommandWorkerManager(void(*CleanUpFunc)(CommandWorkerData*))
{
	System_HandleManager_ClearData(CommandWorkerManager, (void(*)(void*))CleanUpFunc);
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

BmRender_ImageView CreateImageViewHandle(const ImageViewData* Data)
{
	BmRender_ImageView Handle;
	Handle.Private = System_HandleManager_CreateHandle(ImageViewManager, Data);
	return Handle;
}

BmRender_GPUBuffer CreateGPUBufferHandle(const GPUBufferData* Data)
{
	BmRender_GPUBuffer Handle;
	Handle.Private = System_HandleManager_CreateHandle(GPUBufferManager, Data);
	return Handle;
}

BmRender_PushConstant CreatePushConstantHandle(const PushConstantData* Data)
{
	BmRender_PushConstant Handle;
	Handle.Private = System_HandleManager_CreateHandle(PushConstantsManager, Data);
	return Handle;
}

BmRender_DescriptorSet CreateDescriptorSetHandle(const DescriptorSetData* Data)
{
	BmRender_DescriptorSet Handle;
	Handle.Private = System_HandleManager_CreateHandle(DescriptorSetManager, Data);
	return Handle;
}

BmRender_CommandWorker CreateCommandWorkerHandle(const CommandWorkerData* Data)
{
	BmRender_CommandWorker Handle;
	Handle.Private = System_HandleManager_CreateHandle(CommandWorkerManager, Data);
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

void DestroyImageViewHandle(BmRender_ImageView Handle)
{
	System_HandleManager_DestroyHandle(ImageViewManager, Handle.Private);
}

void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle)
{
	System_HandleManager_DestroyHandle(GPUBufferManager, Handle.Private);
}


void DestroyPushConstantHandle(BmRender_PushConstant Handle)
{
	System_HandleManager_DestroyHandle(PushConstantsManager, Handle.Private);
}

void DestroyDescriptorSetHandle(BmRender_DescriptorSet Handle)
{
	System_HandleManager_DestroyHandle(DescriptorSetManager, Handle.Private);
}

void DestroyCommandWorkerHandle(BmRender_CommandWorker Handle)
{
	System_HandleManager_DestroyHandle(CommandWorkerManager, Handle.Private);
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

ImageViewData* GetImageViewData(BmRender_ImageView Handle)
{
	return (ImageViewData*)System_HandleManager_GetHandleData(ImageViewManager, Handle.Private);
}

GPUBufferData* GetGPUBufferData(BmRender_GPUBuffer Handle)
{
	return (GPUBufferData*)System_HandleManager_GetHandleData(GPUBufferManager, Handle.Private);
}

PushConstantData* GetPushConstantData(BmRender_PushConstant Handle)
{
	return (PushConstantData*)System_HandleManager_GetHandleData(PushConstantsManager, Handle.Private);
}

DescriptorSetData* GetDescriptorSetData(BmRender_DescriptorSet Handle)
{
	return (DescriptorSetData*)System_HandleManager_GetHandleData(DescriptorSetManager, Handle.Private);
}

CommandWorkerData* GetSubmitPoolData(BmRender_CommandWorker Handle)
{
	return (CommandWorkerData*)System_HandleManager_GetHandleData(CommandWorkerManager, Handle.Private);
}
// GET