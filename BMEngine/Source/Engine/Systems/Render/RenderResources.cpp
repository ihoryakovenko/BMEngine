#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"
#include "Engine/Systems/Render/Render.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace RenderResources
{
	static VulkanCoreContext::VulkanCoreContext CoreContext;

	static std::unordered_map<std::string, VertexAttribute> VAttributes;
	static std::unordered_map<std::string, VertexBinding> VBindings;
	
	static std::unordered_map<std::string, VkSampler> Samplers;
	static std::unordered_map<std::string, VkDescriptorSetLayout> DescriptorSetLayouts;
	static std::unordered_map<std::string, VkShaderModule> Shaders;

	void Init(GLFWwindow* WindowHandler, Yaml::Node& Root)
	{
		VulkanCoreContext::CreateCoreContext(&CoreContext, WindowHandler);
		VkDevice Device = CoreContext.LogicalDevice;

		Yaml::Node& VerticesNode = Util::GetVertices(Root);
		for (auto VertexIt = VerticesNode.Begin(); VertexIt != VerticesNode.End(); VertexIt++)
		{
			Yaml::Node& VertexNode = (*VertexIt).second;
			
			RenderResources::VertexBinding Binding = { };
			Util::ParseVertexBindingNode(Util::GetVertexBindingNode(VertexNode), &Binding);
			
			Yaml::Node& AttributesNode = Util::GetVertexAttributesNode(VertexNode);

			u32 Stride = 0;
			for (auto AttributeIt = AttributesNode.Begin(); AttributeIt != AttributesNode.End(); AttributeIt++)
			{
				Yaml::Node& AttributeNode = (*AttributeIt).second;
				Yaml::Node& FormatNode = Util::GetVertexAttributeFormatNode(AttributeNode);
				std::string FormatStr = FormatNode.As<std::string>();
				Stride += VulkanHelper::CalculateFormatSizeFromString(FormatStr.c_str(), static_cast<u32>(FormatStr.length()));
			}

			Binding.Stride = Stride;
			VBindings[(*VertexIt).first] = Binding;

			for (auto AttributeIt = AttributesNode.Begin(); AttributeIt != AttributesNode.End(); AttributeIt++)
			{
				RenderResources::VertexAttribute Attribute = { };
				Util::ParseVertexAttributeNode((*AttributeIt).second, &Attribute);
				VAttributes[(*AttributeIt).first] = Attribute;
			}
		}

		Yaml::Node& ShadersNode = Util::GetShaders(Root);
		for (auto It = ShadersNode.Begin(); It != ShadersNode.End(); It++)
		{
			std::string ShaderPath;
			Util::ParseShaderNode((*It).second, &ShaderPath);
			
			std::vector<char> ShaderCode;
			if (Util::OpenAndReadFileFull(ShaderPath.c_str(), ShaderCode, "rb"))
			{
				VkShaderModuleCreateInfo shaderInfo = { };
				shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shaderInfo.pNext = nullptr;
				shaderInfo.flags = 0;
				shaderInfo.codeSize = ShaderCode.size();
				shaderInfo.pCode = reinterpret_cast<const uint32_t*>(ShaderCode.data());
				
				VkShaderModule NewShaderModule;
				VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &shaderInfo, nullptr, &NewShaderModule));
				Shaders[(*It).first] = NewShaderModule;
			}
			else
			{
				assert(false);
			}
		}

		Yaml::Node& SamplersNode = Util::GetSamplers(Root);
		for (auto It = SamplersNode.Begin(); It != SamplersNode.End(); It++)
		{
			VkSamplerCreateInfo CreateInfo = { };
			Util::ParseSamplerNode((*It).second, &CreateInfo);

			VkSampler NewSampler;
			VULKAN_CHECK_RESULT(vkCreateSampler(Device, &CreateInfo, nullptr, &NewSampler));
			Samplers[(*It).first] = NewSampler;
		}

		Yaml::Node& DescriptorSetLayoutsNode = Util::GetDescriptorSetLayouts(Root);
		for (auto LayoutIt = DescriptorSetLayoutsNode.Begin(); LayoutIt != DescriptorSetLayoutsNode.End(); LayoutIt++)
		{
			Yaml::Node& BindingsNode = Util::ParseDescriptorSetLayoutNode((*LayoutIt).second);

			Memory::DynamicHeapArray<VkDescriptorSetLayoutBinding> Bindings = Memory::AllocateArray<VkDescriptorSetLayoutBinding>(1);
			
			for (auto BindingIt = BindingsNode.Begin(); BindingIt != BindingsNode.End(); BindingIt++)
			{
				VkDescriptorSetLayoutBinding Binding = { };
				Util::ParseDescriptorSetLayoutBindingNode((*BindingIt).second, &Binding);
				Memory::PushBackToArray(&Bindings, &Binding);
			}

			VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
			LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfo.bindingCount = Bindings.Count;
			LayoutCreateInfo.pBindings = Bindings.Data;
			LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			LayoutCreateInfo.pNext = nullptr;

			VkDescriptorSetLayout NewLayout;
			VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, nullptr, &NewLayout));
			DescriptorSetLayouts[(*LayoutIt).first] = NewLayout;

			Memory::FreeArray(&Bindings);
		}
	}

	void DeInit()
	{
		VkDevice Device = CoreContext.LogicalDevice;

		for (auto It = Shaders.begin(); It != Shaders.end(); ++It)
		{
			vkDestroyShaderModule(Device, It->second, nullptr);
		}

		for (auto It = Samplers.begin(); It != Samplers.end(); ++It)
		{
			vkDestroySampler(Device, It->second, nullptr);
		}

		for (auto It = DescriptorSetLayouts.begin(); It != DescriptorSetLayouts.end(); ++It)
		{
			vkDestroyDescriptorSetLayout(Device, It->second, nullptr);
		}

		Shaders.clear();
		Samplers.clear();
		DescriptorSetLayouts.clear();

		VulkanCoreContext::DestroyCoreContext(&CoreContext);
	}

	VulkanCoreContext::VulkanCoreContext* GetCoreContext()
	{
		return &CoreContext;
	}

	VkSampler GetSampler(const std::string& Id)
	{
		auto It = Samplers.find(Id);
		if (It != Samplers.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VkDescriptorSetLayout GetSetLayout(const std::string& Id)
	{
		auto It = DescriptorSetLayouts.find(Id);
		if (It != DescriptorSetLayouts.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VkShaderModule GetShader(const std::string& Id)
	{
		auto It = Shaders.find(Id);
		if (It != Shaders.end())
		{
			return It->second;
		}

		assert(false);
		return nullptr;
	}

	VertexAttribute GetVertexAttribute(const std::string& Id)
	{
		auto It = VAttributes.find(Id);
		if (It != VAttributes.end())
		{
			return It->second;
		}

		assert(false);
		return { };
	}

	VertexBinding GetVertexBinding(const std::string& Id)
	{
		auto It = VBindings.find(Id);
		if (It != VBindings.end())
		{
			return It->second;
		}

		assert(false);
		return { };
	}
}