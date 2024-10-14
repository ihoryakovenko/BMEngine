#include "VulkanCoreTypes.h"

#include "VulkanHelper.h"
#include "Util/Util.h"

namespace Core
{
	GPUBuffer GPUBuffer::CreateIndexBuffer(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
		VkCommandPool TransferCommandPool, VkQueue TransferQueue, u32* Indices, u32 IndicesCount)
	{
		const VkDeviceSize BufferSize = sizeof(u32) * IndicesCount;

		GPUBuffer StagingBuffer = CreateGPUBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* Data;
		vkMapMemory(LogicalDevice, StagingBuffer.Memory, 0, BufferSize, 0, &Data);
		memcpy(Data, Indices, (u64)BufferSize);
		vkUnmapMemory(LogicalDevice, StagingBuffer.Memory);

		// Create buffer for INDEX data on GPU access only area
		GPUBuffer indexBuffer = CreateGPUBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy from staging buffer to GPU access buffer
		GPUBuffer::CopyGPUBuffer(LogicalDevice, TransferCommandPool, TransferQueue, StagingBuffer, indexBuffer);
		GPUBuffer::DestroyGPUBuffer(LogicalDevice, StagingBuffer);

		return indexBuffer;
	}

	GPUBuffer GPUBuffer::CreateIndirectBuffer(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
		VkCommandPool TransferCommandPool, VkQueue TransferQueue, const std::vector<VkDrawIndexedIndirectCommand>& DrawCommands)
	{
		const VkDeviceSize BufferSize = sizeof(VkDrawIndexedIndirectCommand) * DrawCommands.size();

		GPUBuffer StagingBuffer = CreateGPUBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* Data;
		vkMapMemory(LogicalDevice, StagingBuffer.Memory, 0, BufferSize, 0, &Data);
		memcpy(Data, DrawCommands.data(), (u64)BufferSize);
		vkUnmapMemory(LogicalDevice, StagingBuffer.Memory);

		GPUBuffer indirectBuffer = CreateGPUBuffer(PhysicalDevice, LogicalDevice, BufferSize, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // TODO: VK_MEMORY_PROPERTY_HOST_COHERENT_BIT - mistake?

		// Copy from staging buffer to GPU access buffer
		GPUBuffer::CopyGPUBuffer(LogicalDevice, TransferCommandPool, TransferQueue, StagingBuffer, indirectBuffer);
		GPUBuffer::DestroyGPUBuffer(LogicalDevice, StagingBuffer);

		return indirectBuffer;
	}

	GPUBuffer GPUBuffer::CreateGPUBuffer(VkPhysicalDevice PhysicalDevice,  VkDevice LogicalDevice, VkDeviceSize BufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties)
	{
		assert(BufferSize > 0);

		GPUBuffer Buffer;
		Buffer.Size = BufferSize;

		VkBufferCreateInfo bufferInfo = { };
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = Buffer.Size;
		bufferInfo.usage = bufferUsage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkResult Result = vkCreateBuffer(LogicalDevice, &bufferInfo, nullptr, &Buffer.Buffer);
		if (Result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Vertex Buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(LogicalDevice, Buffer.Buffer, &memRequirements);

		VkMemoryAllocateInfo memoryAllocInfo = { };
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memRequirements.size;
		memoryAllocInfo.memoryTypeIndex = GetMemoryTypeIndex(PhysicalDevice, memRequirements.memoryTypeBits,		// Index of memory type on Physical Device that has required bit flags
			bufferProperties);																						// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT	: CPU can interact with memory
		// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	: Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
		Result = vkAllocateMemory(LogicalDevice, &memoryAllocInfo, nullptr, &Buffer.Memory);
		if (Result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
		}

		vkBindBufferMemory(LogicalDevice, Buffer.Buffer, Buffer.Memory, 0);

		return Buffer;
	}

	void GPUBuffer::CopyGPUBuffer(VkDevice LogicalDevice, VkCommandPool TransferCommandPool, VkQueue TransferQueue,
		const GPUBuffer& srcBuffer, GPUBuffer& dstBuffer)
	{
		assert(srcBuffer.Size <= dstBuffer.Size);

		VkCommandBuffer transferCommandBuffer;

		VkCommandBufferAllocateInfo allocInfo = { };
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = TransferCommandPool;
		allocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(LogicalDevice, &allocInfo, &transferCommandBuffer);

		VkCommandBufferBeginInfo beginInfo = { };
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// We're only using the command buffer once, so set up for one time submit
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

		VkBufferCopy bufferCopyRegion = { };
		bufferCopyRegion.srcOffset = 0;
		bufferCopyRegion.dstOffset = 0;
		bufferCopyRegion.size = srcBuffer.Size;

		vkCmdCopyBuffer(transferCommandBuffer, srcBuffer.Buffer, dstBuffer.Buffer, 1, &bufferCopyRegion);
		vkEndCommandBuffer(transferCommandBuffer);

		VkSubmitInfo submitInfo = { };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &transferCommandBuffer;

		vkQueueSubmit(TransferQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(TransferQueue);

		vkFreeCommandBuffers(LogicalDevice, TransferCommandPool, 1, &transferCommandBuffer);
	}

	void GPUBuffer::DestroyGPUBuffer(VkDevice LogicalDevice, GPUBuffer& buffer)
	{
		vkDestroyBuffer(LogicalDevice, buffer.Buffer, nullptr);
		vkFreeMemory(LogicalDevice, buffer.Memory, nullptr);
	}

	MainInstance MainInstance::CreateMainInstance(const char** RequiredExtensions, u32 RequiredExtensionsCount,
		bool IsValidationLayersEnabled, const char* ValidationLayers[], u32 ValidationLayersSize)
	{
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
		CreateInfo.enabledExtensionCount = RequiredExtensionsCount;
		CreateInfo.ppEnabledExtensionNames = RequiredExtensions;

		if (IsValidationLayersEnabled)
		{
			MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			MessengerCreateInfo.pfnUserCallback = Util::MessengerDebugCallback;
			MessengerCreateInfo.pUserData = nullptr;

			CreateInfo.pNext = &MessengerCreateInfo;
			CreateInfo.enabledLayerCount = ValidationLayersSize;
			CreateInfo.ppEnabledLayerNames = ValidationLayers;
		}
		else
		{
			CreateInfo.ppEnabledLayerNames = nullptr;
			CreateInfo.enabledLayerCount = 0;
			CreateInfo.pNext = nullptr;
		}

		MainInstance Instance;
		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
			assert(false);
		}

		if (IsValidationLayersEnabled)
		{
			const bool CreateDebugUtilsMessengerResult =
				Util::CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);
			assert(CreateDebugUtilsMessengerResult);
		}

		return Instance;
	}

	void MainInstance::DestroyMainInstance(MainInstance& Instance)
	{
		if (Instance.DebugMessenger != nullptr)
		{
			Util::DestroyDebugMessenger(Instance.VulkanInstance, Instance.DebugMessenger, nullptr);
		}

		vkDestroyInstance(Instance.VulkanInstance, nullptr);
	}

	void DeviceInstance::Init(VkInstance VulkanInstance, VkSurfaceKHR Surface, const char** DeviceExtensions,
		u32 DeviceExtensionsSize)
	{
		Memory::FrameArray<VkPhysicalDevice> DeviceList = GetPhysicalDeviceList(VulkanInstance);
		
		bool IsDeviceFound = false;
		for (u32 i = 0; i < DeviceList.Count; ++i)
		{
			PhysicalDevice = DeviceList[i];

			Memory::FrameArray<VkExtensionProperties> DeviceExtensionsData = GetDeviceExtensionProperties(PhysicalDevice);
			Memory::FrameArray<VkQueueFamilyProperties> FamilyPropertiesData = GetQueueFamilyProperties(PhysicalDevice);

			Indices = GetPhysicalDeviceIndices(FamilyPropertiesData.Pointer.Data, FamilyPropertiesData.Count, PhysicalDevice, Surface);
			vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
			vkGetPhysicalDeviceFeatures(PhysicalDevice, &AvailableFeatures);

			IsDeviceFound = CheckDeviceSuitability(DeviceExtensions, DeviceExtensionsSize,
				DeviceExtensionsData.Pointer.Data, DeviceExtensionsData.Count, Indices, AvailableFeatures);

			if (IsDeviceFound)
			{
				IsDeviceFound = true;
				break;
			}
		}

		assert(IsDeviceFound);
	}

	Memory::FrameArray<VkPhysicalDevice> DeviceInstance::GetPhysicalDeviceList(VkInstance VulkanInstance)
	{
		u32 Count;
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, nullptr);

		auto Data = Memory::FrameArray<VkPhysicalDevice>::Create(Count);
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkExtensionProperties> DeviceInstance::GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice)
	{
		u32 Count;
		const VkResult Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateDeviceExtensionProperties result is {}", static_cast<int>(Result));
			assert(false);
		}

		auto Data = Memory::FrameArray<VkExtensionProperties>::Create(Count);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkQueueFamilyProperties> DeviceInstance::GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice)
	{
		u32 Count;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Count, nullptr);

		auto Data = Memory::FrameArray<VkQueueFamilyProperties>::Create(Count);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Count, Data.Pointer.Data);

		return Data;
	}

	// TODO check function
	// In current realization if GraphicsFamily is valid but if PresentationFamily is not valid
	// GraphicsFamily could be overridden on next iteration even when it is valid
	PhysicalDeviceIndices DeviceInstance::GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
		VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		PhysicalDeviceIndices Indices;

		for (u32 i = 0; i < PropertiesCount; ++i)
		{
			// check if Queue is graphics type
			if (Properties[i].queueCount > 0 && Properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				// if we QueueFamily[i] graphics type but not presentation type
				// and QueueFamily[i + 1] graphics type and presentation type
				// then we rewrite GraphicsFamily
				// toto check what is better rewrite or have different QueueFamilies
				Indices.GraphicsFamily = i;
			}

			// check if Queue is presentation type (can be graphics and presentation)
			VkBool32 PresentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &PresentationSupport);
			if (Properties[i].queueCount > 0 && PresentationSupport)
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

	bool DeviceInstance::CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, PhysicalDeviceIndices Indices,
		VkPhysicalDeviceFeatures AvailableFeatures)
	{
		if (!CheckDeviceExtensionsSupport(ExtensionProperties, ExtensionPropertiesCount, DeviceExtensions, DeviceExtensionsSize))
		{
			Util::Log().Warning("PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (Indices.GraphicsFamily < 0 || Indices.PresentationFamily < 0)
		{
			Util::Log().Warning("PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (!AvailableFeatures.samplerAnisotropy)
		{
			Util::Log().Warning("Feature samplerAnisotropy is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			Util::Log().Warning("Feature samplerAnisotropy is not supported");
			return false;
		}

		return true;
	}

	bool DeviceInstance::CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
		const char** ExtensionsToCheck, u32 ExtensionsToCheckSize)
	{
		for (u32 i = 0; i < ExtensionsToCheckSize; ++i)
		{
			bool IsDeviceExtensionSupported = false;
			for (u32 j = 0; j < ExtensionPropertiesCount; ++j)
			{
				if (std::strcmp(ExtensionsToCheck[i], ExtensionProperties[j].extensionName) == 0)
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

	SwapchainInstance SwapchainInstance::CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
		PhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
		VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent)
	{
		SwapchainInstance Instance;

		Instance.SwapExtent = Extent;

		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };
		VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkPresentModeKHR PresentationMode = GetBestPresentationMode(PhysicalDevice, Surface);

		Instance.VulkanSwapchain = CreateSwapchain(LogicalDevice, SurfaceCapabilities, Surface, SurfaceFormat, Instance.SwapExtent,
			PresentationMode, Indices);

		Memory::FrameArray<VkImage> Images = GetSwapchainImages(LogicalDevice, Instance.VulkanSwapchain);

		Instance.ImagesCount = Images.Count;

		for (u32 i = 0; i < Instance.ImagesCount; ++i)
		{
			VkImageView ImageView = CreateImageView(LogicalDevice, Images[i], SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
			if (ImageView == nullptr)
			{
				assert(false);
			}

			Instance.ImageViews[i] = ImageView;
		}

		return Instance;
	}

	void SwapchainInstance::DestroySwapchainInstance(VkDevice LogicalDevice, SwapchainInstance& Instance)
	{
		for (u32 i = 0; i < Instance.ImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, Instance.ImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(LogicalDevice, Instance.VulkanSwapchain, nullptr);
	}

	VkSwapchainKHR SwapchainInstance::CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
		VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
		PhysicalDeviceIndices DeviceIndices)
	{
		// How many images are in the swap chain
		// Get 1 more then the minimum to allow triple buffering
		u32 ImageCount = SurfaceCapabilities.minImageCount + 1;

		// If maxImageCount > 0, then limitless
		if (SurfaceCapabilities.maxImageCount > 0
			&& SurfaceCapabilities.maxImageCount < ImageCount)
		{
			ImageCount = SurfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR SwapchainCreateInfo = { };
		SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapchainCreateInfo.surface = Surface;
		SwapchainCreateInfo.imageFormat = SurfaceFormat.format;
		SwapchainCreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
		SwapchainCreateInfo.presentMode = PresentationMode;
		SwapchainCreateInfo.imageExtent = SwapExtent;
		SwapchainCreateInfo.minImageCount = ImageCount;
		SwapchainCreateInfo.imageArrayLayers = 1;
		SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		SwapchainCreateInfo.preTransform = SurfaceCapabilities.currentTransform;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle windows blending
		SwapchainCreateInfo.clipped = VK_TRUE;

		u32 Indices[] = {
			static_cast<u32>(DeviceIndices.GraphicsFamily),
			static_cast<u32>(DeviceIndices.PresentationFamily)
		};

		if (DeviceIndices.GraphicsFamily != DeviceIndices.PresentationFamily)
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

		VkSwapchainKHR Swapchain;
		VkResult Result = vkCreateSwapchainKHR(LogicalDevice, &SwapchainCreateInfo, nullptr, &Swapchain);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateSwapchainKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		return Swapchain;
	}

	VkPresentModeKHR SwapchainInstance::GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		Memory::FrameArray<VkPresentModeKHR> PresentModes = GetAvailablePresentModes(PhysicalDevice, Surface);

		VkPresentModeKHR Mode = VK_PRESENT_MODE_FIFO_KHR;
		for (u32 i = 0; i < PresentModes.Count; ++i)
		{
			if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				Mode = VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		// Has to be present by spec
		if (Mode != VK_PRESENT_MODE_MAILBOX_KHR)
		{
			Util::Log().Warning("Using default VK_PRESENT_MODE_FIFO_KHR");
		}

		return Mode;
	}

	Memory::FrameArray<VkPresentModeKHR> SwapchainInstance::GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface)
	{
		u32 Count;
		const VkResult Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfacePresentModesKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		auto Data = Memory::FrameArray<VkPresentModeKHR>::Create(Count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkImage> SwapchainInstance::GetSwapchainImages(VkDevice LogicalDevice,
		VkSwapchainKHR VulkanSwapchain)
	{
		u32 Count;
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, nullptr);

		auto Data = Memory::FrameArray<VkImage>::Create(Count);
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, Data.Pointer.Data);

		return Data;
	}
}