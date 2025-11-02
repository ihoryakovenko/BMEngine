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

	CreateCoreContext(WindowHandler);

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
	InitCommandWorkerManager(InMaxFramesInFly);

	InitCommandSystem(InMaxFramesInFly);
	InitDrawSystem(InMaxFramesInFly);
}

void BmRender_DeInit()
{
	DeInitDrawSystem();

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
	DeinitCommandWorkerManager(OnComandWorkerClear);

	DestroyCoreContext();
	DeinitFrameMemory();
}

void Test_FrameFree()
{
	Memory::FrameFree(GetFrameMemory());
}
