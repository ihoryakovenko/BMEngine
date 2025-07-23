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

	void Init(GLFWwindow* WindowHandler, const ResourcesDescription* ResDescription)
	{
		VulkanCoreContext::CreateCoreContext(&CoreContext, WindowHandler);
		VkDevice Device = CoreContext.LogicalDevice;

		VBindings.reserve(1);
		VBindings[STATIC_MESH_VERTEX] = { sizeof(Render::StaticMeshVertex), VK_VERTEX_INPUT_RATE_VERTEX };

		VAttributes.reserve(3);
		VAttributes[STATIC_MESH_POSITION] = { VK_FORMAT_R32G32B32_SFLOAT, offsetof(Render::StaticMeshVertex, Position) };
		VAttributes[STATIC_MESH_TEXTURE_COORDINATE] = { VK_FORMAT_R32G32_SFLOAT, offsetof(Render::StaticMeshVertex, TextureCoords) };
		VAttributes[STATIC_MESH_NORMAL] = { VK_FORMAT_R32G32B32_SFLOAT, offsetof(Render::StaticMeshVertex, Normal) };

		for (auto it = ResDescription->Shaders.begin(); it != ResDescription->Shaders.end(); ++it)
		{
			std::vector<char> shaderCode;
			if (Util::OpenAndReadFileFull(it->second.c_str(), shaderCode, "rb"))
			{
				VkShaderModuleCreateInfo shaderInfo = { };
				shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shaderInfo.pNext = nullptr;
				shaderInfo.flags = 0;
				shaderInfo.codeSize = shaderCode.size();
				shaderInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
				
				VkShaderModule NewShaderModule;
				VULKAN_CHECK_RESULT(vkCreateShaderModule(Device, &shaderInfo, nullptr, &NewShaderModule));
				Shaders[it->first] = NewShaderModule;
			}
			else
			{
				assert(false);
			}
		}

		for (auto it = ResDescription->Samplers.begin(); it != ResDescription->Samplers.end(); ++it)
		{
			VkSampler NewSampler;
			VULKAN_CHECK_RESULT(vkCreateSampler(Device, &it->second, nullptr, &NewSampler));
			Samplers[it->first] = NewSampler;
		}

		for (auto it = ResDescription->LayoutBindings.begin(); it != ResDescription->LayoutBindings.end(); ++it)
		{
			const Memory::DynamicHeapArray<VkDescriptorSetLayoutBinding>* Bindings = &it->second;

			VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
			LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfo.bindingCount = Bindings->Count;
			LayoutCreateInfo.pBindings = Bindings->Data;
			LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			LayoutCreateInfo.pNext = nullptr;

			VkDescriptorSetLayout NewLayout;
			VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device, &LayoutCreateInfo, nullptr, &NewLayout));
			DescriptorSetLayouts[it->first] = NewLayout;
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