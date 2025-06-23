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

namespace VulkanInterface
{
	u32 GetImageCount()
	{
		return Render::GetRenderState()->CoreContext.ImagesCount;
	}

	VkImageView* GetSwapchainImageViews()
	{
		return Render::GetRenderState()->CoreContext.ImageViews;
	}

	VkImage* GetSwapchainImages()
	{
		return Render::GetRenderState()->CoreContext.Images;
	}

	u32 TestGetImageIndex()
	{
		return Render::GetRenderState()->RenderDrawState.CurrentImageIndex;
	}

	void WaitDevice()
	{
		vkDeviceWaitIdle(Render::GetRenderState()->CoreContext.LogicalDevice);
	}

	VkDevice GetDevice()
	{
		return Render::GetRenderState()->CoreContext.LogicalDevice;
	}

	VkPhysicalDevice GetPhysicalDevice()
	{
		return Render::GetRenderState()->CoreContext.PhysicalDevice;
	}

	VkCommandBuffer GetCommandBuffer()
	{
		return Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
	}

	VkQueue GetTransferQueue()
	{
		//return TransferQueue;
		return Render::GetRenderState()->CoreContext.GraphicsQueue;
	}

	VkQueue GetGraphicsQueue()
	{
		return Render::GetRenderState()->CoreContext.GraphicsQueue;
	}

	VkFormat GetSurfaceFormat()
	{
		return Render::GetRenderState()->CoreContext.SurfaceFormat.format;
	}

	u32 GetQueueGraphicsFamilyIndex()
	{
		return Render::GetRenderState()->CoreContext.Indices.GraphicsFamily;
	}

	VkDescriptorPool GetDescriptorPool()
	{
		return Render::GetRenderState()->RenderDrawState.MainPool;
	}

	VkInstance GetInstance()
	{
		return Render::GetRenderState()->CoreContext.VulkanInstance;
	}

	VkCommandPool GetTransferCommandPool()
	{
		return Render::GetRenderState()->RenderDrawState.GraphicsCommandPool;
	}

	VkSwapchainKHR GetSwapchain()
	{
		return Render::GetRenderState()->CoreContext.VulkanSwapchain;
	}
}
