#pragma once

#include <glm/glm.hpp>

#include "Util/EngineTypes.h"

struct VkDescriptorSetLayout_T;
typedef VkDescriptorSetLayout_T* VkDescriptorSetLayout;

struct VkDescriptorSet_T;
typedef VkDescriptorSet_T* VkDescriptorSet;

namespace FrameManager
{
	struct ViewProjectionBuffer
	{
		glm::mat4 View;
		glm::mat4 Projection;
	};

	void Init();
	void Update(const ViewProjectionBuffer* Data);
	void DeInit();

	VkDescriptorSetLayout GetViewProjectionLayout();
	VkDescriptorSet* GetViewProjectionSet();
}