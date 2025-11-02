#include "RenderInterface.h"

#include "RenderResources.h"
#include "VulkanHelper.h"
#include "Render.h"
#include "RenderTypes.h"
#include "Systems.h"
#include "Handles.h"

#include "Util/Util.h"
#include <type_traits>

#include "Engine/Systems/HandleManager.h"

#include "VulkanCoreContext.h"

void BmRender_Init(GLFWwindow* WindowHandler, u32 InMaxFramesInFly)
{
	InitializeFrameMemory();

	InitializeSamplerManager(32);
	InitializePipelineManager(4);
	InitializePipelineLayoutManager(32);
	InitializeDescriptorSetLayoutManager(32);
	InitializeDescriptorPoolManager(1);
	InitializeShaderManager(32);
	InitializeImageManager(32);
	InitializeImageViewManager(32);
	InitializeGPUBufferManager(4);
	InitializeDescriptorSetManager(32);
	InitializePushConstantManager(4);
	InitializeFenceManager(32);
	InitializeSemaphoreManager(32);
	InitializeCommandPoolManager(4);
	InitializeCommandBufferManager(32);

	CreateCoreContext(WindowHandler);
}

void BmRender_DeInit()
{
	DeinitSamplerManager(OnSamplerClear);
	DeinitPipelineManager(OnPipelineClear);
	DeinitPipelineLayoutManager(OnPipelineLayoutClear);
	DeinitDescriptorSetLayoutManager(OnDescriptorSetLayoutClear);
	DeinitDescriptorPoolManager(OnDescriptorPoolClear);
	DeinitShaderManager(OnShaderClear);
	DeinitImageManager(OnImageClear);
	DeinitImageViewManager(OnImageViewClear);
	DeinitGPUBufferManager(OnGPUBufferClear);
	DeinitDescriptorSetManager();
	DeinitPushConstantManager();
	DeinitFenceManager(OnFenceClear);
	DeinitSemaphoreManager(OnSemaphoreClear);
	DeinitCommandPoolManager(OnCommandPoolClear);
	DeinitCommandBufferManager();

	DestroyCoreContext();
	DeinitFrameMemory();
}

u32 BmRender_GetSwapchainImageCount()
{
	return GetCoreContext()->ImagesCount;
}

void Test_FrameFree()
{
	Memory::FrameFree(GetFrameMemory());
}
