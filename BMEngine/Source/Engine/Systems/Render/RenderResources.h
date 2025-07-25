#pragma once

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <string>

#include <mini-yaml/yaml/Yaml.hpp>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"

struct GLFWwindow;

namespace VulkanCoreContext
{
	struct VulkanCoreContext;
}

namespace RenderResources
{
	struct VertexAttribute
	{
		VkFormat Format;
		u32 Offset;
	};

	struct VertexBinding
	{
		u32 Stride;
		VkVertexInputRate InputRate;
	};

	void Init(GLFWwindow* WindowHandler, Yaml::Node& Root);
	void DeInit();

	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	VkDescriptorSetLayout GetSetLayout(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);

	VertexAttribute GetVertexAttribute(const std::string& Id);
	VertexBinding GetVertexBinding(const std::string& Id);
}