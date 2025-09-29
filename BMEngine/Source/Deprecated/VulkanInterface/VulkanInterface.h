#pragma once





//////////////////////////////////////
// TODO: DEPRECATED TO REFACTOR
//////////////////////////////////////








#include <vulkan/vulkan.h>

#include "Util/EngineTypes.h"

#include <mutex>

struct GLFWwindow;

namespace VulkanInterface
{
	struct UniformBuffer
	{
		VkBuffer Buffer;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};

	struct UniformImage
	{
		VkImage Image;
		VkDeviceMemory Memory;
		u64 Size; // Size could be aligned
	};

	struct RenderPipeline
	{
		VkPipeline Pipeline;
		VkPipelineLayout PipelineLayout;
	};

	// TO refactor

	u32 TestGetImageIndex();

	void WaitDevice();

	typedef UniformBuffer IndexBuffer;
	typedef UniformBuffer VertexBuffer;

	VkCommandPool GetTransferCommandPool();
	VkCommandBuffer GetCommandBuffer();
}
