//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/glm.hpp>
//#include <glm/mat4x4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Util/Util.h"

#include <cassert>
#include <algorithm>

#include "Core/VulkanCoreTypes.h"

int Start()
{
	// INSTANCE CREATION

	const char* ValidationExtensions[] = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};

	const uint32_t ValidationExtensionsSize = Util::EnableValidationLayers ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

	Core::VkInstanceCreateInfoSetupData InstanceCreateInfoData;
	if (!Core::InitVkInstanceCreateInfoSetupData(InstanceCreateInfoData, ValidationExtensions, ValidationExtensionsSize, Util::EnableValidationLayers))
	{
		return -1;
	}

	if (!Core::CheckRequiredInstanceExtensionsSupport(InstanceCreateInfoData))
	{
		Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);
		return -1;
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

	if (Util::EnableValidationLayers)
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

	VkInstance VulkanInstance = nullptr;
	VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &VulkanInstance);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
		Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);

		return -1;
	}

	Core::DeinitVkInstanceCreateInfoSetupData(InstanceCreateInfoData);

	VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	if (Util::EnableValidationLayers)
	{
		Util::CreateDebugUtilsMessengerEXT(VulkanInstance, &MessengerCreateInfo, nullptr, &DebugMessenger);
	}

	// INSTANCE CREATION END

	GLFWwindow* Window = glfwCreateWindow(800, 600, "BMEngine", nullptr, nullptr);
	if (Window == nullptr)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	VkSurfaceKHR Surface = nullptr;
	if (glfwCreateWindowSurface(VulkanInstance, Window, nullptr, &Surface) != VK_SUCCESS)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	// Function: SetupPhysicalDevice
	uint32_t DevicesCount = 0;
	vkEnumeratePhysicalDevices(VulkanInstance, &DevicesCount, nullptr);

	VkPhysicalDevice* DeviceList = static_cast<VkPhysicalDevice*>(Util::Memory::Allocate(DevicesCount * sizeof(VkPhysicalDevice)));
	vkEnumeratePhysicalDevices(VulkanInstance, &DevicesCount, DeviceList);

	Core::VkPhysicalDeviceSetupData VkPhysicalDeviceSetupData;

	Core::PhysicalDeviceIndices PhysicalDeviceIndices;
	VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };

	VkPhysicalDevice PhysicalDevice = nullptr;

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
		
		if (!Core::InitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData, DeviceList[i], Surface))
		{
			break;
		}

		if (!Core::CheckDeviceExtensionsSupport(VkPhysicalDeviceSetupData, DeviceExtensions, DeviceExtensionsSize))
		{
			break;
		}

		PhysicalDeviceIndices = Core::GetPhysicalDeviceIndices(VkPhysicalDeviceSetupData, DeviceList[i], Surface);

		if (PhysicalDeviceIndices.GraphicsFamily < 0 || PhysicalDeviceIndices.PresentationFamily < 0)
		{
			Util::Log().Warning("PhysicalDeviceIndices are not initialized");
			break;
		}

		Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DeviceList[i], Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
			break;
		}

		PhysicalDevice = DeviceList[i];
	}

	Util::Memory::Deallocate(DeviceList);

	if (PhysicalDevice == nullptr)
	{
		Core::DeinitVkPhysicalDeviceSetupData(VkPhysicalDeviceSetupData);
		Util::Log().Error("No physical devices found");
		return -1;
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
	VkDevice LogicalDevice = nullptr;
	Result = vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
		return -1;
	}
	// Function end: CreateLogicalDevice

	// Mesh
	const uint32_t MeshVerticesCount = 3;
	Core::Vertex MeshVertices[MeshVerticesCount] = {
		{{0.4, -0.4, 0.0}, {1.0f, 0.0f, 0.0f}},
		{{0.4, 0.4, 0.0}, {0.0f, 1.0f, 0.0f}},
		{{-0.4, 0.4, 0.0}, {0.0f, 0.0f, 1.0f}}
	};

	// CREATE VERTEX BUFFER
	// Information to create a buffer (doesn't include assigning memory)
	VkBufferCreateInfo BufferCreateInfo = {};
	BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.size = sizeof(Core::Vertex) * MeshVerticesCount;		// Size of buffer (size of 1 vertex * number of vertices)
	BufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;		// Multiple types of buffer possible, we want Vertex Buffer
	BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Similar to Swap Chain images, can share vertex buffers

	VkBuffer VertexBuffer = nullptr;
	Result = vkCreateBuffer(LogicalDevice, &BufferCreateInfo, nullptr, &VertexBuffer);
	if (Result != VK_SUCCESS)
	{
		return -1;
	}

	// GET BUFFER MEMORY REQUIREMENTS
	VkMemoryRequirements MemoryRequirements;
	vkGetBufferMemoryRequirements(LogicalDevice, VertexBuffer, &MemoryRequirements);

	// Function FindMemoryTypeIndex
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

	const VkMemoryPropertyFlags MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT	: CPU can interact with memory
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	: Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)

	uint32_t MemoryTypeIndex = 0;
	for (; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++)
	{
		if ((MemoryRequirements.memoryTypeBits & (1 << MemoryTypeIndex))														// Index of memory type must match corresponding bit in allowedTypes
			&& (MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & MemoryPropertyFlags) == MemoryPropertyFlags)	// Desired property bit flags are part of memory type's property flags
		{
			// This memory type is valid, so return its index
			break;
		}
	}
	// Function end FindMemoryTypeIndex

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = MemoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;		// Index of memory type on Physical Device that has required bit flags			
	
	VkDeviceMemory VertexBufferMemory = nullptr;
	Result = vkAllocateMemory(LogicalDevice, &memoryAllocInfo, nullptr, &VertexBufferMemory);
	if (Result != VK_SUCCESS)
	{
		return -1;
	}

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(LogicalDevice, VertexBuffer, VertexBufferMemory, 0);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;																// 1. Create pointer to a point in normal memory
	vkMapMemory(LogicalDevice, VertexBufferMemory, 0, BufferCreateInfo.size, 0, &data);		// 2. "Map" the vertex buffer memory to that point
	memcpy(data, MeshVertices, (size_t)(BufferCreateInfo.size));					// 3. Copy memory from vertices vector to the point
	vkUnmapMemory(LogicalDevice, VertexBufferMemory);									// 4. Unmap the vertex buffer memory
	// Mesh

	// Function: SetupQueues
	VkQueue GraphicsQueue = nullptr;
	VkQueue PresentationQueue = nullptr;

	vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(PhysicalDeviceIndices.GraphicsFamily), 0, &GraphicsQueue);
	vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(PhysicalDeviceIndices.PresentationFamily), 0, &PresentationQueue);

	if (GraphicsQueue == nullptr && PresentationQueue == nullptr)
	{
		return -1;
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
		return -1;
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
	VkExtent2D SwapExtent;
	if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		SwapExtent = SurfaceCapabilities.currentExtent;
	}
	else
	{
		int Width;
		int Height;
		glfwGetFramebufferSize(Window, &Width, &Height);

		Width = std::clamp(static_cast<uint32_t>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
		Height = std::clamp(static_cast<uint32_t>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

		SwapExtent = { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
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

	VkSwapchainKHR VulkanSwapchain = nullptr;
	Result = vkCreateSwapchainKHR(LogicalDevice, &SwapchainCreateInfo, nullptr, &VulkanSwapchain);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateSwapchainKHR result is {}", static_cast<int>(Result));
		return -1;
	}

	// Get swap chain images
	uint32_t SwapchainImagesCount = 0;
	vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &SwapchainImagesCount, nullptr);

	VkImage* Images = static_cast<VkImage*>(Util::Memory::Allocate(SwapchainImagesCount * sizeof(VkImage)));
	vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &SwapchainImagesCount, Images);

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

	VkImageView* ImageViews = static_cast<VkImageView*>(Util::Memory::Allocate(SwapchainImagesCount * sizeof(VkImageView)));

	for (uint32_t i = 0; i < SwapchainImagesCount; ++i)
	{
		ViewCreateInfo.image = Images[i];
		
		VkImageView ImageView;
		Result = vkCreateImageView(LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateImageView result is {}", static_cast<int>(Result));
			Util::Memory::Deallocate(Images);
			Util::Memory::Deallocate(ImageViews);
			return -1;
		}
		
		ImageViews[i] = ImageView;
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

	VkRenderPass RenderPass = nullptr;
	Result = vkCreateRenderPass(LogicalDevice, &renderPassCreateInfo, nullptr, &RenderPass);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateRenderPass result is {}", static_cast<int>(Result));
		return -1;
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
	Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
		return -1;
	}

	ShaderModuleCreateInfo.codeSize = FragmentShaderCode.size();
	ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(FragmentShaderCode.data());

	VkShaderModule FragmentShaderModule = nullptr;
	Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &FragmentShaderModule);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
		return -1;
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
	Viewport.width = static_cast<float>(SwapExtent.width);
	Viewport.height = static_cast<float>(SwapExtent.height);
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;

	VkRect2D Scissor = {};
	Scissor.offset = { 0, 0 };
	Scissor.extent = SwapExtent;

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

	VkPipelineLayout PipelineLayout = nullptr;
	Result = vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
		return -1;
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
	GraphicsPipelineCreateInfo.layout = PipelineLayout;							// Pipeline Layout pipeline should use
	GraphicsPipelineCreateInfo.renderPass = RenderPass;							// Render pass description the pipeline is compatible with
	GraphicsPipelineCreateInfo.subpass = 0;										// Subpass of render pass to use with pipeline

	// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
	GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
	GraphicsPipelineCreateInfo.basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)


	VkPipeline GraphicsPipeline = nullptr;
	Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, GraphicsPipelineCreateInfosCount, &GraphicsPipelineCreateInfo, nullptr, &GraphicsPipeline);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateGraphicsPipelines result is {}", static_cast<int>(Result));
		return -1;
	}

	vkDestroyShaderModule(LogicalDevice, FragmentShaderModule, nullptr);
	vkDestroyShaderModule(LogicalDevice, VertexShaderModule, nullptr);
	// Function end: CreateGraphicsPipeline

	VkFramebuffer* SwapchainFramebuffers = static_cast<VkFramebuffer*>(Util::Memory::Allocate(SwapchainImagesCount * sizeof(VkFramebuffer)));

	// Function CreateFrameBuffers
	// Create a framebuffer for each swap chain image
	for (uint32_t i = 0; i < SwapchainImagesCount; i++)
	{
		const uint32_t AttachmentsCount = 1;
		VkImageView Attachments[AttachmentsCount] = {
			ImageViews[i]
		};

		VkFramebufferCreateInfo FramebufferCreateInfo = {};
		FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.renderPass = RenderPass;								// Render Pass layout the Framebuffer will be used with
		FramebufferCreateInfo.attachmentCount = AttachmentsCount;
		FramebufferCreateInfo.pAttachments = Attachments;							// List of attachments (1:1 with Render Pass)
		FramebufferCreateInfo.width = SwapExtent.width;								// Framebuffer width
		FramebufferCreateInfo.height = SwapExtent.height;							// Framebuffer height
		FramebufferCreateInfo.layers = 1;											// Framebuffer layers

		Result = vkCreateFramebuffer(LogicalDevice, &FramebufferCreateInfo, nullptr, &SwapchainFramebuffers[i]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateFramebuffer result is {}", static_cast<int>(Result));
			return -1;
		}
	}
	// Function end CreateFrameBuffers

	// Function CreateCommandPool
	VkCommandPoolCreateInfo PoolInfo = {};
	PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	PoolInfo.queueFamilyIndex = PhysicalDeviceIndices.GraphicsFamily;	// Queue Family type that buffers from this command pool will use

	VkCommandPool GraphicsCommandPool = nullptr;
	// Create a Graphics Queue Family Command Pool
	Result = vkCreateCommandPool(LogicalDevice, &PoolInfo, nullptr, &GraphicsCommandPool);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkCreateCommandPool result is {}", static_cast<int>(Result));
		return -1;
	}
	// Function end CreateCommandPool

	// Function CreateCommandBuffers
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
	CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	CommandBufferAllocateInfo.commandPool = GraphicsCommandPool;
	CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
	// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
	CommandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(SwapchainImagesCount);

	// Allocate command buffers and place handles in array of buffers
	VkCommandBuffer* CommandBuffers = static_cast<VkCommandBuffer*>(Util::Memory::Allocate(SwapchainImagesCount * sizeof(VkCommandBuffer)));
	Result = vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, CommandBuffers);
	if (Result != VK_SUCCESS)
	{
		Util::Log().Error("vkAllocateCommandBuffers result is {}", static_cast<int>(Result));
		return -1;
	}
	// Function end CreateCommandBuffers

	// Function RecordCommands
	// Information about how to begin each command buffer
	VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
	CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	
	// Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo RenderPassBeginInfo = {};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	RenderPassBeginInfo.renderPass = RenderPass;							// Render Pass to begin
	RenderPassBeginInfo.renderArea.offset = { 0, 0 };						// Start point of render pass in pixels
	RenderPassBeginInfo.renderArea.extent = SwapExtent;				// Size of region to run render pass on (starting at offset)

	const uint32_t ClearValuesSize = 1;
	VkClearValue ClearValues[ClearValuesSize] = {
		{0.6f, 0.65f, 0.4f, 1.0f}
	};
	RenderPassBeginInfo.pClearValues = ClearValues;							// List of clear values (TODO: Depth Attachment Clear Value)
	RenderPassBeginInfo.clearValueCount = ClearValuesSize;

	for (uint32_t i = 0; i < SwapchainImagesCount; i++)
	{
		RenderPassBeginInfo.framebuffer = SwapchainFramebuffers[i];

		// Start recording commands to command buffer!
		Result = vkBeginCommandBuffer(CommandBuffers[i], &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			return -1;
		}

		// Begin Render Pass
		vkCmdBeginRenderPass(CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind Pipeline to be used in render pass
		vkCmdBindPipeline(CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);


		VkBuffer VertexBuffers[] = { VertexBuffer };					// Buffers to bind
		VkDeviceSize Offsets[] = { 0 };												// Offsets into buffers being bound
		vkCmdBindVertexBuffers(CommandBuffers[i], 0, 1, VertexBuffers, Offsets);	// Command to bind vertex buffer before drawing with them


		// Execute pipeline
		vkCmdDraw(CommandBuffers[i], MeshVerticesCount, 1, 0, 0);

		// End Render Pass
		vkCmdEndRenderPass(CommandBuffers[i]);

		// Stop recording to command buffer
		Result = vkEndCommandBuffer(CommandBuffers[i]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			return -1;
		}
	}
	// Function end RecordCommands

	const int MaxFrameDraws = 2;
	int CurrentFrame = 0;

	// Function CreateSynchronisation
	VkSemaphore* ImageAvalible = static_cast<VkSemaphore*>(Util::Memory::Allocate(MaxFrameDraws * sizeof(VkSemaphore)));
	VkSemaphore* RenderFinished = static_cast<VkSemaphore*>(Util::Memory::Allocate(MaxFrameDraws * sizeof(VkSemaphore)));
	VkFence* DrawFences = static_cast<VkFence*>(Util::Memory::Allocate(MaxFrameDraws * sizeof(VkFence)));

	VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
	SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence creation information
	VkFenceCreateInfo FenceCreateInfo = {};
	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MaxFrameDraws; i++)
	{
		if (vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &ImageAvalible[i]) != VK_SUCCESS ||
			vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(LogicalDevice, &FenceCreateInfo, nullptr, &DrawFences[i]) != VK_SUCCESS)
		{
			Util::Log().Error("CreateSynchronisation error");
			return -1;
		}
	}

	// Function end CreateSynchronisation

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();

		vkWaitForFences(LogicalDevice, 1, &DrawFences[CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(LogicalDevice, 1, &DrawFences[CurrentFrame]);

		// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
		uint32_t ImageIndex;
		vkAcquireNextImageKHR(LogicalDevice, VulkanSwapchain, std::numeric_limits<uint64_t>::max(), ImageAvalible[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

		// -- SUBMIT COMMAND BUFFER TO RENDER --
		// Queue submission information
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		submitInfo.pWaitSemaphores = &ImageAvalible[CurrentFrame];				// List of semaphores to wait on
		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		submitInfo.pWaitDstStageMask = waitStages;						// Stages to check semaphores at
		submitInfo.commandBufferCount = 1;								// Number of command buffers to submit
		submitInfo.pCommandBuffers = &CommandBuffers[ImageIndex];		// Command buffer to submit
		submitInfo.signalSemaphoreCount = 1;							// Number of semaphores to signal
		submitInfo.pSignalSemaphores = &RenderFinished[CurrentFrame];	// Semaphores to signal when command buffer finishes

		// Submit command buffer to queue
		Result = vkQueueSubmit(GraphicsQueue, 1, &submitInfo, DrawFences[CurrentFrame]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueueSubmit result is {}", static_cast<int>(Result));
			return -1;
		}

		// -- PRESENT RENDERED IMAGE TO SCREEN --
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		presentInfo.pWaitSemaphores = &RenderFinished[CurrentFrame];			// Semaphores to wait on
		presentInfo.swapchainCount = 1;											// Number of swapchains to present to
		presentInfo.pSwapchains = &VulkanSwapchain;									// Swapchains to present images to
		presentInfo.pImageIndices = &ImageIndex;								// Index of images in swapchains to present

		// Present image
		Result = vkQueuePresentKHR(PresentationQueue, &presentInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueuePresentKHR result is {}", static_cast<int>(Result));
			return -1;
		}

		CurrentFrame = (CurrentFrame + 1) % MaxFrameDraws;
	}

	vkDeviceWaitIdle(LogicalDevice);

	vkDestroyBuffer(LogicalDevice, VertexBuffer, nullptr);
	vkFreeMemory(LogicalDevice, VertexBufferMemory, nullptr);

	for (size_t i = 0; i < MaxFrameDraws; i++)
	{
		vkDestroySemaphore(LogicalDevice, RenderFinished[i], nullptr);
		vkDestroySemaphore(LogicalDevice, ImageAvalible[i], nullptr);
		vkDestroyFence(LogicalDevice, DrawFences[i], nullptr);
	}

	Util::Memory::Deallocate(DrawFences);
	Util::Memory::Deallocate(RenderFinished);
	Util::Memory::Deallocate(ImageAvalible);

	Util::Memory::Deallocate(CommandBuffers);

	vkDestroyCommandPool(LogicalDevice, GraphicsCommandPool, nullptr);

	for (uint32_t i = 0; i < SwapchainImagesCount; i++)
	{
		vkDestroyFramebuffer(LogicalDevice, SwapchainFramebuffers[i], nullptr);
	}

	Util::Memory::Deallocate(SwapchainFramebuffers);

	vkDestroyPipeline(LogicalDevice, GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
	vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);

	for (uint32_t i = 0; i < SwapchainImagesCount; ++i)
	{
		vkDestroyImageView(LogicalDevice, ImageViews[i], nullptr);
	}

	Util::Memory::Deallocate(ImageViews);

	vkDestroySwapchainKHR(LogicalDevice, VulkanSwapchain, nullptr);
	vkDestroySurfaceKHR(VulkanInstance, Surface, nullptr);
	vkDestroyDevice(LogicalDevice, nullptr);

	if (Util::EnableValidationLayers)
	{
		Util::DestroyDebugMessenger(VulkanInstance, DebugMessenger, nullptr);
	}

	vkDestroyInstance(VulkanInstance, nullptr);

	glfwDestroyWindow(Window);

	return 0;
}

int main()
{
	if (glfwInit() == GL_FALSE)
	{
		Util::Log().GlfwLogError();
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	int Result = Start();

	glfwTerminate();

	if (Util::Memory::AllocateCounter != 0)
	{
		Util::Log().Error("AllocateCounter in not equal 0, counter is {}", Util::Memory::AllocateCounter);
		assert(false);
	}

	return Result;
}