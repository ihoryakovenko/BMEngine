#pragma once

#include "VulkanCoreTypes.h"

namespace Core
{
	class VulkanRenderingSystem
	{
	public:
		bool Init(GLFWwindow* Window);
		void DeInit();

	public:
		MainInstance Instance;
		VulkanRenderInstance RenderInstance;

	private:
		ExtensionPropertiesData AvailableExtensions;
		RequiredInstanceExtensionsData RequiredExtensions;
		AvailableInstanceLayerPropertiesData LayerPropertiesData;
		PhysicalDevicesData DevicesData;
		ExtensionPropertiesData DeviceExtensionsData;
		QueueFamilyPropertiesData FamilyPropertiesData;

		VkDevice LogicalDevice = nullptr;
	};
}