#include "RenderResources.h"

#include "Util/EngineTypes.h"
#include "Util/Util.h"

#include "VulkanCoreContext.h"

#include <GLFW/glfw3.h>

namespace RenderResources
{
	static VulkanCoreContext::VulkanCoreContext CoreContext;

	static std::unordered_map<std::string, VkSampler> Samplers;
	static std::unordered_map<std::string, VkDescriptorSetLayout> DescriptorSetLayouts;

	void Init(GLFWwindow* WindowHandler, const ResourcesDescription* ResDescription)
	{
		VulkanCoreContext::CreateCoreContext(&CoreContext, WindowHandler);

		VkDevice Device = CoreContext.LogicalDevice;

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

		for (auto It = Samplers.begin(); It != Samplers.end(); ++It)
		{
			vkDestroySampler(Device, It->second, nullptr);
		}

		for (auto It = DescriptorSetLayouts.begin(); It != DescriptorSetLayouts.end(); ++It)
		{
			vkDestroyDescriptorSetLayout(Device, It->second, nullptr);
		}

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
}