#include "RenderInterface.h"

#include "VulkanHelper.h"
#include "RenderTypes.h"

#include <type_traits>

#include "VulkanCoreContext.h"

void BmRender_Init(GLFWwindow* WindowHandler)
{
	InitializeFrameMemory();

	CreateCoreContext(WindowHandler);
}

void BmRender_DeInit()
{
	DestroyCoreContext();
	DeMemory_LinearAllocator_Init();
}

u32 BmRender_GetSwapchainImageCount()
{
	return GetCoreContext()->ImagesCount;
}

BmRender_SurfaceFormat BmRender_GetSurfaceFormat()
{
	return VkSurfaceFormatToBmRender(GetCoreContext()->SurfaceFormat);
}

BmRender_Image* BmRender_GetSwapchainImage(u32 Index)
{
	VulkanCoreContext* CoreContext = GetCoreContext();
	if (Index >= CoreContext->ImagesCount)
	{
		return nullptr;
	}
	return CoreContext->Images + Index;
}

BmRender_ImageView BmRender_GetSwapchainImageView(u32 Index)
{
	VulkanCoreContext* CoreContext = GetCoreContext();
	if (Index >= CoreContext->ImagesCount)
	{
		return { };
	}
	return CoreContext->ImageViews[Index];
}

BmRender_Dimensions BmRender_GetSwapchainExtent()
{
	return VkExtent2DToBmRender(GetCoreContext()->SwapExtent);
}

BmRender_Instance BmRender_GetVulkanInstance()
{
	return (BmRender_Instance)GetCoreContext()->VulkanInstance;
}

BmRender_PhysicalDevice BmRender_GetPhysicalDevice()
{
	return (BmRender_PhysicalDevice)GetCoreContext()->PhysicalDevice;
}

BmRender_Device BmRender_GetLogicalDevice()
{
	return (BmRender_Device)GetCoreContext()->LogicalDevice;
}

u32 BmRender_GetGraphicsQueueFamily()
{
	return (u32)GetCoreContext()->Indices.GraphicsFamily;
}

u32 BmRender_GetQueueFamily(BmRender_Queue Queue)
{
	VulkanCoreContext* CoreContext = GetCoreContext();
	s32 FamilyIndex = GetQueueFamilyIndexFromQueueType(Queue.QueueType, CoreContext->Indices);
	if (FamilyIndex != -1)
	{
		return (u32)FamilyIndex;
	}

	return 0;
}

void BmRender_FrameFree()
{
	Memory_LinearAllocator_FreeMemory(GetFrameMemory());
}
