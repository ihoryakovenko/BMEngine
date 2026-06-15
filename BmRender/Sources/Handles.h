#pragma once

#include <vulkan/vulkan.h>

#include <ShortTypes.h>

#include "RenderInterface.h"
#include "RenderTypes.h"

void InitializePipelineManager(u32 Size);


void DeinitPipelineManager();

BmRender_Sampler CreateSamplerHandle(VkSampler Sampler);
BmRender_Pipeline CreatePipelineHandle(VkPipeline Pipeline, const PipelineData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(VkDescriptorPool DescriptorPool);
BmRender_Shader CreateShaderHandle(VkShaderModule VulkanShaderModule);

BmRender_Fence CreateFenceHandle(VkFence Fence);
BmRender_DeviceMemory CreateDeviceMemoryHandle(VkDeviceMemory Memory);


bool BmRender_GetPipelineData(BmRender_Pipeline Handle, PipelineData* OutData);

void DestroyPipelineData(BmRender_Pipeline Handle);
