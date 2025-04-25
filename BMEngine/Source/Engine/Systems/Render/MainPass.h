#pragma once

#include <vulkan/vulkan.h>

namespace MainPass
{
	void Init();
	void DeInit();

	void BeginPass();
	void EndPass();


	VkRenderPass TestGetRenderPass();
}