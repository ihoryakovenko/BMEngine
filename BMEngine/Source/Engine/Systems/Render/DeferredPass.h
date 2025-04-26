#pragma once

#include <vulkan/vulkan.h>

namespace DeferredPass
{
	void Init();
	void DeInit();

	void Draw();

	VkImageView* TestDeferredInputColorImageInterface();
	VkImageView* TestDeferredInputDepthImageInterface();
}