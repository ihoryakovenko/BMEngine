#include <vector>
#include <cassert>
#include <algorithm>

#include "VulkanRender.h"

namespace Core
{
	bool Core::VulkanRender::Init(Window* NewWindow)
	{
		_Window = NewWindow;
		if (!CreateInstance())
		{
			return false;
		}

		SetupPhysicalDevice();
		CreateLogicalDevice();
		return true;
	}

	void VulkanRender::Cleanup()
	{
		vkDestroyDevice(_MainDevice._LogicalDevice, nullptr);
		vkDestroyInstance(_Instance, nullptr);
	}

	bool Core::VulkanRender::CreateInstance()
	{
		// ApplicationInfo initialization
		VkApplicationInfo ApplicationInfo = { };
		ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		ApplicationInfo.pApplicationName = "Blank App";
		ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.pEngineName = "BMEngine";
		ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.apiVersion = VK_API_VERSION_1_3;

		// Extensions info initialization
		uint32_t ExtensionsCount = 0;
		const char** Extensions = glfwGetRequiredInstanceExtensions(&ExtensionsCount);
		if (Extensions == nullptr)
		{
			return false;
		}

		std::vector<const char*> InstanceExtensions;
		for (uint32_t i = 0; i < ExtensionsCount; ++i)
		{
			const char* Extension = Extensions[i];
			if (!IsInstanceExtensionSupported(Extension))
			{
				return false;
			}

			InstanceExtensions.push_back(Extension);
		}

		// CreateInfo info initialization
		VkInstanceCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &ApplicationInfo;
		CreateInfo.enabledLayerCount = 0;
		CreateInfo.ppEnabledLayerNames = nullptr;
		CreateInfo.enabledExtensionCount = static_cast<uint32_t>(InstanceExtensions.size());
		CreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

		const VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &_Instance);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		return true;
	}

	void VulkanRender::CreateLogicalDevice()
	{
		QueueFamilyIndices Indices = SetupQueueFamilies(_MainDevice._PhysicalDevice);
		const float Priority = 1.0f;

		VkDeviceQueueCreateInfo QueueCreateInfo = { };
		QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(Indices._GraphicsFamily);
		QueueCreateInfo.queueCount = 1;
		QueueCreateInfo.pQueuePriorities = &Priority;

		VkPhysicalDeviceFeatures DeviceFeatures = {};

		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = 1;
		DeviceCreateInfo.pQueueCreateInfos = &QueueCreateInfo;
		DeviceCreateInfo.enabledExtensionCount = 0;
		DeviceCreateInfo.ppEnabledExtensionNames = nullptr;
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		const VkResult Result = vkCreateDevice(_MainDevice._PhysicalDevice, &DeviceCreateInfo, nullptr, &_MainDevice._LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			// toto check
		}

		// toto check
		// queues are created at the same time as the device
		// so we want to handle them
		vkGetDeviceQueue(_MainDevice._LogicalDevice, Indices._GraphicsFamily, 0, &GraphicsQueue);
	}

	void VulkanRender::SetupPhysicalDevice()
	{
		uint32_t DevicesCount = 0;
		vkEnumeratePhysicalDevices(_Instance, &DevicesCount, nullptr);
		if (DevicesCount <= 0)
		{
			// todo error
			return;
		}

		std::vector<VkPhysicalDevice> DeviceList(DevicesCount);
		vkEnumeratePhysicalDevices(_Instance, &DevicesCount, DeviceList.data());

		for (const auto& Device : DeviceList)
		{
			if (IsDeviceSuitable(Device))
			{
				_MainDevice._PhysicalDevice = Device;
				break;
			}
		}
	}

	// todo validate function
	QueueFamilyIndices VulkanRender::SetupQueueFamilies(VkPhysicalDevice Device) const
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
	}

	bool VulkanRender::IsInstanceExtensionSupported(const char* Extension) const
	{
		const static std::vector<VkExtensionProperties>& AvalibleExtensions = []()
		{
			uint32_t ExtensionsCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionsCount, nullptr);

			std::vector<VkExtensionProperties> Extensions(ExtensionsCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionsCount, Extensions.data());

			return Extensions;
		}();

		return std::ranges::any_of(AvalibleExtensions, [Extension](VkExtensionProperties AvalibleExtension) {
				return std::strcmp(Extension, AvalibleExtension.extensionName);
			});
	}

	// todo validate function
	bool VulkanRender::IsDeviceSuitable(VkPhysicalDevice Device) const
	{
		/*
		// ID, name, type, vendor, etc
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(Device, &Properties);

		// geo shader, tess shader, wide lines, etc
		VkPhysicalDeviceFeatures Features;
		vkGetPhysicalDeviceFeatures(Device, &Features);
		*/

		QueueFamilyIndices Indices = SetupQueueFamilies(Device);
		return Indices.IsValid();
	}
}
