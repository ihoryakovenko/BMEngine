#include <vector>
#include <algorithm>
#include <cassert>

#include "VulkanRender.h"
#include "VulkanUtil.h"
#include "Util/Log.h"

namespace Core
{
	bool VulkanRender::Init(Window* NewWindow)
	{
		assert(NewWindow != nullptr);
		_Window = NewWindow;
		if (!CreateInstance())
		{
			return false;
		}

		if (!SetupPhysicalDevice())
		{
			return false;
		}

		if (!CreateLogicalDevice())
		{
			return false;
		}

		return true;
	}

	void VulkanRender::Cleanup()
	{
		vkDestroyDevice(_LogicalDevice, nullptr);
		vkDestroyInstance(_Instance, nullptr);
	}

	bool VulkanRender::CreateInstance()
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
		std::vector<const char*> InstanceExtensions;
		if (!VulkanUtil::GetRequiredExtensions(InstanceExtensions))
		{
			return false;
		}

		// CreateInfo info initialization
		VkInstanceCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &ApplicationInfo;

		VulkanUtil::GetEnabledValidationLayers(CreateInfo.enabledLayerCount, CreateInfo.ppEnabledLayerNames);

		CreateInfo.enabledExtensionCount = static_cast<uint32_t>(InstanceExtensions.size());
		CreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

		// Create instance
		const VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &_Instance);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
			return false;
		}

		return true;
	}

	bool VulkanRender::CreateLogicalDevice()
	{
		QueueFamilyIndices Indices = QueueFamilyIndices::GetQueueFamilies(_PhysicalDevice);
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

		const VkResult Result = vkCreateDevice(_PhysicalDevice, &DeviceCreateInfo, nullptr, &_LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
			return false;
		}

		// Queues are created at the same time as the device
		// So we want to handle them
		vkGetDeviceQueue(_LogicalDevice, Indices._GraphicsFamily, 0, &GraphicsQueue);

		return true;
	}

	bool VulkanRender::SetupPhysicalDevice()
	{
		uint32_t DevicesCount = 0;
		vkEnumeratePhysicalDevices(_Instance, &DevicesCount, nullptr);
		if (DevicesCount <= 0)
		{
			return false;
		}

		std::vector<VkPhysicalDevice> DeviceList(DevicesCount);
		vkEnumeratePhysicalDevices(_Instance, &DevicesCount, DeviceList.data());

		for (const auto& Device : DeviceList)
		{
			if (VulkanUtil::IsDeviceSuitable(Device))
			{
				_PhysicalDevice = Device;
				return true;
			}
		}

		Util::Log().Error("No physical devices found");
		return false;
	}
}
