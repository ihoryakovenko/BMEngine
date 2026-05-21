#pragma once

#include <vulkan/vulkan.h>
#include "VulkanHelper.h"

#include "RenderInterface.h"

#include <atomic>
#include <mutex>

#include <SharedLib.h>

struct GLFWwindow;

struct VulkanCoreContext;

struct DescriptorSetLayoutBindingData
{
	BmRender_DescriptorType DescriptorType;
};

struct DescriptorSetLayoutData
{
	DescriptorSetLayoutBindingData LayoutBindings[MAX_DESCRIPTOR_SET_LAYOUT_BUINDINGS];
	u32 BindingsCount;
};

struct ImageData
{
	BmRender_DeviceMemory Memory;
	BmRender_Format Format;
	u64 Size;
	BmRender_ImageType Type;
	BmRender_Dimensions Dimensions;
	BmRender_SampleCount SampleCount;
};

struct ImageViewData
{
	BmRender_Image Image;
	BmRender_Format Format;
};

struct BufferData
{
	BmRender_DeviceMemory Memory;
	MemoryPropertyFlag PropertyFlag;
};

struct DescriptorSetData
{
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

struct QueueData
{
	BmRender_QueueType QueueType;
};

struct PipelineLayoutData
{
	BmRender_PipelineType PipelineType;
};

struct PipelineData
{
	BmRender_PipelineLayout Layout;
};

void InitializeFrameMemory();
void DeMemory_LinearAllocator_Init();

void CreateCoreContext(GLFWwindow* WindowHandler);
void DestroyCoreContext();
VulkanCoreContext* GetCoreContext();

VkAllocationCallbacks* GetVulkanAllocator();

Memory_LinearAllocator* GetFrameMemory();
