//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Log.h"

#include <iostream>
#include <vector>

#include "Core/Window.h"
#include "Core/VulkanUtil.h"
#include "Core/VulkanCoreTypes.h"

int main()
{
	const int InitResult = glfwInit();
	if (InitResult == GL_FALSE)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	Core::Window Window;
	if (!Window.Init())
	{
		return -1;
	}

	Core::Instance _Instance;
	if (!_Instance.CreateInstance())
	{
		return -1;
	}

	Core::VulkanUtil::SetupDebugMessenger(_Instance._VulkanInstance);

	VkSurfaceKHR _Surface = Window.CreateSurface(_Instance._VulkanInstance, nullptr);
	if (_Surface == nullptr)
	{
		return -1;
	}

	Core::MainDevice _MainDevice;
	if (!_MainDevice.SetupPhysicalDevice(_Instance._VulkanInstance, _Surface))
	{
		return -1;
	}

	if (!_MainDevice.CreateLogicalDevice())
	{
		return -1;
	}

	Core::Queues _Queues;
	_Queues.SetupQueues(_MainDevice._LogicalDevice, _MainDevice._PhysicalDeviceIndices);

	Core::Swapchain _Swapchain;
	if (!_Swapchain.CreateSwapchain(_MainDevice, Window, _Surface))
	{
		return -1;
	}

	Core::GraphicsPipeline _GraphicsPipeline;
	if (!_GraphicsPipeline.CreateGraphicsPipeline())
	{
		return -1;
	}

	while (!glfwWindowShouldClose(Window._Window))
	{
		glfwPollEvents();
	}

	_Swapchain.CleanupSwapchain(_MainDevice._LogicalDevice);
	vkDestroySurfaceKHR(_Instance._VulkanInstance, _Surface, nullptr); // todo check
	vkDestroyDevice(_MainDevice._LogicalDevice, nullptr);
	Core::VulkanUtil::DestroyDebugMessenger(_Instance._VulkanInstance);
	vkDestroyInstance(_Instance._VulkanInstance, nullptr);

	Window.Destroy();
	glfwTerminate();

	return 0;
}