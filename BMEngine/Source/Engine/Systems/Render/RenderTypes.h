#pragma once

#include <vulkan/vulkan.h>
#include "Engine/Systems/HandleManager.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "RenderInterface.h"

#include <atomic>
#include <mutex>

struct GLFWwindow;

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}


struct SamplerData
{
	VkSampler VulkanSampler;
};

struct PipelineData
{
	VkPipeline VulkanPipeline;
};

struct PipelineLayoutData
{
	VkPipelineLayout VulkanPipelineLayout;
};

struct DescriptorSetLayoutBinding
{
	VkDescriptorType DescriptorType;
};

struct DescriptorSetLayoutData
{
	VkDescriptorSetLayout Layout;
	DescriptorSetLayoutBinding* LayoutBindings;
	u32 BindingsCount;
};

struct DescriptorPoolData
{
	VkDescriptorPool VulkanDescriptorPool;
};

struct ShaderData
{
	VkShaderModule VulkanShaderModule;
	BmRender_PipelineShaderStage Stage;
};

struct ImageResource
{
	VkImage Image;
	VkDeviceMemory Memory;
	u64 ReadyValue;
	VkFormat Format;
	u64 Size;
	u32 Width;
	u32 Height;
};

struct ImageViewData
{
	VkImageView View;
};

struct GPUBufferData
{
	VkBuffer Buffer;
	VkDeviceMemory Memory;
	MemoryPropertyFlag PropertyFlag;
	BmRender_PipelineSyncStage BufferStage;
	u64 ReadyValue;
};

struct DescriptorSetData
{
	VkDescriptorSet Set;
	BmRender_DescriptorSetLayout Layout;
};

struct PushConstantData
{
	VkPushConstantRange PushConstants;
};

struct CommandWorkerData
{
	VkCommandPool CommandPool;
	VkCommandBuffer CommandBuffer;
	VkFence Fence;
	std::atomic_bool IsLocked;
};

void OnSamplerClear(SamplerData* SamplerData);
void OnPipelineClear(PipelineData* PipelineData);
void OnPipelineLayoutClear(PipelineLayoutData* LayoutData);
void OnDescriptorSetLayoutClear(DescriptorSetLayoutData* LayoutData);
void OnDescriptorPoolClear(DescriptorPoolData* PoolData);
void OnShaderClear(ShaderData* Shader);
void OnImageClear(ImageResource* Image);
void OnImageViewClear(ImageViewData* Data);
void OnGPUBufferClear(GPUBufferData* Data);
void OnComandWorkerClear(CommandWorkerData* PoolData);

void InitializeFrameMemory();
void DeinitFrameMemory();

void CreateCoreContext(GLFWwindow* WindowHandler);
void DestroyCoreContext();
VulkanCoreContext::VulkanCoreContext* GetCoreContext();

VkAllocationCallbacks* GetVulkanAllocator();

Memory::FrameMemory GetFrameMemory();
