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
	inline constexpr const char STATIC_MESH_VERTEX[] = "StaticMeshVertex";
	inline constexpr const char STATIC_MESH_POSITION[] = "Position";
	inline constexpr const char STATIC_MESH_TEXTURE_COORDINATE[] = "TextureCoords";
	inline constexpr const char STATIC_MESH_NORMAL[] = "Normal";

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

	struct ResourcesDescription
	{
		std::unordered_map<std::string, std::string> Shaders;
		std::unordered_map<std::string, VkSamplerCreateInfo> Samplers;
		std::unordered_map<std::string, Memory::DynamicHeapArray<VkDescriptorSetLayoutBinding>> LayoutBindings;
	};

	void Init(GLFWwindow* WindowHandler, const ResourcesDescription* ResDescription);
	void DeInit();

	VulkanCoreContext::VulkanCoreContext* GetCoreContext();
	VkSampler GetSampler(const std::string& Id);
	VkDescriptorSetLayout GetSetLayout(const std::string& Id);
	VkShaderModule GetShader(const std::string& Id);

	VertexAttribute GetVertexAttribute(const std::string& Id);
	VertexBinding GetVertexBinding(const std::string& Id);
}