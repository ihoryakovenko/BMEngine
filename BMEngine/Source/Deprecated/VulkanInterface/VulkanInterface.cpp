#include "VulkanInterface.h"






//////////////////////////////////////
// TODO: DEPRECATED TO REFACTOR
//////////////////////////////////////









#include <cassert>
#include <cstring>
#include <stdio.h>
#include <stdarg.h>

#include <glm/glm.hpp>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/Util.h"


#include "Engine/Systems/Render/Render.h"
#include "Engine/Systems/Render/RenderResources.h"

namespace VulkanInterface
{
	u32 GetImageCount()
	{
		return RenderResources::GetCoreContext()->ImagesCount;
	}

	VkImageView* GetSwapchainImageViews()
	{
		return RenderResources::GetCoreContext()->ImageViews;
	}

	VkImage* GetSwapchainImages()
	{
		return RenderResources::GetCoreContext()->Images;
	}

	u32 TestGetImageIndex()
	{
		return Render::GetRenderState()->RenderDrawState.CurrentImageIndex;
	}

	void WaitDevice()
	{
		vkDeviceWaitIdle(RenderResources::GetCoreContext()->LogicalDevice);
	}

	VkDevice GetDevice()
	{
		return RenderResources::GetCoreContext()->LogicalDevice;
	}

	VkPhysicalDevice GetPhysicalDevice()
	{
		return RenderResources::GetCoreContext()->PhysicalDevice;
	}

	VkCommandBuffer GetCommandBuffer()
	{
		return Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
	}

	VkQueue GetTransferQueue()
	{
		//return TransferQueue;
		return RenderResources::GetCoreContext()->GraphicsQueue;
	}

	VkQueue GetGraphicsQueue()
	{
		return RenderResources::GetCoreContext()->GraphicsQueue;
	}

	VkFormat GetSurfaceFormat()
	{
		return RenderResources::GetCoreContext()->SurfaceFormat.format;
	}

	u32 GetQueueGraphicsFamilyIndex()
	{
		return RenderResources::GetCoreContext()->Indices.GraphicsFamily;
	}

	VkDescriptorPool GetDescriptorPool()
	{
		return Render::GetRenderState()->RenderDrawState.MainPool;
	}

	VkInstance GetInstance()
	{
		return RenderResources::GetCoreContext()->VulkanInstance;
	}

	VkCommandPool GetTransferCommandPool()
	{
		return Render::GetRenderState()->RenderDrawState.GraphicsCommandPool;
	}

	VkSwapchainKHR GetSwapchain()
	{
		return RenderResources::GetCoreContext()->VulkanSwapchain;
	}
}
