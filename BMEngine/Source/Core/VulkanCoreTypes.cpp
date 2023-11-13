#include "VulkanCoreTypes.h"

#include "Util/Util.h"

#include <algorithm>

namespace Core
{
	bool InitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data, const char** ValidationExtensions,
		uint32_t ValidationExtensionsCount, bool EnumerateInstanceLayerProperties)
	{
		// Get count part
		const char** RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&Data.RequiredExtensionsCount);
		if (Data.RequiredExtensionsCount == 0)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceExtensionProperties result is {}", static_cast<int>(Result));
			return false;
		}

		if (EnumerateInstanceLayerProperties)
		{
			Result = vkEnumerateInstanceLayerProperties(&Data.AvalibleValidationLayersCount, nullptr);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkEnumerateInstanceLayerProperties result is {}", static_cast<int>(Result));
				return false;
			}
		}

		// Allocation part
		Data.RequiredAndValidationExtensions = static_cast<const char**>(Util::Memory::Allocate((ValidationExtensionsCount + Data.RequiredExtensionsCount) * sizeof(const char**)));
		Data.AvalibleExtensions = static_cast<VkExtensionProperties*>(Util::Memory::Allocate(Data.AvalibleExtensionsCount * sizeof(VkExtensionProperties)));
		Data.AvalibleValidationLayers = static_cast<VkLayerProperties*>(Util::Memory::Allocate(Data.AvalibleValidationLayersCount * sizeof(VkLayerProperties)));

		// Data structuring part
		Data.RequiredAndValidationExtensionsCount = Data.RequiredExtensionsCount + ValidationExtensionsCount;
		Data.RequiredInstanceExtensions = Data.RequiredAndValidationExtensions + ValidationExtensionsCount;

		// Data assignment part
		for (uint32_t i = 0; i < ValidationExtensionsCount; ++i)
		{
			Data.RequiredAndValidationExtensions[i] = ValidationExtensions[i];
		}

		for (uint32_t i = 0; i < Data.RequiredExtensionsCount; ++i)
		{
			Data.RequiredInstanceExtensions[i] = RequiredInstanceExtensions[i];
		}

		vkEnumerateInstanceExtensionProperties(nullptr, &Data.AvalibleExtensionsCount, Data.AvalibleExtensions);
		vkEnumerateInstanceLayerProperties(&Data.AvalibleValidationLayersCount, Data.AvalibleValidationLayers);

		return true;
	}

	void DeinitVkInstanceCreateInfoSetupData(VkInstanceCreateInfoSetupData& Data)
	{
		Util::Memory::Deallocate(Data.RequiredAndValidationExtensions);
		Util::Memory::Deallocate(Data.AvalibleExtensions);
		Util::Memory::Deallocate(Data.AvalibleValidationLayers);
	}

	bool CheckRequiredInstanceExtensionsSupport(const VkInstanceCreateInfoSetupData& Data)
	{
		for (uint32_t i = 0; i < Data.RequiredAndValidationExtensionsCount; ++i)
		{
			bool IsExtensionSupported = false;
			for (uint32_t j = 0; j < Data.AvalibleExtensionsCount; ++j)
			{
				if (std::strcmp(Data.RequiredAndValidationExtensions[i], Data.AvalibleExtensions[j].extensionName) == 0)
				{
					IsExtensionSupported = true;
					break;
				}
			}

			if (!IsExtensionSupported)
			{
				Util::Log().Error("Extension {} unsupported", Data.RequiredAndValidationExtensions[i]);
				return false;
			}
		}

		return true;
	}

	bool CheckValidationLayersSupport(const VkInstanceCreateInfoSetupData& Data, const char** ValidationLeyersToCheck,
		uint32_t ValidationLeyersToCheckSize)
	{
		for (uint32_t i = 0; i < ValidationLeyersToCheckSize; ++i)
		{
			bool IsLayerAvalible = false;
			for (uint32_t j = 0; j < Data.AvalibleValidationLayersCount; ++j)
			{
				if (std::strcmp(ValidationLeyersToCheck[i], Data.AvalibleValidationLayers[j].layerName) == 0)
				{
					IsLayerAvalible = true;
					break;
				}
			}

			if (!IsLayerAvalible)
			{
				Util::Log().Error("Validation layer {} unsupported", ValidationLeyersToCheck[i]);
				return false;
			}
		}

		return true;
	}

	bool InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		// Get count part
		VkResult Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Data.AvalibleDeviceExtensionsCount, nullptr);
		if (Data.AvalibleDeviceExtensionsCount == 0)
		{
			Util::Log().Error("vkEnumerateDeviceExtensionProperties result is {}", static_cast<int>(Result));
			return false;
		}

		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Data.FamilyPropertiesCount, nullptr);

		Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &Data.SurfaceFormatsCount, nullptr);
		if (Data.SurfaceFormatsCount == 0)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfaceFormatsKHR result is {}", static_cast<int>(Result));
			return false;
		}

		Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Data.PresentModesCount, nullptr);
		if (Data.PresentModesCount == 0)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfacePresentModesKHR result is {}", static_cast<int>(Result));
			return false;
		}

		// Allocation part
		Data.AvalibleDeviceExtensions = static_cast<VkExtensionProperties*>(Util::Memory::Allocate(Data.AvalibleDeviceExtensionsCount * sizeof(VkExtensionProperties)));
		Data.FamilyProperties = static_cast<VkQueueFamilyProperties*>(Util::Memory::Allocate(Data.FamilyPropertiesCount * sizeof(VkQueueFamilyProperties)));
		Data.SurfaceFormats = static_cast<VkSurfaceFormatKHR*>(Util::Memory::Allocate(Data.SurfaceFormatsCount * sizeof(VkSurfaceFormatKHR)));
		Data.PresentModes = static_cast<VkPresentModeKHR*>(Util::Memory::Allocate(Data.PresentModesCount * sizeof(VkPresentModeKHR)));

		// Data assignment part
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Data.AvalibleDeviceExtensionsCount, Data.AvalibleDeviceExtensions);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Data.FamilyPropertiesCount, Data.FamilyProperties);
		vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &Data.SurfaceFormatsCount, Data.SurfaceFormats);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Data.PresentModesCount, Data.PresentModes);

		return true;
	}

	void DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData& Data)
	{
		Util::Memory::Deallocate(Data.AvalibleDeviceExtensions);
		Util::Memory::Deallocate(Data.FamilyProperties);
		Util::Memory::Deallocate(Data.SurfaceFormats);
		Util::Memory::Deallocate(Data.PresentModes);
	}

	bool CheckDeviceExtensionsSupport(const VkPhysicalDeviceSetupData& Data, const char** ExtensionsToCheck, uint32_t ExtensionsToCheckSize)
	{
		for (uint32_t i = 0; i < ExtensionsToCheckSize; ++i)
		{
			bool IsDeviceExtensionSupported = false;
			for (uint32_t j = 0; j < Data.AvalibleDeviceExtensionsCount; ++j)
			{
				if (std::strcmp(ExtensionsToCheck[i], Data.AvalibleDeviceExtensions[j].extensionName) == 0)
				{
					IsDeviceExtensionSupported = true;
					break;
				}
			}

			if (!IsDeviceExtensionSupported)
			{
				Util::Log().Error("Device extension {} unsupported", ExtensionsToCheck[i]);
				return false;
			}
		}

		return true;
	}

	// TODO check function
	// In current realization if GraphicsFamily is valid but if PresentationFamily is not valid
	// GraphicsFamily could be overridden on next iteration even when it is valid
	PhysicalDeviceIndices GetPhysicalDeviceIndices(const VkPhysicalDeviceSetupData& Data, VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		PhysicalDeviceIndices Indices;

		for (uint32_t i = 0; i < Data.FamilyPropertiesCount; ++i)
		{
			// check if Queue is graphics type
			if (Data.FamilyProperties[i].queueCount > 0 && Data.FamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				// if we QueueFamily[i] graphics type but not presentation type
				// and QueueFamily[i + 1] graphics type and presentation type
				// then we rewrite GraphicsFamily
				// toto check what is better rewrite or have different QueueFamilys
				Indices.GraphicsFamily = i;
			}

			// check if Queue is presentation type (can be graphics and presentation)
			VkBool32 PresentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &PresentationSupport);
			if (Data.FamilyProperties[i].queueCount > 0 && PresentationSupport)
			{
				Indices.PresentationFamily = i;
			}

			if (Indices.GraphicsFamily >= 0 && Indices.PresentationFamily >= 0)
			{
				break;
			}
		}

		return Indices;
	}

	bool CreateVulkanBuffer(const VulkanRenderInstance& RenderInstance, VkDeviceSize BufferSize,
		VkBufferUsageFlags BufferUsage, VkMemoryPropertyFlags BufferProperties, VulkanBuffer& OutBuffer)
	{
		VkBufferCreateInfo BufferCreateInfo = {};
		BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.size = BufferSize;		// Size of buffer (size of 1 vertex * number of vertices)
		BufferCreateInfo.usage = BufferUsage;		// Multiple types of buffer possible, we want Vertex Buffer
		BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Similar to Swap Chain images, can share vertex buffers

		VkResult Result = vkCreateBuffer(RenderInstance.LogicalDevice, &BufferCreateInfo, nullptr, &OutBuffer.Buffer);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		// GET BUFFER MEMORY REQUIREMENTS
		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(RenderInstance.LogicalDevice, OutBuffer.Buffer, &MemoryRequirements);

		// Function FindMemoryTypeIndex
		VkPhysicalDeviceMemoryProperties MemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(RenderInstance.PhysicalDevice, &MemoryProperties);

		uint32_t MemoryTypeIndex = 0;
		for (; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++)
		{
			if ((MemoryRequirements.memoryTypeBits & (1 << MemoryTypeIndex))														// Index of memory type must match corresponding bit in allowedTypes
				&& (MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & BufferProperties) == BufferProperties)	// Desired property bit flags are part of memory type's property flags
			{
				// This memory type is valid, so return its index
				break;
			}
		}
		// Function end FindMemoryTypeIndex

		// ALLOCATE MEMORY TO BUFFER
		VkMemoryAllocateInfo MemoryAllocInfo = {};
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;		// Index of memory type on Physical Device that has required bit flags			

		Result = vkAllocateMemory(RenderInstance.LogicalDevice, &MemoryAllocInfo, nullptr, &OutBuffer.BufferMemory);
		if (Result != VK_SUCCESS)
		{
			return false;
		}

		// Allocate memory to given vertex buffer
		vkBindBufferMemory(RenderInstance.LogicalDevice, OutBuffer.Buffer, OutBuffer.BufferMemory, 0);

		return true;
	}

	void DestroyVulkanBuffer(const VulkanRenderInstance& RenderInstance, VulkanBuffer& Buffer)
	{
		vkDestroyBuffer(RenderInstance.LogicalDevice, Buffer.Buffer, nullptr);
		vkFreeMemory(RenderInstance.LogicalDevice, Buffer.BufferMemory, nullptr);
	}

	void CopyBuffer(const VulkanRenderInstance& RenderInstance, VkBuffer SourceBuffer,
		VkBuffer DstinationBuffer, VkDeviceSize BufferSize)
	{
		// Command buffer to hold transfer commands
		VkCommandBuffer TransferCommandBuffer;

		// Command Buffer details
		VkCommandBufferAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = RenderInstance.GraphicsCommandPool;
		AllocInfo.commandBufferCount = 1;

		// Allocate command buffer from pool
		vkAllocateCommandBuffers(RenderInstance.LogicalDevice, &AllocInfo, &TransferCommandBuffer);

		// Information to begin the command buffer record
		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	// We're only using the command buffer once, so set up for one time submit

		// Begin recording transfer commands
		vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo);

		// Region of data to copy from and to
		VkBufferCopy bufferCopyRegion = {};
		bufferCopyRegion.srcOffset = 0;
		bufferCopyRegion.dstOffset = 0;
		bufferCopyRegion.size = BufferSize;

		// Command to copy src buffer to dst buffer
		vkCmdCopyBuffer(TransferCommandBuffer, SourceBuffer, DstinationBuffer, 1, &bufferCopyRegion);

		// End commands
		vkEndCommandBuffer(TransferCommandBuffer);

		// Queue submission information
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &TransferCommandBuffer;

		// Submit transfer command to transfer queue and wait until it finishes
		vkQueueSubmit(RenderInstance.GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(RenderInstance.GraphicsQueue);

		// Free temporary command buffer back to pool
		vkFreeCommandBuffers(RenderInstance.LogicalDevice, RenderInstance.GraphicsCommandPool, 1, &TransferCommandBuffer);
	}

	bool InitMainInstance(MainInstance& Instance, bool IsValidationLayersEnabled)
	{
		const char* ValidationExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};

		const uint32_t ValidationExtensionsSize = IsValidationLayersEnabled ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

		Core::VkInstanceCreateInfoSetupData InstanceCreateInfoData;
		if (!Core::InitVkInstanceCreateInfoSetupData(InstanceCreateInfoData, ValidationExtensions, ValidationExtensionsSize, IsValidationLayersEnabled))
		{
			return false;
		}

		if (!Core::CheckRequiredInstanceExtensionsSupport(InstanceCreateInfoData))
		{
			Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);
			return false;
		}

		VkApplicationInfo ApplicationInfo = { };
		ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		ApplicationInfo.pApplicationName = "Blank App";
		ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.pEngineName = "BMEngine";
		ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0); // todo set from settings
		ApplicationInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.pApplicationInfo = &ApplicationInfo;

		VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = { };

		if (IsValidationLayersEnabled)
		{
			MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			MessengerCreateInfo.pfnUserCallback = Util::MessengerDebugCallback;
			MessengerCreateInfo.pUserData = nullptr;

			const char* ValidationLayers[] = {
				"VK_LAYER_KHRONOS_validation"
			};
			const uint32_t ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

			Core::CheckValidationLayersSupport(InstanceCreateInfoData, ValidationLayers, ValidationLayersSize);

			CreateInfo.enabledExtensionCount = InstanceCreateInfoData.RequiredAndValidationExtensionsCount;
			CreateInfo.ppEnabledExtensionNames = InstanceCreateInfoData.RequiredAndValidationExtensions;
			CreateInfo.pNext = &MessengerCreateInfo;
			CreateInfo.enabledLayerCount = ValidationLayersSize;
			CreateInfo.ppEnabledLayerNames = ValidationLayers;
		}
		else
		{
			CreateInfo.enabledExtensionCount = InstanceCreateInfoData.RequiredExtensionsCount;
			CreateInfo.ppEnabledExtensionNames = InstanceCreateInfoData.RequiredInstanceExtensions;
			CreateInfo.ppEnabledLayerNames = nullptr;
			CreateInfo.enabledLayerCount = 0;
			CreateInfo.pNext = nullptr;
		}

		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
			Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);

			return false;
		}

		Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);

		if (IsValidationLayersEnabled)
		{
			Util::CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);
		}

		return true;
	}

	void DeinitMainInstance(MainInstance& Instance)
	{
		if (Instance.DebugMessenger != nullptr)
		{
			Util::DestroyDebugMessenger(Instance.VulkanInstance, Instance.DebugMessenger, nullptr);
		}

		vkDestroyInstance(Instance.VulkanInstance, nullptr);
	}

	bool InitVulkanRenderInstance(VulkanRenderInstance& RenderInstance, VkInstance VulkanInstance, GLFWwindow* Window)
	{
		RenderInstance.VulkanInstance = VulkanInstance;
		RenderInstance.Window = Window;

		if (glfwCreateWindowSurface(RenderInstance.VulkanInstance, RenderInstance.Window, nullptr, &RenderInstance.Surface) != VK_SUCCESS)
		{
			Util::Log().GlfwLogError();
			return false;
		}

		// Function: SetupPhysicalDevice
		uint32_t DevicesCount = 0;
		vkEnumeratePhysicalDevices(RenderInstance.VulkanInstance, &DevicesCount, nullptr);

		VkPhysicalDevice* DeviceList = static_cast<VkPhysicalDevice*>(Util::Memory::Allocate(DevicesCount * sizeof(VkPhysicalDevice)));
		vkEnumeratePhysicalDevices(RenderInstance.VulkanInstance, &DevicesCount, DeviceList);

		Core::VkPhysicalDeviceSetupData VkPhysicalDeviceSetupData;

		Core::PhysicalDeviceIndices PhysicalDeviceIndices;
		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };

		const char* DeviceExtensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		const uint32_t DeviceExtensionsSize = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

		for (uint32_t i = 0; i < DevicesCount; ++i)
		{
			/* ID, name, type, vendor, etc
			VkPhysicalDeviceProperties Properties;
			vkGetPhysicalDeviceProperties(Device, &Properties);

			// geo shader, tess shader, wide lines, etc
			VkPhysicalDeviceFeatures Features;
			vkGetPhysicalDeviceFeatures(Device, &Features); */

			if (!Core::InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData, DeviceList[i], RenderInstance.Surface))
			{
				break;
			}

			if (!Core::CheckDeviceExtensionsSupport(VkPhysicalDeviceSetupData, DeviceExtensions, DeviceExtensionsSize))
			{
				break;
			}

			PhysicalDeviceIndices = Core::GetPhysicalDeviceIndices(VkPhysicalDeviceSetupData, DeviceList[i], RenderInstance.Surface);

			if (PhysicalDeviceIndices.GraphicsFamily < 0 || PhysicalDeviceIndices.PresentationFamily < 0)
			{
				Util::Log().Warning("PhysicalDeviceIndices are not initialized");
				break;
			}

			VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceList[i], RenderInstance.Surface, &SurfaceCapabilities);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
				break;
			}

			RenderInstance.PhysicalDevice = DeviceList[i];
		}

		Util::Memory::Deallocate(DeviceList);

		if (RenderInstance.PhysicalDevice == nullptr)
		{
			Core::DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData);
			Util::Log().Error("No physical devices found");
			return false;
		}
		// Function end: SetupPhysicalDevice

		// Function: CreateLogicalDevice
		const float Priority = 1.0f;

		// One family can suppurt graphics and presentation
		// In that case create mulltiple VkDeviceQueueCreateInfo
		VkDeviceQueueCreateInfo QueueCreateInfos[2] = {};
		uint32_t FamilyIndicesSize = 1;

		QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfos[0].queueFamilyIndex = static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily);
		QueueCreateInfos[0].queueCount = 1;
		QueueCreateInfos[0].pQueuePriorities = &Priority;

		if (PhysicalDeviceIndices.GraphicsFamily != PhysicalDeviceIndices.PresentationFamily)
		{
			QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfos[1].queueFamilyIndex = static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily);
			QueueCreateInfos[1].queueCount = 1;
			QueueCreateInfos[1].pQueuePriorities = &Priority;

			++FamilyIndicesSize;
		}

		VkPhysicalDeviceFeatures DeviceFeatures = {};

		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = (FamilyIndicesSize);
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
		DeviceCreateInfo.enabledExtensionCount = DeviceExtensionsSize;
		DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions;
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		// Queues are created at the same time as the device
		VkResult Result = vkCreateDevice(RenderInstance.PhysicalDevice, &DeviceCreateInfo, nullptr, &RenderInstance.LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
			return false;
		}
		// Function end: CreateLogicalDevice

		// Function: SetupQueues
		vkGetDeviceQueue(RenderInstance.LogicalDevice, static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily), 0, &RenderInstance.GraphicsQueue);
		vkGetDeviceQueue(RenderInstance.LogicalDevice, static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily), 0, &RenderInstance.PresentationQueue);

		if (RenderInstance.GraphicsQueue == nullptr && RenderInstance.PresentationQueue == nullptr)
		{
			return false;
		}
		// Function end: SetupQueues

		// Function: GetBestSurfaceFormat
		// Return most common format
		VkSurfaceFormatKHR SurfaceFormat = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };
		// All formats avalible
		if (VkPhysicalDeviceSetupData.SurfaceFormatsCount == 1 && VkPhysicalDeviceSetupData.SurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			SurfaceFormat = { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
		else
		{
			for (uint32_t i = 0; i < VkPhysicalDeviceSetupData.SurfaceFormatsCount; ++i)
			{
				VkSurfaceFormatKHR Format = VkPhysicalDeviceSetupData.SurfaceFormats[i];
				if ((Format.format == VK_FORMAT_R8G8B8_UNORM || Format.format == VK_FORMAT_B8G8R8A8_UNORM)
					&& Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					SurfaceFormat = Format;
				}
			}
		}

		if (SurfaceFormat.format == VK_FORMAT_UNDEFINED)
		{
			Util::Log().Error("SurfaceFormat is undefined");
			return false;
		}
		// Function end: GetBestSurfaceFormat

		// Function: GetBestPresentationMode
		// Optimal presentation mode
		VkPresentModeKHR PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
		for (uint32_t i = 0; i < VkPhysicalDeviceSetupData.PresentModesCount; ++i)
		{
			if (VkPhysicalDeviceSetupData.PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				PresentationMode = VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		Core::DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData);

		// Has to be present by spec
		if (PresentationMode != VK_PRESENT_MODE_MAILBOX_KHR)
		{
			Util::Log().Warning("Using default VK_PRESENT_MODE_FIFO_KHR");
		}
		// Function end: GetBestPresentationMode

		// Function: GetBestSwapExtent
		if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			RenderInstance.SwapExtent = SurfaceCapabilities.currentExtent;
		}
		else
		{
			int Width;
			int Height;
			glfwGetFramebufferSize(RenderInstance.Window, &Width, &Height);

			Width = std::clamp(static_cast<uint32_t>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
			Height = std::clamp(static_cast<uint32_t>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

			RenderInstance.SwapExtent = { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
		}
		// Function end: GetBestSwapExtent

		// How many images are in the swap chain
		// Get 1 more then the minimum to allow triple buffering
		uint32_t ImageCount = SurfaceCapabilities.minImageCount + 1;

		// If maxImageCount > 0, then limitless
		if (SurfaceCapabilities.maxImageCount > 0
			&& SurfaceCapabilities.maxImageCount < ImageCount)
		{
			ImageCount = SurfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR SwapchainCreateInfo = {};
		SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapchainCreateInfo.surface = RenderInstance.Surface;
		SwapchainCreateInfo.imageFormat = SurfaceFormat.format;
		SwapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
		SwapchainCreateInfo.presentMode = PresentationMode;
		SwapchainCreateInfo.imageExtent = RenderInstance.SwapExtent;
		SwapchainCreateInfo.minImageCount = ImageCount;
		SwapchainCreateInfo.imageArrayLayers = 1;
		SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		SwapchainCreateInfo.preTransform = SurfaceCapabilities.currentTransform;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle windows blending
		SwapchainCreateInfo.clipped = VK_TRUE;

		uint32_t Indices[] = {
			static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily),
			static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily)
		};

		if (PhysicalDeviceIndices.GraphicsFamily != PhysicalDeviceIndices.PresentationFamily)
		{
			// Less efficient mode
			SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			SwapchainCreateInfo.queueFamilyIndexCount = 2;
			SwapchainCreateInfo.pQueueFamilyIndices = Indices;
		}
		else
		{
			SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			SwapchainCreateInfo.queueFamilyIndexCount = 0;
			SwapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		// Used if old cwap chain been destroyed and this one replaces it
		SwapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		Result = vkCreateSwapchainKHR(RenderInstance.LogicalDevice, &SwapchainCreateInfo, nullptr, &RenderInstance.VulkanSwapchain);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateSwapchainKHR result is {}", static_cast<int>(Result));
			return false;
		}

		// Get swap chain images
		vkGetSwapchainImagesKHR(RenderInstance.LogicalDevice, RenderInstance.VulkanSwapchain, &RenderInstance.SwapchainImagesCount, nullptr);

		VkImage* Images = static_cast<VkImage*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkImage)));
		vkGetSwapchainImagesKHR(RenderInstance.LogicalDevice, RenderInstance.VulkanSwapchain, &RenderInstance.SwapchainImagesCount, Images);

		VkImageViewCreateInfo ViewCreateInfo = {};
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = SurfaceFormat.format;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// Subresources allow the view to view only a part of an image
		ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;

		RenderInstance.ImageViews = static_cast<VkImageView*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkImageView)));

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; ++i)
		{
			ViewCreateInfo.image = Images[i];

			VkImageView ImageView;
			Result = vkCreateImageView(RenderInstance.LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkCreateImageView result is {}", static_cast<int>(Result));
				Util::Memory::Deallocate(Images);
				Util::Memory::Deallocate(RenderInstance.ImageViews);
				return false;
			}

			RenderInstance.ImageViews[i] = ImageView;
		}

		Util::Memory::Deallocate(Images);
		// Function end: CreateSwapchain

			// Function CreateRenderPath
		// Colour attachment of render pass
		VkAttachmentDescription ColourAttachment = {};
		ColourAttachment.format = SurfaceFormat.format;						// Format to use for attachment
		ColourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples to write for multisampling
		ColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Describes what to do with attachment before rendering
		ColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;			// Describes what to do with attachment after rendering
		ColourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Describes what to do with stencil before rendering
		ColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Describes what to do with stencil after rendering

		// Framebuffer data will be stored as an image, but images can be given different data layouts
		// to give optimal use for certain operations
		ColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts
		ColourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass (to change to)

		// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
		VkAttachmentReference ColourAttachmentReference = {};
		ColourAttachmentReference.attachment = 0;
		ColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Information about a particular subpass the Render Pass is using
		VkSubpassDescription SubpassDescription = {};
		SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// Pipeline type subpass is to be bound to
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.pColorAttachments = &ColourAttachmentReference;

		// Need to determine when layout transitions occur using subpass dependencies
		const uint32_t SubpassDependenciesSize = 2;
		VkSubpassDependency SubpassDependencies[SubpassDependenciesSize];

		// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		// Transition must happen after...
		SubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
		SubpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// Pipeline stage
		SubpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// Stage access mask (memory access)
		// But must happen before...
		SubpassDependencies[0].dstSubpass = 0;
		SubpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		SubpassDependencies[0].dependencyFlags = 0;


		// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		// Transition must happen after...
		SubpassDependencies[1].srcSubpass = 0;
		SubpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
		// But must happen before...
		SubpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		SubpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		SubpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		SubpassDependencies[1].dependencyFlags = 0;

		// Create info for Render Pass
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &ColourAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &SubpassDescription;
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(SubpassDependenciesSize);
		renderPassCreateInfo.pDependencies = SubpassDependencies;

		Result = vkCreateRenderPass(RenderInstance.LogicalDevice, &renderPassCreateInfo, nullptr, &RenderInstance.RenderPass);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateRenderPass result is {}", static_cast<int>(Result));
			return false;
		}

		// Function end CreateRenderPath

		// Function: CreateGraphicsPipeline
		// TODO FIX!!!
		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull(Util::UtilHelper::GetVertexShaderPath().data(), VertexShaderCode, "rb");

		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull(Util::UtilHelper::GetFragmentShaderPath().data(), FragmentShaderCode, "rb");


		//Build shader modules
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = VertexShaderCode.size();
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(VertexShaderCode.data());

		VkShaderModule VertexShaderModule = nullptr;
		Result = vkCreateShaderModule(RenderInstance.LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			return false;
		}

		ShaderModuleCreateInfo.codeSize = FragmentShaderCode.size();
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(FragmentShaderCode.data());

		VkShaderModule FragmentShaderModule = nullptr;
		Result = vkCreateShaderModule(RenderInstance.LogicalDevice, &ShaderModuleCreateInfo, nullptr, &FragmentShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			return false;
		}

		VkPipelineShaderStageCreateInfo VertexShaderCreateInfo = {};
		VertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		VertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		VertexShaderCreateInfo.module = VertexShaderModule;
		VertexShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo FragmentShaderCreateInfo = {};
		FragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		FragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		FragmentShaderCreateInfo.module = FragmentShaderModule;
		FragmentShaderCreateInfo.pName = "main";

		const uint32_t ShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo ShaderStages[ShaderStagesCount] = { VertexShaderCreateInfo , FragmentShaderCreateInfo };

		// How the data for a single vertex (including info such as position, colour, texture coords, normals, etc) is as a whole
		VkVertexInputBindingDescription VertexInputBindingDescription = {};
		VertexInputBindingDescription.binding = 0;									// Can bind multiple streams of data, this defines which one
		VertexInputBindingDescription.stride = sizeof(Core::Vertex);						// Size of a single vertex object
		VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// How to move between data after each vertex.
		// VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex
		// VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance

		// How the data for an attribute is defined within a vertex
		const uint32_t VertexInputBindingDescriptionCount = 2;
		VkVertexInputAttributeDescription AttributeDescriptions[VertexInputBindingDescriptionCount];

		// Position Attribute
		AttributeDescriptions[0].binding = 0;							// Which binding the data is at (should be same as above)
		AttributeDescriptions[0].location = 0;							// Location in shader where data will be read from
		AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Format the data will take (also helps define size of data)
		AttributeDescriptions[0].offset = offsetof(Core::Vertex, Position);		// Where this attribute is defined in the data for a single vertex

		// Colour Attribute
		AttributeDescriptions[1].binding = 0;
		AttributeDescriptions[1].location = 1;
		AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		AttributeDescriptions[1].offset = offsetof(Core::Vertex, Color);

		// Vertex input
		VkPipelineVertexInputStateCreateInfo VertexInputCreateInfo = {};
		VertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		VertexInputCreateInfo.pVertexBindingDescriptions = &VertexInputBindingDescription;
		VertexInputCreateInfo.vertexAttributeDescriptionCount = VertexInputBindingDescriptionCount;
		VertexInputCreateInfo.pVertexAttributeDescriptions = AttributeDescriptions;

		// Inputassembly
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = {};
		InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Viewport and scissor
		VkViewport Viewport = {};
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;
		Viewport.width = static_cast<float>(RenderInstance.SwapExtent.width);
		Viewport.height = static_cast<float>(RenderInstance.SwapExtent.height);
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = {};
		Scissor.offset = { 0, 0 };
		Scissor.extent = RenderInstance.SwapExtent;

		VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {};
		ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		ViewportStateCreateInfo.viewportCount = 1;
		ViewportStateCreateInfo.pViewports = &Viewport;
		ViewportStateCreateInfo.scissorCount = 1;
		ViewportStateCreateInfo.pScissors = &Scissor;

		// Dynamic states TODO
		// Use vkCmdSetViewport(Commandbuffer, 0, 1, &Viewport) and vkCmdSetScissor(Commandbuffer, 0, 1, &Scissor)
		//VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		//VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
		//DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		//DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(2);
		//DynamicStateCreateInfo.pDynamicStates = DynamicStates;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {};
		RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
		RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;	// How to handle filling points between vertices
		RasterizationStateCreateInfo.lineWidth = 1.0f;						// How thick lines should be when drawn
		RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		// Which face of a tri to cull
		RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;	// Winding to determine which side is front
		RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

		// Multisampling
		VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = {};
		MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
		MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

		// Blending

		VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
		ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	// Colours to apply blending to
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorBlendAttachmentState.blendEnable = VK_TRUE;													// Enable blending

		// Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
		ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;

		// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
		//			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

		ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		// Summarised: (1 * new alpha) + (0 * old alpha) = new alpharesult != VK_SUCCESS

		VkPipelineColorBlendStateCreateInfo ColourBlendingCreateInfo = {};
		ColourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		ColourBlendingCreateInfo.logicOpEnable = VK_FALSE;				// Alternative to calculations is to use logical operations
		ColourBlendingCreateInfo.attachmentCount = 1;
		ColourBlendingCreateInfo.pAttachments = &ColorBlendAttachmentState;

		// Pipeline layout
		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = 0;
		PipelineLayoutCreateInfo.pSetLayouts = nullptr;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

		Result = vkCreatePipelineLayout(RenderInstance.LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &RenderInstance.PipelineLayout);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
			return false;
		}

		// Depth stencil testing
		// TODO: Set up depth stencil testing

		// Graphics pipeline creation
		const uint32_t GraphicsPipelineCreateInfosCount = 1;
		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
		GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.stageCount = ShaderStagesCount;					// Number of shader stages
		GraphicsPipelineCreateInfo.pStages = ShaderStages;							// List of shader stages
		GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputCreateInfo;		// All the fixed function pipeline states
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;
		GraphicsPipelineCreateInfo.pViewportState = &ViewportStateCreateInfo;
		GraphicsPipelineCreateInfo.pDynamicState = nullptr;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
		GraphicsPipelineCreateInfo.pMultisampleState = &MultisampleStateCreateInfo;
		GraphicsPipelineCreateInfo.pColorBlendState = &ColourBlendingCreateInfo;
		GraphicsPipelineCreateInfo.pDepthStencilState = nullptr;
		GraphicsPipelineCreateInfo.layout = RenderInstance.PipelineLayout;							// Pipeline Layout pipeline should use
		GraphicsPipelineCreateInfo.renderPass = RenderInstance.RenderPass;							// Render pass description the pipeline is compatible with
		GraphicsPipelineCreateInfo.subpass = 0;										// Subpass of render pass to use with pipeline

		// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)


		Result = vkCreateGraphicsPipelines(RenderInstance.LogicalDevice, VK_NULL_HANDLE, GraphicsPipelineCreateInfosCount, &GraphicsPipelineCreateInfo, nullptr, &RenderInstance.GraphicsPipeline);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateGraphicsPipelines result is {}", static_cast<int>(Result));
			return false;
		}

		vkDestroyShaderModule(RenderInstance.LogicalDevice, FragmentShaderModule, nullptr);
		vkDestroyShaderModule(RenderInstance.LogicalDevice, VertexShaderModule, nullptr);
		// Function end: CreateGraphicsPipeline

		RenderInstance.SwapchainFramebuffers = static_cast<VkFramebuffer*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkFramebuffer)));

		// Function CreateFrameBuffers
		// Create a framebuffer for each swap chain image
		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			const uint32_t AttachmentsCount = 1;
			VkImageView Attachments[AttachmentsCount] = {
				RenderInstance.ImageViews[i]
			};

			VkFramebufferCreateInfo FramebufferCreateInfo = {};
			FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			FramebufferCreateInfo.renderPass = RenderInstance.RenderPass;								// Render Pass layout the Framebuffer will be used with
			FramebufferCreateInfo.attachmentCount = AttachmentsCount;
			FramebufferCreateInfo.pAttachments = Attachments;							// List of attachments (1:1 with Render Pass)
			FramebufferCreateInfo.width = RenderInstance.SwapExtent.width;								// Framebuffer width
			FramebufferCreateInfo.height = RenderInstance.SwapExtent.height;							// Framebuffer height
			FramebufferCreateInfo.layers = 1;											// Framebuffer layers

			Result = vkCreateFramebuffer(RenderInstance.LogicalDevice, &FramebufferCreateInfo, nullptr, &RenderInstance.SwapchainFramebuffers[i]);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkCreateFramebuffer result is {}", static_cast<int>(Result));
				return false;
			}
		}
		// Function end CreateFrameBuffers

		// Function CreateCommandPool
		VkCommandPoolCreateInfo PoolInfo = {};
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.queueFamilyIndex = PhysicalDeviceIndices.GraphicsFamily;	// Queue Family type that buffers from this command pool will use

		// Create a Graphics Queue Family Command Pool
		Result = vkCreateCommandPool(RenderInstance.LogicalDevice, &PoolInfo, nullptr, &RenderInstance.GraphicsCommandPool);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateCommandPool result is {}", static_cast<int>(Result));
			return false;
		}
		// Function end CreateCommandPool

		// Function CreateCommandBuffers
		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
		CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CommandBufferAllocateInfo.commandPool = RenderInstance.GraphicsCommandPool;
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		CommandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(RenderInstance.SwapchainImagesCount);

		// Allocate command buffers and place handles in array of buffers
		RenderInstance.CommandBuffers = static_cast<VkCommandBuffer*>(Util::Memory::Allocate(RenderInstance.SwapchainImagesCount * sizeof(VkCommandBuffer)));
		Result = vkAllocateCommandBuffers(RenderInstance.LogicalDevice, &CommandBufferAllocateInfo, RenderInstance.CommandBuffers);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateCommandBuffers result is {}", static_cast<int>(Result));
			return false;
		}
		// Function end CreateCommandBuffers

		RenderInstance.MaxFrameDraws = 2;
		RenderInstance.CurrentFrame = 0;

		// Function CreateSynchronisation
		RenderInstance.ImageAvalible = static_cast<VkSemaphore*>(Util::Memory::Allocate(RenderInstance.MaxFrameDraws * sizeof(VkSemaphore)));
		RenderInstance.RenderFinished = static_cast<VkSemaphore*>(Util::Memory::Allocate(RenderInstance.MaxFrameDraws * sizeof(VkSemaphore)));
		RenderInstance.DrawFences = static_cast<VkFence*>(Util::Memory::Allocate(RenderInstance.MaxFrameDraws * sizeof(VkFence)));

		VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
		SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// Fence creation information
		VkFenceCreateInfo FenceCreateInfo = {};
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < RenderInstance.MaxFrameDraws; i++)
		{
			if (vkCreateSemaphore(RenderInstance.LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderInstance.ImageAvalible[i]) != VK_SUCCESS ||
				vkCreateSemaphore(RenderInstance.LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderInstance.RenderFinished[i]) != VK_SUCCESS ||
				vkCreateFence(RenderInstance.LogicalDevice, &FenceCreateInfo, nullptr, &RenderInstance.DrawFences[i]) != VK_SUCCESS)
			{
				Util::Log().Error("CreateSynchronisation error");
				return false;
			}
		}
		// Function end CreateSynchronisation

		return true;
	}

	bool LoadMesh(VulkanRenderInstance& RenderInstance, Mesh Mesh)
	{
		const VkDeviceSize VerticesBufferSize = sizeof(Vertex) * Mesh.MeshVerticesCount;
		const VkDeviceSize IndecesBufferSize = sizeof(uint32_t) * Mesh.MeshIndicesCount;
		const VkDeviceSize StagingBufferSize = VerticesBufferSize > IndecesBufferSize ? VerticesBufferSize : IndecesBufferSize;
		void* data = nullptr;

		VulkanBuffer StagingBuffer;
		CreateVulkanBuffer(RenderInstance, StagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer);

		// Vertex buffer
		vkMapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory, 0, VerticesBufferSize, 0, &data);
		memcpy(data, Mesh.MeshVertices, (size_t)(VerticesBufferSize));
		vkUnmapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory);

		CreateVulkanBuffer(RenderInstance, VerticesBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			RenderInstance.DrawableObject.VertexBuffer);

		CopyBuffer(RenderInstance, StagingBuffer.Buffer, RenderInstance.DrawableObject.VertexBuffer.Buffer, VerticesBufferSize);

		RenderInstance.DrawableObject.VerticesCount = Mesh.MeshVerticesCount;
		
		// Index buffer
		vkMapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory, 0, IndecesBufferSize, 0, &data);
		memcpy(data, Mesh.MeshIndices, (size_t)(IndecesBufferSize));
		vkUnmapMemory(RenderInstance.LogicalDevice, StagingBuffer.BufferMemory);

		CreateVulkanBuffer(RenderInstance, IndecesBufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			RenderInstance.DrawableObject.IndexBuffer);

		CopyBuffer(RenderInstance, StagingBuffer.Buffer, RenderInstance.DrawableObject.IndexBuffer.Buffer, IndecesBufferSize);

		RenderInstance.DrawableObject.IndicesCount = Mesh.MeshIndicesCount;

		DestroyVulkanBuffer(RenderInstance, StagingBuffer);

		return true;
	}

	bool RecordCommands(VulkanRenderInstance& RenderInstance)
	{
		VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkRenderPassBeginInfo RenderPassBeginInfo = {};
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.renderPass = RenderInstance.RenderPass;							// Render Pass to begin
		RenderPassBeginInfo.renderArea.offset = { 0, 0 };						// Start point of render pass in pixels
		RenderPassBeginInfo.renderArea.extent = RenderInstance.SwapExtent;				// Size of region to run render pass on (starting at offset)

		const uint32_t ClearValuesSize = 1;
		VkClearValue ClearValues[ClearValuesSize] = {
			{0.6f, 0.65f, 0.4f, 1.0f}
		};
		RenderPassBeginInfo.pClearValues = ClearValues;
		RenderPassBeginInfo.clearValueCount = ClearValuesSize; // List of clear values (TODO: Depth Attachment Clear Value)

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			RenderPassBeginInfo.framebuffer = RenderInstance.SwapchainFramebuffers[i];

			VkResult Result = vkBeginCommandBuffer(RenderInstance.CommandBuffers[i], &CommandBufferBeginInfo);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
				return false;
			}

			// Begin Render Pass
			vkCmdBeginRenderPass(RenderInstance.CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Bind Pipeline to be used in render pass
			vkCmdBindPipeline(RenderInstance.CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, RenderInstance.GraphicsPipeline);

			// TODO: Support rework to not create identical index buffers
			VkBuffer VertexBuffers[] = { RenderInstance.DrawableObject.VertexBuffer.Buffer };					// Buffers to bind
			VkDeviceSize Offsets[] = { 0 };												// Offsets into buffers being bound
			vkCmdBindVertexBuffers(RenderInstance.CommandBuffers[i], 0, 1, VertexBuffers, Offsets);

			vkCmdBindIndexBuffer(RenderInstance.CommandBuffers[i], RenderInstance.DrawableObject.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

			// Execute pipeline
			//vkCmdDraw(RenderInstance.CommandBuffers[i], RenderInstance.DrawableObject.VerticesCount, 1, 0, 0);
			vkCmdDrawIndexed(RenderInstance.CommandBuffers[i], RenderInstance.DrawableObject.IndicesCount, 1, 0, 0, 0);

			// End Render Pass
			vkCmdEndRenderPass(RenderInstance.CommandBuffers[i]);

			Result = vkEndCommandBuffer(RenderInstance.CommandBuffers[i]);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
				return false;
			}
		}

		return true;
	}

	bool Draw(VulkanRenderInstance& RenderInstance)
	{
		vkWaitForFences(RenderInstance.LogicalDevice, 1, &RenderInstance.DrawFences[RenderInstance.CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(RenderInstance.LogicalDevice, 1, &RenderInstance.DrawFences[RenderInstance.CurrentFrame]);

		// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
		uint32_t ImageIndex;
		vkAcquireNextImageKHR(RenderInstance.LogicalDevice, RenderInstance.VulkanSwapchain, std::numeric_limits<uint64_t>::max(), RenderInstance.ImageAvalible[RenderInstance.CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

		// -- SUBMIT COMMAND BUFFER TO RENDER --
		// Queue submission information
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		submitInfo.pWaitSemaphores = &RenderInstance.ImageAvalible[RenderInstance.CurrentFrame];				// List of semaphores to wait on
		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		submitInfo.pWaitDstStageMask = waitStages;						// Stages to check semaphores at
		submitInfo.commandBufferCount = 1;								// Number of command buffers to submit
		submitInfo.pCommandBuffers = &RenderInstance.CommandBuffers[ImageIndex];		// Command buffer to submit
		submitInfo.signalSemaphoreCount = 1;							// Number of semaphores to signal
		submitInfo.pSignalSemaphores = &RenderInstance.RenderFinished[RenderInstance.CurrentFrame];	// Semaphores to signal when command buffer finishes

		// Submit command buffer to queue
		VkResult Result = vkQueueSubmit(RenderInstance.GraphicsQueue, 1, &submitInfo, RenderInstance.DrawFences[RenderInstance.CurrentFrame]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueueSubmit result is {}", static_cast<int>(Result));
			return false;
		}

		// -- PRESENT RENDERED IMAGE TO SCREEN --
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		presentInfo.pWaitSemaphores = &RenderInstance.RenderFinished[RenderInstance.CurrentFrame];			// Semaphores to wait on
		presentInfo.swapchainCount = 1;											// Number of swapchains to present to
		presentInfo.pSwapchains = &RenderInstance.VulkanSwapchain;									// Swapchains to present images to
		presentInfo.pImageIndices = &ImageIndex;								// Index of images in swapchains to present

		// Present image
		Result = vkQueuePresentKHR(RenderInstance.PresentationQueue, &presentInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueuePresentKHR result is {}", static_cast<int>(Result));
			return false;
		}

		RenderInstance.CurrentFrame = (RenderInstance.CurrentFrame + 1) % RenderInstance.MaxFrameDraws;

		return true;
	}

	void DeinitVulkanRenderInstance(VulkanRenderInstance& RenderInstance)
	{
		vkDeviceWaitIdle(RenderInstance.LogicalDevice);

		DestroyVulkanBuffer(RenderInstance, RenderInstance.DrawableObject.VertexBuffer);
		DestroyVulkanBuffer(RenderInstance, RenderInstance.DrawableObject.IndexBuffer);

		for (size_t i = 0; i < RenderInstance.MaxFrameDraws; i++)
		{
			vkDestroySemaphore(RenderInstance.LogicalDevice, RenderInstance.RenderFinished[i], nullptr);
			vkDestroySemaphore(RenderInstance.LogicalDevice, RenderInstance.ImageAvalible[i], nullptr);
			vkDestroyFence(RenderInstance.LogicalDevice, RenderInstance.DrawFences[i], nullptr);
		}

		Util::Memory::Deallocate(RenderInstance.DrawFences);
		Util::Memory::Deallocate(RenderInstance.RenderFinished);
		Util::Memory::Deallocate(RenderInstance.ImageAvalible);

		Util::Memory::Deallocate(RenderInstance.CommandBuffers);

		vkDestroyCommandPool(RenderInstance.LogicalDevice, RenderInstance.GraphicsCommandPool, nullptr);

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; i++)
		{
			vkDestroyFramebuffer(RenderInstance.LogicalDevice, RenderInstance.SwapchainFramebuffers[i], nullptr);
		}

		Util::Memory::Deallocate(RenderInstance.SwapchainFramebuffers);

		vkDestroyPipeline(RenderInstance.LogicalDevice, RenderInstance.GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(RenderInstance.LogicalDevice, RenderInstance.PipelineLayout, nullptr);
		vkDestroyRenderPass(RenderInstance.LogicalDevice, RenderInstance.RenderPass, nullptr);

		for (uint32_t i = 0; i < RenderInstance.SwapchainImagesCount; ++i)
		{
			vkDestroyImageView(RenderInstance.LogicalDevice, RenderInstance.ImageViews[i], nullptr);
		}

		Util::Memory::Deallocate(RenderInstance.ImageViews);

		vkDestroySwapchainKHR(RenderInstance.LogicalDevice, RenderInstance.VulkanSwapchain, nullptr);
		vkDestroySurfaceKHR(RenderInstance.VulkanInstance, RenderInstance.Surface, nullptr);
		vkDestroyDevice(RenderInstance.LogicalDevice, nullptr);
	}
}
