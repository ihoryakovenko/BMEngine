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

struct DrawSystemData
{
	VkSemaphore ImagesAvailable[VulkanHelper::MAX_DRAW_FRAMES];
	VkSemaphore RenderFinished[VulkanHelper::MAX_DRAW_FRAMES];

	u32 CurrentFrame;

	u64 WaitSemaphoreValueCount;
};

struct CommandSystemData
{
	VkQueue GraphicsQueue;
	std::mutex QueueSubmitMutex;
	System_HandleManager WorkerManager;
	BmRender_CommandWorker Workers[16];
	u32 WorkerCount;
	u32 FreeWorkerCount;
};

struct CommandWorkerData
{
	VkCommandPool CommandPool;
	VkCommandBuffer CommandBuffer;
	VkFence Fence;
};

void CreateCoreContext(GLFWwindow* WindowHandler);
void DestroyCoreContext();
VulkanCoreContext::VulkanCoreContext* GetCoreContext();

VkAllocationCallbacks* GetVulkanAllocator();

void InitializeSamplerManager(u32 Size);
void InitializePipelineManager(u32 Size);
void InitializePipelineLayoutManager(u32 Size);
void InitializeDescriptorSetLayoutManager(u32 Size);
void InitializeDescriptorPoolManager(u32 Size);
void InitializeShaderManager(u32 Size);
void InitializeImageManager(u32 Size);
void InitializeImageViewManager(u32 Size);
void InitializeGPUBufferManager(u32 Size);
void InitializePushConstantManager(u32 Size);
void InitializeDescriptorSetManager(u32 Size);
void InitializeFrameMemory();
void DeinitFrameMemory();

void DeinitSamplerManager(void(*CleanUpFunc)(SamplerData*));
void DeinitPipelineManager(void(*CleanUpFunc)(PipelineData*));
void DeinitPipelineLayoutManager(void(*CleanUpFunc)(PipelineLayoutData*));
void DeinitDescriptorSetLayoutManager(void(*CleanUpFunc)(DescriptorSetLayoutData*));
void DeinitDescriptorPoolManager(void(*CleanUpFunc)(DescriptorPoolData*));
void DeinitShaderManager(void(*CleanUpFunc)(ShaderData*));
void DeinitImageManager(void(*CleanUpFunc)(ImageResource*));
void DeinitImageViewManager(void(*CleanUpFunc)(ImageViewData*));
void DeinitGPUBufferManager(void(*CleanUpFunc)(GPUBufferData*));
void DeinitPushConstantManager();
void DeinitDescriptorSetManager();
void DeinitCommandSystem(void(*CleanUpFunc)(CommandWorkerData*));

BmRender_Sampler CreateSamplerHandle(const SamplerData* Data);
BmRender_Pipeline CreatePipelineHandle(const PipelineData* Data);
BmRender_PipelineLayout CreatePipelineLayoutHandle(const PipelineLayoutData* Data);
BmRender_DescriptorSetLayout CreateDescriptorSetLayoutHandle(const DescriptorSetLayoutData* Data);
BmRender_DescriptorPool CreateDescriptorPoolHandle(const DescriptorPoolData* Data);
BmRender_Shader CreateShaderHandle(const ShaderData* Data);
BmRender_Image CreateImageHandle(const ImageResource* Data);
BmRender_ImageView CreateImageViewHandle(const ImageViewData* Data);
BmRender_GPUBuffer CreateGPUBufferHandle(const GPUBufferData* Data);
BmRender_PushConstant CreatePushConstantHandle(const PushConstantData* Data);
BmRender_DescriptorSet CreateDescriptorSetHandle(const DescriptorSetData* Data);
BmRender_CommandWorker CreateCommandWorkerHandle(const CommandWorkerData* Data);

void DestroySamplerHandle(BmRender_Sampler handle);
void DestroyPipelineHandle(BmRender_Pipeline handle);
void DestroyPipelineLayoutHandle(BmRender_PipelineLayout handle);
void DestroyDescriptorSetLayoutHandle(BmRender_DescriptorSetLayout handle);
void DestroyDescriptorPoolHandle(BmRender_DescriptorPool handle);
void DestroyShaderHandle(BmRender_Shader handle);
void DestroyImageHandle(BmRender_Image handle);
void DestroyImageViewHandle(BmRender_ImageView Handle);
void DestroyGPUBufferHandle(BmRender_GPUBuffer Handle);
void DestroyPushConstantHandle(BmRender_PushConstant Handle);
void DestroyDescriptorSetHandle(BmRender_DescriptorSet Handle);
void DestroyCommandWorkerHandle(BmRender_CommandWorker Handle);

SamplerData* GetSamplerData(BmRender_Sampler Handle);
PipelineData* GetPipelineData(BmRender_Pipeline Handle);
PipelineLayoutData* GetPipelineLayoutData(BmRender_PipelineLayout Handle);
DescriptorSetLayoutData* GetDescriptorSetLayoutData(BmRender_DescriptorSetLayout Handle);
DescriptorPoolData* GetDescriptorPoolData(BmRender_DescriptorPool Handle);
ShaderData* GetShaderData(BmRender_Shader Handle);
ImageResource* GetImageData(BmRender_Image Handle);
ImageViewData* GetImageViewData(BmRender_ImageView Handle);
GPUBufferData* GetGPUBufferData(BmRender_GPUBuffer Handle);
PushConstantData* GetPushConstantData(BmRender_PushConstant Handle);
DescriptorSetData* GetDescriptorSetData(BmRender_DescriptorSet Handle);
CommandWorkerData* GetSubmitPoolData(BmRender_CommandWorker Handle);

Memory::FrameMemory GetFrameMemory();


void InitCommandSystem(u32 WorkerCount);
void UpdateCommandSystem();
void DeinitCommandSystem(void(*CleanUpFunc)(CommandWorkerData*));
void InitDrawSystem();
void DeInitDrawSystem();

CommandSystemData* GetCommandSystemData();
DrawSystemData* GetDrawSystemData();