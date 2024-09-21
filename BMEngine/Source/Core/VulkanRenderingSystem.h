#pragma once

#include "VulkanCoreTypes.h"

namespace Core
{
	class VulkanRenderingSystem
	{
	public:
		bool Init();
		void DeInit();

	public:
		MainInstance Instance;

	private:
		ExtensionPropertiesData AvailableExtensions;
		RequiredInstanceExtensionsData RequiredExtensions;
		AvailableInstanceLayerPropertiesData LayerPropertiesData;
	};
}