#pragma once

#include "RenderInterface.h"
#include "RenderTypes.h"


void InitializeDescriptorSetLayoutManager(u32 Size);
void InitializeShaderManager(u32 Size);
void InitializeImageManager(u32 Size);
void InitializeGPUBufferManager(u32 Size);
void InitializeDescriptorSetManager(u32 Size);
void InitializeSemaphoreManager(u32 Size);
void InitializeCommandPoolManager(u32 Size);
void InitializeCommandBufferManager(u32 Size);

void DeinitDescriptorSetLayoutManager();
void DeinitShaderManager();
void DeinitImageManager();
void DeinitGPUBufferManager();
void DeinitDescriptorSetManager();
void DeinitSemaphoreManager();
void DeinitCommandPoolManager();
void DeinitCommandBufferManager();

BmRender_Sampler CreateSamplerHandle(VkSampler Sampler);
BmRender_Pipeline CreatePipelineHandle(VkPipeline Pipeline);
BmRender_PipelineLayout CreatePipelineLayoutHandle(VkPipelineLayout PipelineLayout);
BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(VkDescriptorSetLayout Layout, const DescriptorSetLayoutData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool);
BmRender_Shader CreateShaderHandle(VkShaderModule VulkanShaderModule, const ShaderData* Data);
BmRender_Image CreateImageHandle(VkImage Image, const ImageResource* Data);
BmRender_ImageView CreateImageViewHandle(VkImageView ImageView);
BmRender_GPUBuffer CreateGPUBufferHandle(VkBuffer Buffer, const GPUBufferData* Data);
BmRender_DescriptorSet CreateDescriptorSetHandle(VkDescriptorSet Set, const DescriptorSetData* Data);
BmRender_Fence CreateFenceHandle(VkFence Fence);
BmRender_Semaphore CreateSemaphoreHandle(VkSemaphore VulkanSemaphore, const SemaphoreData* Data);
BmRender_CommandPool CreateCommandPoolHandle(VkCommandPool VulkanCommandPool, const CommandPoolData* Data);
BmRender_CommandBuffer CreateCommandBufferHandle(VkCommandBuffer VulkanCommandBuffer, const CommandBufferData* Data);
BmRender_Queue CreateQueueHandle(VkQueue Queue);

void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle);
void DestroyShaderHandle(BmRender_Shader Handle);
void DestroyImageHandle(BmRender_Image Handle);
void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle);
void DestroySemaphoreHandle(BmRender_Semaphore Handle);
void DestroyCommandPoolHandle(BmRender_CommandPool Handle);
void DestroyCommandBufferHandle(BmRender_CommandBuffer Handle);

void GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle, DescriptorSetLayoutData* OutData);
bool GetShaderData(BmRender_Shader Handle, ShaderData* OutData);
bool GetImageData(BmRender_Image Handle, ImageResource* OutData);
bool GetGPUBufferData(BmRender_GPUBuffer Handle, GPUBufferData* OutData);
bool GetDescriptorSetData(BmRender_DescriptorSet Handle, DescriptorSetData* OutData);
bool GetSemaphoreData(BmRender_Semaphore Handle, SemaphoreData* OutData);
bool GetCommandPoolData(BmRender_CommandPool Handle, CommandPoolData* OutData);
bool GetCommandBufferData(BmRender_CommandBuffer Handle, CommandBufferData* OutData);
