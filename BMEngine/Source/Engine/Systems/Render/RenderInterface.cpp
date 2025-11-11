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

	InitializeDescriptorSetLayoutManager(32);
	InitializeShaderManager(32);
	InitializeImageManager(32);
	InitializeGPUBufferManager(4);
	InitializeDescriptorSetManager(32);
	InitializeSemaphoreManager(32);
	InitializeCommandPoolManager(4);
	InitializeCommandBufferManager(32);

	CreateCoreContext(WindowHandler);
}

void BmRender_DeInit()
{
	DeinitDescriptorSetLayoutManager();
	DeinitShaderManager();
	DeinitImageManager();
	DeinitGPUBufferManager();
	DeinitDescriptorSetManager();
	DeinitSemaphoreManager();
	DeinitCommandPoolManager();
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
