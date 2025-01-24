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
	void DeInit();

	void UpdateViewProjection(const ViewProjectionBuffer* Data);

	u64 ReserveUniformMemory(u64 Size);
	void UpdateUniformMemory(u64 Offset, const void* Data, u64 Size);

	VkDescriptorSetLayout GetViewProjectionLayout();
	VkDescriptorSet GetViewProjectionSet();
}