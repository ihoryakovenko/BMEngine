#pragma once

#include <vulkan/vulkan.h>
#include "Engine/Systems/HandleManager.h"
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "RenderInterface.h"

#include <atomic>
#include <mutex>

struct GLFWwindow;

enum class TrackedDataType
{
	Sampler,
	Pipeline,
	PipelineLayout,
	DescriptorPool,
	Fence,
	ImageVIew,
};

struct TrackedData
{
	TrackedDataType Type;
	void* InternalData;
};

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}


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
	BmRender_ImageType Type;
	u32 Width;
	u32 Height;
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


struct SemaphoreData
{
	VkSemaphore VulkanSemaphore;
	BmRender_SemaphoreType Type;
};

struct CommandPoolData
{
	VkCommandPool VulkanCommandPool;
	u32 QueueFamilyIndex;
};

struct CommandBufferData
{
	VkCommandBuffer VulkanCommandBuffer;
	BmRender_CommandPool CommandPool;
};

void DestroyTrackedData(TrackedData* Data);
void OnDescriptorSetLayoutClear(DescriptorSetLayoutData* LayoutData);
void OnShaderClear(ShaderData* Shader);
void OnImageClear(ImageResource* Image);
void OnGPUBufferClear(GPUBufferData* Data);
void OnSemaphoreClear(SemaphoreData* Semaphore);
void OnCommandPoolClear(CommandPoolData* CommandPool);

void InitializeFrameMemory();
void DeinitFrameMemory();

void CreateCoreContext(GLFWwindow* WindowHandler);
void DestroyCoreContext();
VulkanCoreContext::VulkanCoreContext* GetCoreContext();

VkAllocationCallbacks* GetVulkanAllocator();

Memory::FrameMemory GetFrameMemory();
