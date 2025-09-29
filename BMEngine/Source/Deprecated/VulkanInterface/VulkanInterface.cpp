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
	u32 TestGetImageIndex()
	{
		return Render::GetRenderState()->RenderDrawState.CurrentImageIndex;
	}

	void WaitDevice()
	{
		vkDeviceWaitIdle(RenderResources::GetCoreContext()->LogicalDevice);
	}


	VkCommandBuffer GetCommandBuffer()
	{
		return Render::GetRenderState()->RenderDrawState.Frames.CommandBuffers[Render::GetRenderState()->RenderDrawState.CurrentImageIndex];
	}

	VkCommandPool GetTransferCommandPool()
	{
		return Render::GetRenderState()->RenderDrawState.GraphicsCommandPool;
	}

}
