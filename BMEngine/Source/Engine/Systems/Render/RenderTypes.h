#pragma once

#include <vulkan/vulkan.h>
#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "RenderInterface.h"

#include <atomic>
#include <mutex>

#include <SharedLib.h>

struct GLFWwindow;

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
	DescriptorSetLayoutBinding* LayoutBindings;
	u32 BindingsCount;
};


struct ShaderData
{
	BmRender_PipelineShaderStage Stage;
};

struct ImageResource
{
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
	BmRender_SemaphoreType Type;
};

struct CommandPoolData
{
	u32 QueueFamilyIndex;
};

struct CommandBufferData
{
	BmRender_CommandPool CommandPool;
};


void InitializeFrameMemory();
void DeMemory_LinearAllocator_Init();

void CreateCoreContext(GLFWwindow* WindowHandler);
void DestroyCoreContext();
VulkanCoreContext::VulkanCoreContext* GetCoreContext();

VkAllocationCallbacks* GetVulkanAllocator();

Memory_LinearAllocator* GetFrameMemory();
