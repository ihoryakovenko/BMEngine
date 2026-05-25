#pragma once

#include <vulkan/vulkan.h>

#include <ShortTypes.h>

#include "RenderInterface.h"
#include "RenderTypes.h"

void InitializeDescriptorSetLayoutManager(u32 Size);
void InitializeImageManager(u32 Size);
void InitializeGPUBufferManager(u32 Size);
void InitializeDescriptorSetManager(u32 Size);
void InitializeSemaphoreManager(u32 Size);
void InitializeCommandPoolManager(u32 Size);
void InitializeCommandBufferManager(u32 Size);
void InitializeQueueManager(u32 Size);
void InitializePipelineLayoutManager(u32 Size);
void InitializeImageViewManager(u32 Size);
void InitializePipelineManager(u32 Size);

void DeinitDescriptorSetLayoutManager();
void DeinitImageManager();
void DeinitGPUBufferManager();
void DeinitDescriptorSetManager();
void DeinitSemaphoreManager();
void DeinitCommandPoolManager();
void DeinitCommandBufferManager();
void DeinitQueueManager();
void DeinitPipelineLayoutManager();
void DeinitImageViewManager();
void DeinitPipelineManager();

BmRender_Sampler CreateSamplerHandle(VkSampler Sampler);
BmRender_Pipeline CreatePipelineHandle(VkPipeline Pipeline, const PipelineData* Data);
BmRender_PipelineLayout CreatePipelineLayoutHandle(VkPipelineLayout PipelineLayout, const PipelineLayoutData* Data);
BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(VkDescriptorSetLayout Layout, const DescriptorSetLayoutData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool);
BmRender_Shader CreateShaderHandle(VkShaderModule VulkanShaderModule);
BmRender_Image CreateImageHandle(VkImage Image, const ImageData* Data);
BmRender_ImageView CreateImageViewHandle(VkImageView ImageView, const ImageViewData* Data);
BmRender_GPUBuffer CreateGPUBufferHandle(VkBuffer Buffer, const BufferData* Data);
BmRender_DescriptorSet CreateDescriptorSetHandle(VkDescriptorSet Set, const DescriptorSetData* Data);
BmRender_Fence CreateFenceHandle(VkFence Fence);
BmRender_Semaphore CreateSemaphoreHandle(VkSemaphore VulkanSemaphore, const SemaphoreData* Data);
BmRender_CommandPool CreateCommandPoolHandle(VkCommandPool VulkanCommandPool, const CommandPoolData* Data);
BmRender_CommandBuffer CreateCommandBufferHandle(VkCommandBuffer VulkanCommandBuffer, const CommandBufferData* Data);
BmRender_Queue CreateQueueHandle(VkQueue Queue, const QueueData* Data);
BmRender_DeviceMemory CreateDeviceMemoryHandle(VkDeviceMemory Memory);

void BmRender_GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle, DescriptorSetLayoutData* OutData);
bool BmRender_GetImageData(BmRender_Image Handle, ImageData* OutData);
bool BmRender_GetGPUBufferData(BmRender_GPUBuffer Handle, BufferData* OutData);
bool BmRender_GetDescriptorSetData(BmRender_DescriptorSet Handle, DescriptorSetData* OutData);
bool BmRender_GetSemaphoreData(BmRender_Semaphore Handle, SemaphoreData* OutData);
bool BmRender_GetCommandPoolData(BmRender_CommandPool Handle, CommandPoolData* OutData);
bool BmRender_GetCommandBufferData(BmRender_CommandBuffer Handle, CommandBufferData* OutData);
bool BmRender_GetQueueData(BmRender_Queue Handle, QueueData* OutData);
bool BmRender_GetPipelineLayoutData(BmRender_PipelineLayout Handle, PipelineLayoutData* OutData);
bool BmRender_GetImageViewData(BmRender_ImageView Handle, ImageViewData* OutData);
bool BmRender_GetPipelineData(BmRender_Pipeline Handle, PipelineData* OutData);

void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout Handle);
void DestroyImageHandle(BmRender_Image Handle);
void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle);
void DestroySemaphoreHandle(BmRender_Semaphore Handle);
void DestroyCommandPoolHandle(BmRender_CommandPool Handle);
void DestroyCommandBufferHandle(BmRender_CommandBuffer Handle);
void DestroyPipelineData(BmRender_Pipeline Handle);
