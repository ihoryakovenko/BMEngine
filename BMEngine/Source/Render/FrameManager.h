#pragma once






//////////////////////////////////////
// TODO: DEPRECATED TO REFACTOR
//////////////////////////////////////









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

	typedef u64 UniformMemoryHnadle;

	void Init();
	void DeInit();

	void UpdateViewProjection(const ViewProjectionBuffer* Data);

	UniformMemoryHnadle ReserveUniformMemory(u64 Size);
	void UpdateUniformMemory(UniformMemoryHnadle Handle, const void* Data, u64 Size);
	VkDescriptorSet CreateAndBindSet(UniformMemoryHnadle Handle, u64 Size, VkDescriptorSetLayout Layout);
	VkDescriptorSetLayout CreateCompatibleLayout(u32 Flags);

	VkDescriptorSetLayout GetViewProjectionLayout();
	VkDescriptorSet GetViewProjectionSet();
}