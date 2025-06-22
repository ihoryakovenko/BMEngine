#pragma once

#include <vulkan/vulkan.h>
#include "Deprecated/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"

namespace MainPass
{
	void Init();
	void DeInit();

	void BeginPass();
	void EndPass();

	VulkanHelper::AttachmentData* GetAttachmentData();
}