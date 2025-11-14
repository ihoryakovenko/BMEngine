#include "RenderInterface.h"

#include "RenderResources.h"
#include "VulkanHelper.h"
#include "Render.h"
#include "RenderTypes.h"
#include "Systems.h"
#include "Handles.h"

#include "Util/Util.h"
#include <type_traits>

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
	DeMemory_LinearAllocator_Init();
}

u32 BmRender_GetSwapchainImageCount()
{
	return GetCoreContext()->ImagesCount;
}

VkSurfaceFormatKHR BmRender_GetSurfaceFormat()
{
	return GetCoreContext()->SurfaceFormat;
}

BmRender_Image BmRender_GetSwapchainImage(u32 Index)
{
	VulkanCoreContext::VulkanCoreContext* CoreContext = GetCoreContext();
	if (Index >= CoreContext->ImagesCount)
	{
		return nullptr;
	}
	return CoreContext->Images[Index];
}

BmRender_ImageView BmRender_GetSwapchainImageView(u32 Index)
{
	VulkanCoreContext::VulkanCoreContext* CoreContext = GetCoreContext();
	if (Index >= CoreContext->ImagesCount)
	{
		return nullptr;
	}
	return CoreContext->ImageViews[Index];
}

VkExtent2D BmRender_GetSwapchainExtent()
{
	return GetCoreContext()->SwapExtent;
}

void Test_Memory_LinearAllocator_FreeAll()
{
	Memory_LinearAllocator_FreeAll(GetFrameMemory());
}
