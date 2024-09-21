#include "VulkanRenderingSystem.h"

#include "Util/Util.h"

bool Core::VulkanRenderingSystem::Init(GLFWwindow* Window)
{
	const char* ValidationLayers[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	const uint32_t ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

	const char* ValidationExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
	const uint32_t ValidationExtensionsSize = Util::EnableValidationLayers ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

	const char* DeviceExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	const uint32_t DeviceExtensionsSize = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

	AvailableExtensions = CreateAvailableExtensionPropertiesData();
	RequiredExtensions = CreateRequiredInstanceExtensionsData(ValidationExtensions, ValidationExtensionsSize);
	
	if (!CheckRequiredInstanceExtensionsSupport(AvailableExtensions, RequiredExtensions))
	{
		return false;
	}

	DestroyExtensionPropertiesData(AvailableExtensions);

	if (Util::EnableValidationLayers)
	{
		LayerPropertiesData = CreateAvailableInstanceLayerPropertiesData();
		if (!CheckValidationLayersSupport(LayerPropertiesData, ValidationLayers, ValidationLayersSize))
		{
			return false;
		}

		DestroyAvailableInstanceLayerPropertiesData(LayerPropertiesData);
	}

	Instance = Core::CreateMainInstance(RequiredExtensions, Util::EnableValidationLayers, ValidationLayers, ValidationLayersSize);
	DestroyRequiredInstanceExtensionsData(RequiredExtensions);

	DevicesData = CreatePhysicalDevicesData(Instance.VulkanInstance);

	VkSurfaceKHR Surface = nullptr;
	if (glfwCreateWindowSurface(Instance.VulkanInstance, Window, nullptr, &Surface) != VK_SUCCESS)
	{
		Util::Log().GlfwLogError();
		assert(false);
	}

	DeviceInstance Device;
	bool IsDeviceFound = false;
	for (uint32_t i = 0; i < DevicesData.Count; ++i)
	{
		Device.PhysicalDevice = DevicesData.DeviceList[i];

		DeviceExtensionsData = CreateDeviceExtensionPropertiesData(Device.PhysicalDevice);
		FamilyPropertiesData = CreateQueueFamilyPropertiesData(Device.PhysicalDevice);

		Device.Indices = GetPhysicalDeviceIndices(FamilyPropertiesData, Device.PhysicalDevice, Surface);
		vkGetPhysicalDeviceProperties(Device.PhysicalDevice, &Device.Properties);
		vkGetPhysicalDeviceFeatures(Device.PhysicalDevice, &Device.AvailableFeatures);

		IsDeviceFound = CheckDeviceSuitability(Device, DeviceExtensions, DeviceExtensionsSize, DeviceExtensionsData, Surface);

		DestroyQueueFamilyPropertiesData(FamilyPropertiesData);
		DestroyExtensionPropertiesData(DeviceExtensionsData);

		if (IsDeviceFound)
		{
			IsDeviceFound = true;
			break;
		}
	}

	assert(IsDeviceFound);
	DestroyPhysicalDevicesData(DevicesData);

	LogicalDevice = CreateLogicalDevice(Device.PhysicalDevice, Device.Indices, DeviceExtensions, DeviceExtensionsSize);

	RenderInstance.VulkanInstance = Instance.VulkanInstance;
	RenderInstance.Device = Device; //Avoid copy;
	RenderInstance.LogicalDevice = LogicalDevice;
	InitVulkanRenderInstance(RenderInstance, Surface, Window);


	return true;
}

void Core::VulkanRenderingSystem::DeInit()
{
	DestroyRequiredInstanceExtensionsData(RequiredExtensions);
	DestroyExtensionPropertiesData(AvailableExtensions);
	DestroyAvailableInstanceLayerPropertiesData(LayerPropertiesData);
	DestroyPhysicalDevicesData(DevicesData);
	DestroyExtensionPropertiesData(DeviceExtensionsData);
	DestroyQueueFamilyPropertiesData(FamilyPropertiesData);

	
	DeinitVulkanRenderInstance(RenderInstance);
	vkDestroyDevice(RenderInstance.LogicalDevice, nullptr);
	DestroyMainInstance(Instance);
}
