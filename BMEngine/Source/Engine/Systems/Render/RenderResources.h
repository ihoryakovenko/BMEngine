#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

struct GLFWwindow;

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}

namespace RenderResources
{
	struct ResourcesDescription
	{
		std::unordered_map<std::string, VkSamplerCreateInfo> Samplers;
		std::unordered_map<std::string, Memory::DynamicHeapArray<VkDescriptorSetLayoutBinding>> LayoutBindings;
	};

	void Init(GLFWwindow* WindowHandler, const ResourcesDescription* ResDescription);
	void DeInit();

	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	VkDescriptorSetLayout GetSetLayout(const std::string& Id);
}