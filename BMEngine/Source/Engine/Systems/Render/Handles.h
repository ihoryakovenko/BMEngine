#pragma once

#include "RenderInterface.h"
#include "RenderTypes.h"


void InitializeGeneralHandleStorage(u32 Size);
void InitializeDescriptorSetLayoutManager(u32 Size);
void InitializeShaderManager(u32 Size);
void InitializeImageManager(u32 Size);
void InitializeGPUBufferManager(u32 Size);
void InitializeDescriptorSetManager(u32 Size);
void InitializeSemaphoreManager(u32 Size);
void InitializeCommandPoolManager(u32 Size);
void InitializeCommandBufferManager(u32 Size);

void DeinitGeneralHandleStorage(void(*CleanUpFunc)(TrackedData*));
void DeinitDescriptorSetLayoutManager(void(*CleanUpFunc)(DescriptorSetLayoutData*));
void DeinitShaderManager(void(*CleanUpFunc)(ShaderData*));
void DeinitImageManager(void(*CleanUpFunc)(ImageResource*));
void DeinitGPUBufferManager(void(*CleanUpFunc)(GPUBufferData*));
void DeinitDescriptorSetManager();
void DeinitSemaphoreManager(void(*CleanUpFunc)(SemaphoreData*));
void DeinitCommandPoolManager(void(*CleanUpFunc)(CommandPoolData*));
void DeinitCommandBufferManager();

BmRender_Sampler CreateSamplerHandle(VkSampler Sampler);
BmRender_Pipeline CreatePipelineHandle(VkPipeline Pipeline);
BmRender_PipelineLayout CreatePipelineLayoutHandle(VkPipelineLayout PipelineLayout);
BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(const DescriptorSetLayoutData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool);
BmRender_Shader CreateShaderHandle(const ShaderData* Data);
BmRender_Image CreateImageHandle(const ImageResource* Data);
BmRender_ImageView CreateImageViewHandle(VkImageView ImageView);
BmRender_GPUBuffer CreateGPUBufferHandle(const GPUBufferData* Data);
BmRender_DescriptorSet CreateDescriptorSetHandle(const DescriptorSetData* Data);
BmRender_Fence CreateFenceHandle(VkFence Fence);
BmRender_Semaphore CreateSemaphoreHandle(const SemaphoreData* Data);
BmRender_CommandPool CreateCommandPoolHandle(const CommandPoolData* Data);
BmRender_CommandBuffer CreateCommandBufferHandle(const CommandBufferData* Data);

void DestroySamplerHandle(BmRender_Sampler Handle);
void DestroyPipelineHandle(BmRender_Pipeline Handle);
void DestroyPipelineLayoutHandle(BmRender_PipelineLayout Handle);
void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle);
void DestroyDescriptorPoolHandle(BmRender_DescriptorPool Handle);
void DestroyShaderHandle(BmRender_Shader Handle);
void DestroyImageHandle(BmRender_Image Handle);
void DestroyImageViewHandle(BmRender_ImageView Handle);
void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle);
void DestroyFenceHandle(BmRender_Fence Handle);
void DestroySemaphoreHandle(BmRender_Semaphore Handle);
void DestroyCommandPoolHandle(BmRender_CommandPool Handle);
void DestroyCommandBufferHandle(BmRender_CommandBuffer Handle);

DescriptorSetLayoutData* GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle);
ShaderData* GetShaderData(BmRender_Shader Handle);
ImageResource* GetImageData(BmRender_Image Handle);
GPUBufferData* GetGPUBufferData(BmRender_GPUBuffer Handle);
DescriptorSetData* GetDescriptorSetData(BmRender_DescriptorSet Handle);
SemaphoreData* GetSemaphoreData(BmRender_Semaphore Handle);
CommandPoolData* GetCommandPoolData(BmRender_CommandPool Handle);
CommandBufferData* GetCommandBufferData(BmRender_CommandBuffer Handle);
