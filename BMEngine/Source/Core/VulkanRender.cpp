#include <vector>
#include <algorithm>
#include <cassert>
#include <unordered_set>

#include "VulkanRender.h"
#include "Util/Log.h"
#include "Window.h"

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

		VulkanUtil::SetupDebugMessenger(_Instance);

		_Surface = _Window->CreateSurface(_Instance, nullptr);
		if (_Surface == nullptr)
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
		vkDestroySurfaceKHR(_Instance, _Surface, nullptr); // todo check
		vkDestroyDevice(_LogicalDevice, nullptr);
		VulkanUtil::DestroyDebugMessenger(_Instance);
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
		if (!VulkanUtil::GetRequiredInstanceExtensions(InstanceExtensions))
		{
			return false;
		}

		// CreateInfo info initialization
		VkInstanceCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &ApplicationInfo;

		VulkanUtil::GetEnabledValidationLayers(CreateInfo);

		CreateInfo.enabledExtensionCount = static_cast<uint32_t>(InstanceExtensions.size());
		CreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

		VulkanUtil::GetDebugCreateInfo(CreateInfo);

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
		const float Priority = 1.0f;

		// One family can suppurt graphics and presentation
		// In that case create mulltiple VkDeviceQueueCreateInfo
		std::unordered_set<int> FamilyIndices = { _PhysicalDeviceIndices._GraphicsFamily,
			_PhysicalDeviceIndices._PresentationFamily };

		const size_t FamilyIndicesSize = FamilyIndices.size();
		std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos(FamilyIndicesSize);

		int i = 0;
		for (int QueueFamilyIndex : FamilyIndices)
		{
			QueueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfos[i].queueFamilyIndex = static_cast<uint32_t>(QueueFamilyIndex);
			QueueCreateInfos[i].queueCount = 1;
			QueueCreateInfos[i].pQueuePriorities = &Priority;
			++i;
		}

		VkPhysicalDeviceFeatures DeviceFeatures = {};

		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(FamilyIndicesSize);
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
		DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanUtil::_DeviceExtensions.size());
		DeviceCreateInfo.ppEnabledExtensionNames = VulkanUtil::_DeviceExtensions.data();
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		// Queues are created at the same time as the device
		const VkResult Result = vkCreateDevice(_PhysicalDevice, &DeviceCreateInfo, nullptr, &_LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
			return false;
		}

		vkGetDeviceQueue(_LogicalDevice, static_cast<uint32_t>(_PhysicalDeviceIndices._GraphicsFamily), 0, &_GraphicsQueue);
		vkGetDeviceQueue(_LogicalDevice, static_cast<uint32_t>(_PhysicalDeviceIndices._PresentationFamily), 0, &_PresentationQueue);

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
			// todo validate
			if (VulkanUtil::IsDeviceSuitable(Device) && _PhysicalDeviceIndices.Init(Device, _Surface)
				&& _SwapchainDetails.Init(Device, _Surface))
			{
				_PhysicalDevice = Device;
				return true;
			}
		}

		Util::Log().Error("No physical devices found");
		return false;
	}
}
