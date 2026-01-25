#pragma once

#include <vulkan/vulkan.h>

#include <ShortTypes.h>

#include "RenderInterface.h"

void InitializeDescriptorSetLayoutManager(u32 Size);
void InitializeShaderManager(u32 Size);
void InitializeImageManager(u32 Size);
void InitializeGPUBufferManager(u32 Size);
void InitializeDescriptorSetManager(u32 Size);
void InitializeSemaphoreManager(u32 Size);
void InitializeCommandPoolManager(u32 Size);
void InitializeCommandBufferManager(u32 Size);
void InitializeQueueManager(u32 Size);
void InitializePipelineLayoutManager(u32 Size);
void InitializeImageViewManager(u32 Size);

void DeinitDescriptorSetLayoutManager();
void DeinitShaderManager();
void DeinitImageManager();
void DeinitGPUBufferManager();
void DeinitDescriptorSetManager();
void DeinitSemaphoreManager();
void DeinitCommandPoolManager();
void DeinitCommandBufferManager();
void DeinitQueueManager();
void DeinitPipelineLayoutManager();
void DeinitImageViewManager();

BmRender_Sampler CreateSamplerHandle(VkSampler Sampler);
BmRender_Pipeline CreatePipelineHandle(VkPipeline Pipeline);
BmRender_PipelineLayout CreatePipelineLayoutHandle(VkPipelineLayout PipelineLayout, const BmRender_PipelineLayoutData* Data);
BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(VkDescriptorSetLayout Layout, const BmRender_DescriptorSetLayoutData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool);
BmRender_Shader CreateShaderHandle(VkShaderModule VulkanShaderModule, const BmRender_ShaderData* Data);
BmRender_Image CreateImageHandle(VkImage Image, const BmRender_ImageResource* Data);
BmRender_ImageView CreateImageViewHandle(VkImageView ImageView, const BmRender_ImageViewData* Data);
BmRender_GPUBuffer CreateGPUBufferHandle(VkBuffer Buffer, const BmRender_GPUBufferData* Data);
BmRender_DescriptorSet CreateDescriptorSetHandle(VkDescriptorSet Set, const BmRender_DescriptorSetData* Data);
BmRender_Fence CreateFenceHandle(VkFence Fence);
BmRender_Semaphore CreateSemaphoreHandle(VkSemaphore VulkanSemaphore, const BmRender_SemaphoreData* Data);
BmRender_CommandPool CreateCommandPoolHandle(VkCommandPool VulkanCommandPool, const BmRender_CommandPoolData* Data);
BmRender_CommandBuffer CreateCommandBufferHandle(VkCommandBuffer VulkanCommandBuffer, const BmRender_CommandBufferData* Data);
BmRender_Queue CreateQueueHandle(VkQueue Queue, const BmRender_QueueData* Data);
BmRender_DeviceMemory CreateDeviceMemoryHandle(VkDeviceMemory Memory);

void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle);
void DestroyShaderHandle(BmRender_Shader Handle);
void DestroyImageHandle(BmRender_Image Handle);
void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle);
void DestroySemaphoreHandle(BmRender_Semaphore Handle);
void DestroyCommandPoolHandle(BmRender_CommandPool Handle);
void DestroyCommandBufferHandle(BmRender_CommandBuffer Handle);

