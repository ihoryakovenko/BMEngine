#include <vector>
#include <algorithm>
#include <cassert>

#include "VulkanRender.h"
#include "Util/Log.h"
#include "Window.h"

namespace Core
{
	bool VulkanRender::Init(Window* NewWindow)
	{
		assert(NewWindow != nullptr);
		_Window = NewWindow;
		if (!_Instance.CreateInstance())
		{
			return false;
		}

		VulkanUtil::SetupDebugMessenger(_Instance._VulkanInstance);

		_Surface = _Window->CreateSurface(_Instance._VulkanInstance, nullptr);
		if (_Surface == nullptr)
		{
			return false;
		}

		if (!_MainDevice.SetupPhysicalDevice(_Instance._VulkanInstance, _Surface))
		{
			return false;
		}

		if (!_MainDevice.CreateLogicalDevice())
		{
			return false;
		}

		_Queues.SetupQueues(_MainDevice._LogicalDevice, _MainDevice._PhysicalDeviceIndices);

		if (!_Swapchain.CreateSwapchain(_MainDevice, *_Window, _Surface))
		{
			return false;
		}

		return true;
	}

	void VulkanRender::Cleanup()
	{
		_Swapchain.CleanupSwapchain(_MainDevice._LogicalDevice);
		vkDestroySurfaceKHR(_Instance._VulkanInstance, _Surface, nullptr); // todo check
		vkDestroyDevice(_MainDevice._LogicalDevice, nullptr);
		VulkanUtil::DestroyDebugMessenger(_Instance._VulkanInstance);
		vkDestroyInstance(_Instance._VulkanInstance, nullptr);
	}
}
