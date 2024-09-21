#include "VulkanRenderingSystem.h"

#include "Util/Util.h"

bool Core::VulkanRenderingSystem::Init()
{
	const char* ValidationLayers[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	const uint32_t ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

	const char* ValidationExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
	const uint32_t ValidationExtensionsSize = Util::EnableValidationLayers ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

	AvailableExtensions = CreateAvailableExtensionPropertiesData();
	RequiredExtensions = CreateRequiredInstanceExtensionsData(ValidationExtensions, ValidationExtensionsSize);
	
	if (!CheckRequiredInstanceExtensionsSupport(AvailableExtensions, RequiredExtensions))
	{
		return false;
	}

	if (Util::EnableValidationLayers)
	{
		LayerPropertiesData = CreateAvailableInstanceLayerPropertiesData();
		if (!CheckValidationLayersSupport(LayerPropertiesData, ValidationLayers, ValidationLayersSize))
		{
			return false;
		}
	}

	Instance = Core::CreateMainInstance(RequiredExtensions, Util::EnableValidationLayers, ValidationLayers, ValidationLayersSize);
	return true;
}

void Core::VulkanRenderingSystem::DeInit()
{
	DestroyRequiredInstanceExtensionsData(RequiredExtensions);
	DestroyExtensionPropertiesData(AvailableExtensions);
	DestroyAvailableInstanceLayerPropertiesData(LayerPropertiesData);
	DestroyMainInstance(Instance);
}
