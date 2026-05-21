#include "RenderInterface.h"

#include "VulkanHelper.h"
#include "RenderTypes.h"
#include "Handles.h"

#include <type_traits>

#include "VulkanCoreContext.h"

void BmRender_Init(GLFWwindow* WindowHandler)
{
	InitializeFrameMemory();

	InitializeDescriptorSetLayoutManager(32);
	InitializeImageManager(32);
	InitializeGPUBufferManager(4);
	InitializeDescriptorSetManager(32);
	InitializeSemaphoreManager(32);
	InitializeCommandPoolManager(4);
	InitializeCommandBufferManager(32);
	InitializeQueueManager(2);
	InitializePipelineLayoutManager(32);
	InitializeImageViewManager(32);
	InitializePipelineManager(4);

	CreateCoreContext(WindowHandler);
}

void BmRender_DeInit()
{
	DeinitDescriptorSetLayoutManager();
	DeinitImageManager();
	DeinitGPUBufferManager();
	DeinitDescriptorSetManager();
	DeinitSemaphoreManager();
	DeinitCommandPoolManager();
	DeinitCommandBufferManager();
	DeinitQueueManager();
	DeinitPipelineLayoutManager();
	DeinitImageViewManager();
	DeinitPipelineManager();

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

BmRender_Image BmRender_GetSwapchainImage(u32 Index)
{
	VulkanCoreContext* CoreContext = GetCoreContext();
	if (Index >= CoreContext->ImagesCount)
	{
		return nullptr;
	}
	return CoreContext->Images[Index];
}

BmRender_ImageView BmRender_GetSwapchainImageView(u32 Index)
{
	VulkanCoreContext* CoreContext = GetCoreContext();
	if (Index >= CoreContext->ImagesCount)
	{
		return nullptr;
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

BmRender_QueueType BmRender_GetQueueType(BmRender_Queue Queue)
{
	QueueData Data;
	if (BmRender_GetQueueData(Queue, &Data))
	{
		return Data.QueueType;
	}
	return BmRender_QueueType::None;
}

u32 BmRender_GetQueueFamily(BmRender_Queue Queue)
{
	QueueData Data;
	if (BmRender_GetQueueData(Queue, &Data))
	{
		VulkanCoreContext* CoreContext = GetCoreContext();
		s32 FamilyIndex = GetQueueFamilyIndexFromQueueType(Data.QueueType, CoreContext->Indices);
		if (FamilyIndex != -1)
		{
			return (u32)FamilyIndex;
		}
	}
	return 0;
}

void BmRender_FrameFree()
{
	Memory_LinearAllocator_FreeMemory(GetFrameMemory());
}
