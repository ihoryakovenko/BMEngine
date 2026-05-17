#pragma once

#include <vulkan/vulkan.h>
#include "VulkanHelper.h"

#include "RenderInterface.h"

#include <atomic>
#include <mutex>

#include <SharedLib.h>

struct GLFWwindow;

struct VulkanCoreContext;

struct BmRender_DescriptorSetLayoutBindingData
{
	BmRender_DescriptorType DescriptorType;
};

struct BmRender_DescriptorSetLayoutData
{
	BmRender_DescriptorSetLayoutBindingData LayoutBindings[MAX_DESCRIPTOR_SET_LAYOUT_BUINDINGS];
	u32 BindingsCount;
};

struct BmRender_ImageResource
{
	BmRender_DeviceMemory Memory;
	BmRender_Format Format;
	u64 Size;
	BmRender_ImageType Type;
	BmRender_Dimensions Dimensions;
	BmRender_SampleCount SampleCount;
};

struct BmRender_ImageViewData
{
	BmRender_Image Image;
	BmRender_Format Format;
};

struct BmRender_GPUBufferData
{
	BmRender_DeviceMemory Memory;
	MemoryPropertyFlag PropertyFlag;
};

struct BmRender_DescriptorSetData
{
	BmRender_DescriptorSetLayout Layout;
};

struct BmRender_SemaphoreData
{
	BmRender_SemaphoreType Type;
};

struct BmRender_CommandPoolData
{
	u32 QueueFamilyIndex;
};

struct BmRender_CommandBufferData
{
	BmRender_CommandPool CommandPool;
};

struct BmRender_QueueData
{
	BmRender_QueueType QueueType;
};

struct BmRender_PipelineLayoutData
{
	BmRender_PipelineType PipelineType;
};

void InitializeFrameMemory();
void DeMemory_LinearAllocator_Init();

void CreateCoreContext(GLFWwindow* WindowHandler);
void DestroyCoreContext();
VulkanCoreContext* GetCoreContext();

VkAllocationCallbacks* GetVulkanAllocator();

Memory_LinearAllocator* GetFrameMemory();
