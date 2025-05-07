#pragma once

#include <vulkan/vulkan.h>
#include "Render/VulkanInterface/VulkanInterface.h"

namespace DeferredPass
{
	void Init();
	void DeInit();

	void Draw();

	void BeginPass();
	void EndPass();

	VkImageView* TestDeferredInputColorImageInterface();
	VkImageView* TestDeferredInputDepthImageInterface();

	VulkanInterface::UniformImage* TestDeferredInputColorImage();
	VulkanInterface::UniformImage* TestDeferredInputDepthImage();

	VulkanInterface::AttachmentData* GetAttachmentData();
}