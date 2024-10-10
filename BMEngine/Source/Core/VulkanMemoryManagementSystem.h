#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Core
{
	enum class MemoryType
	{
		None = 0,
		Static
	};

	struct GPUMemoryPool
	{
		MemoryType MemType = MemoryType::None;
		VkDescriptorPool DescriptorPool = nullptr;
	};

	struct MemorySourceDevice
	{
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;
	};

	class VulkanMemoryManagementSystem
	{
	public:
		void Init(MemorySourceDevice Device);

		void AllocateStaticMemory();

	private:
		MemorySourceDevice MemorySource;
	};
}