#pragma once

#include <vulkan/vulkan.h>

#include <stb_image.h>

#include "BMRInterfaceTypes.h"
#include "Util/EngineTypes.h"
#include <Memory/MemoryManagmentSystem.h>

#include "BMRVulkan/BMRVulkan.h"

namespace BMR
{
	static const u32 MAX_IMAGES = 1024;
	static const u32 MB64 = 1024 * 1024 * 64;
	static const u32 MB128 = 1024 * 1024 * 128;
	static const u32 BUFFER_ALIGNMENT = 64;
	static const u32 IMAGE_ALIGNMENT = 4096;
	static const u32 MAX_LIGHT_SOURCES = 2;
	static const u32 MAX_DESCRIPTOR_SET_LAYOUTS_PER_PIPELINE = 8;
	static const u32 MAX_DESCRIPTOR_BINDING_PER_SET = 16;





	//struct BMRUniformSet
	//{
	//	VkDescriptorSetLayout Layout = nullptr;
	//	const BMRUniformBuffer* BMRUniformBuffers = nullptr;
	//	u32 BuffersCount = 0;
	//};

	//struct BMRUniformPushBufferDescription
	//{
	//	VkShaderStageFlags StageFlags = 0;
	//	u32 Offset = 0;
	//	u32 Size = 0;
	//};

	//struct BMRUniformInput
	//{
	//	const VkDescriptorSetLayout* UniformLayout = nullptr;
	//	u32 layoutCount = 0;

	//	VkPushConstantRange PushConstants;
	//};


}
