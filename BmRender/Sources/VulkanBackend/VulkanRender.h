#pragma once

#include <vulkan/vulkan.h>
#include <SharedLib.h>
#include <Sources/RenderInternal.h>

struct GLFWwindow;

typedef VkInstance BmRender_Instance;
typedef VkPhysicalDevice BmRender_PhysicalDevice;
typedef VkDevice BmRender_Device;
typedef VkSampler BmRender_Sampler;
typedef VkDescriptorPool BmRender_DescriptorPool;
typedef VkShaderModule BmRender_Shader;
typedef VkFence BmRender_Fence;
typedef VkDeviceMemory BmRender_DeviceMemory;

struct BmRender_DescriptorSetLayout
{
	VkDescriptorSetLayout InternalLayout;
	BmRender_DescriptorType LayoutBindings[MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS];
	u32 BindingsCount;
};

struct BmRender_Image
{
	VkImage InternalImage;
	BmRender_DeviceMemory Memory;
	BmRender_Format Format;
	u64 Size;
	BmRender_ImageType Type;
	u32 Width;
	u32 Height;
	BmRender_SampleCount SampleCount;
};

struct BmRender_GPUBuffer
{
	VkBuffer InternalBuffer;
	BmRender_DeviceMemory Memory;
	MemoryPropertyFlag PropertyFlag;
};

struct BmRender_DescriptorSet
{
	VkDescriptorSet InternalSet;
	BmRender_DescriptorSetLayout* Layout;
};

struct BmRender_Semaphore
{
	VkSemaphore InternalSemaphore;
	BmRender_SemaphoreType Type;
};

struct BmRender_CommandPool
{
	VkCommandPool InternalPool;
	u32 QueueFamilyIndex;
};

struct BmRender_CommandBuffer
{
	VkCommandBuffer InternalBuffer;
	const BmRender_CommandPool* CommandPool;
};

struct BmRender_Queue
{
	VkQueue InternalQueue;
	BmRender_QueueType QueueType;
};

struct BmRender_PipelineLayout
{
	VkPipelineLayout InternalLayout;
};

struct BmRender_ImageView
{
	VkImageView InternalView;
	const BmRender_Image* Image;
	BmRender_Format Format;
};

struct BmRender_Pipeline
{
	VkPipeline InternalPipeline;
	BmRender_PipelineType PipelineType;
};

void InitBackend(GLFWwindow* WindowHandler);
void DeInitBackend();
