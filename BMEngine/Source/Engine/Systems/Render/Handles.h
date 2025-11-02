#pragma once

#include "RenderInterface.h"
#include "RenderTypes.h"

void InitializeSamplerManager(u32 Size);
void InitializePipelineManager(u32 Size);
void InitializePipelineLayoutManager(u32 Size);
void InitializeDescriptorSetLayoutManager(u32 Size);
void InitializeDescriptorPoolManager(u32 Size);
void InitializeShaderManager(u32 Size);
void InitializeImageManager(u32 Size);
void InitializeImageViewManager(u32 Size);
void InitializeGPUBufferManager(u32 Size);
void InitializePushConstantManager(u32 Size);
void InitializeDescriptorSetManager(u32 Size);
void InitCommandWorkerManager(u32 Size);

void DeinitSamplerManager(void(*CleanUpFunc)(SamplerData*));
void DeinitPipelineManager(void(*CleanUpFunc)(PipelineData*));
void DeinitPipelineLayoutManager(void(*CleanUpFunc)(PipelineLayoutData*));
void DeinitDescriptorSetLayoutManager(void(*CleanUpFunc)(DescriptorSetLayoutData*));
void DeinitDescriptorPoolManager(void(*CleanUpFunc)(DescriptorPoolData*));
void DeinitShaderManager(void(*CleanUpFunc)(ShaderData*));
void DeinitImageManager(void(*CleanUpFunc)(ImageResource*));
void DeinitImageViewManager(void(*CleanUpFunc)(ImageViewData*));
void DeinitGPUBufferManager(void(*CleanUpFunc)(GPUBufferData*));
void DeinitPushConstantManager();
void DeinitDescriptorSetManager();
void DeinitCommandWorkerManager(void(*CleanUpFunc)(CommandWorkerData*));

BmRender_Sampler CreateSamplerHandle(const SamplerData* Data);
BmRender_Pipeline CreatePipelineHandle(const PipelineData* Data);
BmRender_PipelineLayout CreatePipelineLayoutHandle(const PipelineLayoutData* Data);
BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(const DescriptorSetLayoutData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(const DescriptorPoolData* Data);
BmRender_Shader CreateShaderHandle(const ShaderData* Data);
BmRender_Image CreateImageHandle(const ImageResource* Data);
BmRender_ImageView CreateImageViewHandle(const ImageViewData* Data);
BmRender_GPUBuffer CreateGPUBufferHandle(const GPUBufferData* Data);
BmRender_PushConstant CreatePushConstantHandle(const PushConstantData* Data);
BmRender_DescriptorSet CreateDescriptorSetHandle(const DescriptorSetData* Data);
BmRender_CommandWorker CreateCommandWorkerHandle(const CommandWorkerData* Data);

void DestroySamplerHandle(BmRender_Sampler Handle);
void DestroyPipelineHandle(BmRender_Pipeline Handle);
void DestroyPipelineLayoutHandle(BmRender_PipelineLayout Handle);
void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle);
void DestroyDescriptorPoolHandle(BmRender_DescriptorPool Handle);
void DestroyShaderHandle(BmRender_Shader Handle);
void DestroyImageHandle(BmRender_Image Handle);
void DestroyImageViewHandle(BmRender_ImageView Handle);
void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle);
void DestroyPushConstantHandle(BmRender_PushConstant Handle);
void DestroyDescriptorSetHandle(BmRender_DescriptorSet Handle);
void DestroyCommandWorkerHandle(BmRender_CommandWorker Handle);

SamplerData* GetSamplerData(BmRender_Sampler Handle);
PipelineData* GetPipelineData(BmRender_Pipeline Handle);
PipelineLayoutData* GetPipelineLayoutData(BmRender_PipelineLayout Handle);
DescriptorSetLayoutData* GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle);
DescriptorPoolData* GetDescriptorPoolData(BmRender_DescriptorPool Handle);
ShaderData* GetShaderData(BmRender_Shader Handle);
ImageResource* GetImageData(BmRender_Image Handle);
ImageViewData* GetImageViewData(BmRender_ImageView Handle);
GPUBufferData* GetGPUBufferData(BmRender_GPUBuffer Handle);
PushConstantData* GetPushConstantData(BmRender_PushConstant Handle);
DescriptorSetData* GetDescriptorSetData(BmRender_DescriptorSet Handle);
CommandWorkerData* GetSubmitPoolData(BmRender_CommandWorker Handle);