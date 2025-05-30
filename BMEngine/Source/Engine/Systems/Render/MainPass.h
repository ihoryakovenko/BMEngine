#pragma once

#include <vulkan/vulkan.h>
#include "Render/VulkanInterface/VulkanInterface.h"
#include "Engine/Systems/Render/VulkanHelper.h"

namespace MainPass
{
	void Init();
	void DeInit();

	void BeginPass();
	void EndPass();

	VulkanHelper::AttachmentData* GetAttachmentData();
}