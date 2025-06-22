#pragma once

#include <vulkan/vulkan.h>
#include "Deprecated/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"

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

	VulkanHelper::AttachmentData* GetAttachmentData();
}