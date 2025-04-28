#pragma once

#include <vulkan/vulkan.h>
#include "Render/VulkanInterface/VulkanInterface.h"

namespace MainPass
{
	void Init();
	void DeInit();

	void BeginPass();
	void EndPass();

	VulkanInterface::AttachmentData* GetAttachmentData();
}