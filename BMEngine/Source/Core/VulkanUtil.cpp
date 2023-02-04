#include <vector>

#include "VulkanUtil.h"

#include "Util/Log.h"

namespace Core
{
	QueueFamilyIndices QueueFamilyIndices::GetQueueFamilies(VkPhysicalDevice Device)
	{
		static QueueFamilyIndices FamilyIndices = [Device]()
		{
			QueueFamilyIndices Indices;
			uint32_t FamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(Device, &FamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> FamilyProperties(FamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(Device, &FamilyCount, FamilyProperties.data());

			int i = 0;
			for (const auto& QueueFamily : FamilyProperties)
			{
				if (QueueFamily.queueCount > 0 && QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					Indices._GraphicsFamily = i;
				}

				if (Indices.IsValid())
				{
					break;
				}

				++i;
			}

			return Indices;
		}();

		return FamilyIndices;
	}

	bool VulkanUtil::IsInstanceExtensionSupported(const char* Extension)
	{
		static const std::vector<VkExtensionProperties>& AvalibleExtensions = []()
		{
			uint32_t ExtensionsCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionsCount, nullptr);

			std::vector<VkExtensionProperties> Extensions(ExtensionsCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionsCount, Extensions.data());

			return Extensions;
		}();

		for (const auto& AvalibleExtension : AvalibleExtensions)
		{
			if (std::strcmp(Extension, AvalibleExtension.extensionName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	bool VulkanUtil::IsDeviceSuitable(VkPhysicalDevice Device)
	{
		/*
		// ID, name, type, vendor, etc
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(Device, &Properties);

		// geo shader, tess shader, wide lines, etc
		VkPhysicalDeviceFeatures Features;
		vkGetPhysicalDeviceFeatures(Device, &Features);
		*/

		QueueFamilyIndices Indices = QueueFamilyIndices::GetQueueFamilies(Device);
		return Indices.IsValid();
	}

	bool VulkanUtil::IsValidationLayerSupported(const char* Layer)
	{
		static const std::vector<VkLayerProperties>& AvailableLayers = []()
		{
			uint32_t LayerCount = 0;
			vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

			std::vector<VkLayerProperties> Layers(LayerCount);
			vkEnumerateInstanceLayerProperties(&LayerCount, Layers.data());

			return Layers;
		}();

		for (const auto& AvalibleLayer : AvailableLayers)
		{
			if (std::strcmp(Layer, AvalibleLayer.layerName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	bool VulkanUtil::GetRequiredExtensions(std::vector<const char*>& InstanceExtensions)
	{
		uint32_t ExtensionsCount = 0;
		const char** Extensions = glfwGetRequiredInstanceExtensions(&ExtensionsCount);
		if (Extensions == nullptr)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		for (uint32_t i = 0; i < ExtensionsCount; ++i)
		{
			const char* Extension = Extensions[i];
			if (!IsInstanceExtensionSupported(Extension))
			{
				Util::Log().Error("Extension {} unsupported", Extension);
				return false;
			}

			InstanceExtensions.push_back(Extension);
		}

		if (_EnableValidationLayers)
		{
			InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return true;
	}

	void VulkanUtil::GetEnabledValidationLayers(uint32_t& EnabledLayerCount, const char* const*& EnabledLayerNames)
	{
		if (_EnableValidationLayers)
		{
			std::erase_if(_ValidationLayers, [](const char* Layer)
			{
				if (!VulkanUtil::IsValidationLayerSupported(Layer))
				{
					Util::Log().Error("Validation layer {} unsupported", Layer);
					return true;
				}
				
				return false;
			});

			EnabledLayerCount = static_cast<uint32_t>(_ValidationLayers.size());
			EnabledLayerNames = VulkanUtil::_ValidationLayers.data();
			return;
		}

		EnabledLayerNames = nullptr;
		EnabledLayerCount = 0;
	}
}
