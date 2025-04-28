#pragma once

#include <vulkan/vulkan.h>
#include "Render/VulkanInterface/VulkanInterface.h"

namespace DeferredPass
{
	void Init();
	void DeInit();

	void Draw();

	VkImageView* TestDeferredInputColorImageInterface();
	VkImageView* TestDeferredInputDepthImageInterface();

	VulkanInterface::UniformImage* TestDeferredInputColorImage();
	VulkanInterface::UniformImage* TestDeferredInputDepthImage();
}