#pragma once

#include <vulkan/vulkan.h>
#include <SharedLib.h>

#define MAX_DRAW_FRAMES 3
#define MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS 16

enum class BmRender_DescriptorShaderStage : u64;
enum class BmRender_PipelineShaderStage : u8;
enum class BmRender_PipelineSyncStage : u64;
enum class BmRender_ImageType : u8;
enum class BmRender_FenceStatus : u8;
enum class BmRender_WaitResult : u8;
enum class BmRender_SwapchainResult : u8;
enum class BmRender_SemaphoreType : u8;
enum class BmRender_QueueType : u32;
enum class BmRender_PipelineType : u8;
enum class BmRender_DescriptorPoolType : u32;
enum class MemoryPropertyFlag : u32;
enum class BmRender_Filter : u32;
enum class BmRender_SamplerMipmapMode : u32;
enum class BmRender_SamplerAddressMode : u32;
enum class BmRender_CompareOp : u32;
enum class BmRender_BorderColor : u32;
enum class BmRender_ImageLayout : u32;
enum class BmRender_AttachmentLoadOp : u32;
enum class BmRender_AttachmentStoreOp : u32;
enum class BmRender_DescriptorType : u32;
enum class BmRender_IndexType : u32;
enum class BmRender_Format : u32;
enum class BmRender_PolygonMode : u32;
enum class BmRender_CullModeFlags : u32;
enum class BmRender_FrontFace : u32;
enum class BmRender_ColorComponentFlags : u32;
enum class BmRender_BlendFactor : u32;
enum class BmRender_BlendOp : u32;
enum class BmRender_PrimitiveTopology : u32;
enum class BmRender_SampleCount : u32;

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

void InitBackend(void* WindowHandle);
void DeInitBackend();
