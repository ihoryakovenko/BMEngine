#include "VulkanMemoryManagementSystem.h"

namespace Core
{
	void VulkanMemoryManagementSystem::Init(MemorySourceDevice Device)
	{
		MemorySource = Device;
	}

	void VulkanMemoryManagementSystem::AllocateStaticMemory()
	{
	}
}
