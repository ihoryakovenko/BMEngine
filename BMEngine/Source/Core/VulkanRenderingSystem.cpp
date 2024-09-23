#include "VulkanRenderingSystem.h"

#include <algorithm>

#include "Util/Util.h"

namespace Core
{
	bool VulkanRenderingSystem::Init(GLFWwindow* Window, GLFWwindow* Window2)
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

		std::vector<VkExtensionProperties> AvailableExtensions;
		GetAvailableExtensionProperties(AvailableExtensions);

		std::vector<const char*> RequiredExtensions;
		GetRequiredInstanceExtensions(ValidationExtensions, ValidationExtensionsSize, RequiredExtensions);

		if (!CheckRequiredInstanceExtensionsSupport(AvailableExtensions.data(), AvailableExtensions.size(),
			RequiredExtensions.data(), RequiredExtensions.size()))
		{
			return false;
		}

		if (Util::EnableValidationLayers)
		{
			std::vector<VkLayerProperties> LayerPropertiesData;
			GetAvailableInstanceLayerProperties(LayerPropertiesData);

			if (!CheckValidationLayersSupport(LayerPropertiesData.data(), LayerPropertiesData.size(),
				ValidationLayers, ValidationLayersSize))
			{
				return false;
			}
		}

		Instance = MainInstance::CreateMainInstance(RequiredExtensions.data(), RequiredExtensions.size(),
			Util::EnableValidationLayers, ValidationLayers, ValidationLayersSize);

		VkSurfaceKHR Surface = nullptr;
		if (glfwCreateWindowSurface(Instance.VulkanInstance, Window, nullptr, &Surface) != VK_SUCCESS)
		{
			Util::Log().GlfwLogError();
			assert(false);
		}

		// TODO: CHeck if device support second surface
		VkSurfaceKHR Surface2 = nullptr;
		if (glfwCreateWindowSurface(Instance.VulkanInstance, Window2, nullptr, &Surface2) != VK_SUCCESS)
		{
			Util::Log().GlfwLogError();
			assert(false);
		}
		
		Device.Init(Instance.VulkanInstance, Surface, DeviceExtensions, DeviceExtensionsSize);

		LogicalDevice = CreateLogicalDevice(Device.Indices, DeviceExtensions, DeviceExtensionsSize);

		SurfaceFormat = GetBestSurfaceFormat(Surface); // TODO: Check for surface 2
		PresentationMode = GetBestPresentationMode(Surface);

		const uint32_t FormatPrioritySize = 3;
		VkFormat FormatPriority[FormatPrioritySize] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };

		bool IsSupportedFormatFound = false;
		for (int i = 0; i < FormatPrioritySize; ++i)
		{
			VkFormat FormatToCheck = FormatPriority[i];
			if (IsFormatSupported(FormatToCheck, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
			{
				IsSupportedFormatFound = true;
				break;
			}

			Util::Log().Warning("Format {} is not supported", static_cast<int>(FormatToCheck));
		}

		assert(IsSupportedFormatFound);


		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };

		VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device.PhysicalDevice, Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
			return false;
		}

		VkExtent2D Extent1 = GetBestSwapExtent(SurfaceCapabilities, Window);

		Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device.PhysicalDevice, Surface2, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
			return false;
		}

		VkExtent2D Extent2 = GetBestSwapExtent(SurfaceCapabilities, Window2);

		VkExtent2D Extents[2];
		Extents[0] = Extent1;
		Extents[1] = Extent2;

		MainPass.Init(LogicalDevice, ColorFormat, DepthFormat,
			SurfaceFormat, Device.Indices.GraphicsFamily, MaxFrameDraws, Extents, 2);

		SetupQueues();
		



		TextureSampler = CreateTextureSampler();
		SamplerDescriptorSets = static_cast<VkDescriptorSet*>(Util::Memory::Allocate(MaxTextures * sizeof(VkDescriptorSet)));
		SamplerDescriptorPool = CreateSamplerDescriptorPool(528); // TODO: Check 528

		InitViewport(Window, Surface, &MainViewport, Extent1);
		Viewports[ViewportsCount] = &MainViewport;
		++ViewportsCount;

		ViewportInstance* Viewport = static_cast<ViewportInstance*>(Util::Memory::Allocate(sizeof(ViewportInstance)));
		InitViewport(Window2, Surface2, Viewport, Extent2);
		Viewports[ViewportsCount] = Viewport;
		++ViewportsCount;





		return true;
	}

	void VulkanRenderingSystem::DeInit()
	{
		vkDeviceWaitIdle(LogicalDevice);

		

		// UNIFORM BUFFER SETUP CODE
		//_aligned_free(ModelTransferSpace);

		DeinitViewport(&MainViewport);

		for (int ViewportIndex = 1; ViewportIndex < ViewportsCount; ++ViewportIndex)
		{
			ViewportInstance* ProcessedViewport = Viewports[ViewportIndex];
			DeinitViewport(ProcessedViewport);
			Util::Memory::Deallocate(ProcessedViewport);
		}

		vkDestroySampler(LogicalDevice, TextureSampler, nullptr);

		for (uint32_t i = 0; i < TextureImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, TextureImageBuffer[i].ImageView, nullptr);
			vkDestroyImage(LogicalDevice, TextureImageBuffer[i].Image, nullptr);
			vkFreeMemory(LogicalDevice, TextureImageBuffer[i].Memory, nullptr);
		}

		vkDestroyDescriptorPool(LogicalDevice, SamplerDescriptorPool, nullptr);

		// UNIFORM BUFFER SETUP CODE
		//Util::Memory::Deallocate(ModelDynamicUniformBuffers);

		for (uint32_t i = 0; i < DrawableObjectsCount; ++i)
		{
			DestroyGPUBuffer(DrawableObjects[i].VertexBuffer);
			DestroyGPUBuffer(DrawableObjects[i].IndexBuffer);
		}

		MainPass.DeInit(LogicalDevice);

		Util::Memory::Deallocate(SamplerDescriptorSets);

		vkDestroyDevice(LogicalDevice, nullptr);
		MainInstance::DestroyMainInstance(Instance);
	}

	bool VulkanRenderingSystem::CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, uint32_t AvailableExtensionsCount,
		const char** RequiredExtensions, uint32_t RequiredExtensionsCount)
	{
		for (uint32_t i = 0; i < RequiredExtensionsCount; ++i)
		{
			bool IsExtensionSupported = false;
			for (uint32_t j = 0; j < AvailableExtensionsCount; ++j)
			{
				if (std::strcmp(RequiredExtensions[i], AvailableExtensions[j].extensionName) == 0)
				{
					IsExtensionSupported = true;
					break;
				}
			}

			if (!IsExtensionSupported)
			{
				Util::Log().Error("Extension {} unsupported", RequiredExtensions[i]);
				return false;
			}
		}

		return true;
	}

	bool VulkanRenderingSystem::CheckValidationLayersSupport(VkLayerProperties* Properties, uint32_t PropertiesSize,
		const char** ValidationLeyersToCheck, uint32_t ValidationLeyersToCheckSize)
	{
		for (uint32_t i = 0; i < ValidationLeyersToCheckSize; ++i)
		{
			bool IsLayerAvalible = false;
			for (uint32_t j = 0; j < PropertiesSize; ++j)
			{
				if (std::strcmp(ValidationLeyersToCheck[i], Properties[j].layerName) == 0)
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

	VkExtent2D VulkanRenderingSystem::GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
		GLFWwindow* Window)
	{
		if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return SurfaceCapabilities.currentExtent;
		}
		else
		{
			int Width;
			int Height;
			glfwGetFramebufferSize(Window, &Width, &Height);

			Width = std::clamp(static_cast<uint32_t>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
			Height = std::clamp(static_cast<uint32_t>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

			return { static_cast<uint32_t>(Width), static_cast<uint32_t>(Height) };
		}
	}

	void VulkanRenderingSystem::GetAvailableExtensionProperties(std::vector<VkExtensionProperties>& Data)
	{
		uint32_t Count;
		const VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceExtensionProperties result is {}", static_cast<int>(Result));
			assert(false);
		}

		Data.resize(Count);
		vkEnumerateInstanceExtensionProperties(nullptr, &Count, Data.data());
	}

	void VulkanRenderingSystem::GetAvailableInstanceLayerProperties(std::vector<VkLayerProperties>& Data)
	{
		uint32_t Count;
		const VkResult Result = vkEnumerateInstanceLayerProperties(&Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceLayerProperties result is {}", static_cast<int>(Result));
			assert(false);
		}

		Data.resize(Count);
		vkEnumerateInstanceLayerProperties(&Count, Data.data());
	}

	void VulkanRenderingSystem::GetRequiredInstanceExtensions(const char** ValidationExtensions,
		uint32_t ValidationExtensionsCount, std::vector<const char*>& Data)
	{
		uint32_t RequiredExtensionsCount = 0;
		const char** RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&RequiredExtensionsCount);
		if (RequiredExtensionsCount == 0)
		{
			Util::Log().GlfwLogError();
			assert(false);
		}

		Data.resize(RequiredExtensionsCount + ValidationExtensionsCount);
		for (uint32_t i = 0; i < RequiredExtensionsCount; ++i)
		{
			Data[i] = RequiredInstanceExtensions[i];
			Util::Log().Info("Requested {} extension", Data[i]);
		}

		for (uint32_t i = 0; i < ValidationExtensionsCount; ++i)
		{
			Data[i + RequiredExtensionsCount] = ValidationExtensions[i];
			Util::Log().Info("Requested {} extension", Data[i]);
		}
	}

	void VulkanRenderingSystem::GetSurfaceFormats(VkSurfaceKHR Surface, std::vector<VkSurfaceFormatKHR>& SurfaceFormats)
	{
		uint32_t Count;
		const VkResult Result = vkGetPhysicalDeviceSurfaceFormatsKHR(Device.PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfaceFormatsKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		SurfaceFormats.resize(Count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device.PhysicalDevice, Surface, &Count, SurfaceFormats.data());
	}

	void VulkanRenderingSystem::GetAvailablePresentModes(VkSurfaceKHR Surface, std::vector<VkPresentModeKHR>& PresentModes)
	{
		uint32_t Count;
		const VkResult Result = vkGetPhysicalDeviceSurfacePresentModesKHR(Device.PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfacePresentModesKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		PresentModes.resize(Count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device.PhysicalDevice, Surface, &Count, PresentModes.data());
	}

	void VulkanRenderingSystem::GetSwapchainImages(VkSwapchainKHR VulkanSwapchain, std::vector<VkImage>& Images)
	{
		uint32_t Count;
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, nullptr);

		Images.resize(Count);
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, Images.data());
	}

	uint32_t VulkanRenderingSystem::GetMemoryTypeIndex(uint32_t AllowedTypes, VkMemoryPropertyFlags Properties)
	{
		VkPhysicalDeviceMemoryProperties MemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(Device.PhysicalDevice, &MemoryProperties);

		for (uint32_t MemoryTypeIndex = 0; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++)
		{
			if ((AllowedTypes & (1 << MemoryTypeIndex))														// Index of memory type must match corresponding bit in allowedTypes
				&& (MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & Properties) == Properties)	// Desired property bit flags are part of memory type's property flags
			{
				// This memory type is valid, so return its index
				return MemoryTypeIndex;
			}
		}

		// Todo Error?
		return 0;
	}

	VkImageView VulkanRenderingSystem::CreateImageView(VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags)
	{
		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.image = Image;
		ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCreateInfo.format = Format;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ViewCreateInfo.subresourceRange.layerCount = 1;

		// Create image view and return it
		VkImageView ImageView;
		VkResult Result = vkCreateImageView(LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
		if (Result != VK_SUCCESS)
		{
			// Todo Error?
			return nullptr;
		}

		return ImageView;
	}

	GPUBuffer VulkanRenderingSystem::CreateVertexBuffer(Vertex* Vertices, uint32_t VerticesCount)
	{
		const VkDeviceSize BufferSize = sizeof(Vertex) * VerticesCount;

		GPUBuffer StagingBuffer = CreateGPUBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* Data;
		vkMapMemory(LogicalDevice, StagingBuffer.Memory, 0, BufferSize, 0, &Data);
		memcpy(Data, Vertices, (size_t)BufferSize);
		vkUnmapMemory(LogicalDevice, StagingBuffer.Memory);

		GPUBuffer vertexBuffer = CreateGPUBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy staging buffer to vertex buffer on GPU;
		CopyGPUBuffer(StagingBuffer, vertexBuffer);
		DestroyGPUBuffer(StagingBuffer);

		return vertexBuffer;
	}

	GPUBuffer VulkanRenderingSystem::CreateIndexBuffer(uint32_t* Indices, uint32_t IndicesCount)
	{
		const VkDeviceSize BufferSize = sizeof(uint32_t) * IndicesCount;

		GPUBuffer StagingBuffer = CreateGPUBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* Data;
		vkMapMemory(LogicalDevice, StagingBuffer.Memory, 0, BufferSize, 0, &Data);
		memcpy(Data, Indices, (size_t)BufferSize);
		vkUnmapMemory(LogicalDevice, StagingBuffer.Memory);

		// Create buffer for INDEX data on GPU access only area
		GPUBuffer indexBuffer = CreateGPUBuffer(BufferSize, 	VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Copy from staging buffer to GPU access buffer
		CopyGPUBuffer(StagingBuffer, indexBuffer);
		DestroyGPUBuffer(StagingBuffer);

		return indexBuffer;
	}

	GPUBuffer VulkanRenderingSystem::CreateUniformBuffer(VkDeviceSize BufferSize)
	{
		GPUBuffer uniformBuffer = CreateGPUBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		return uniformBuffer;
	}

	GPUBuffer VulkanRenderingSystem::CreateIndirectBuffer(const std::vector<VkDrawIndexedIndirectCommand>& DrawCommands)
	{
		const VkDeviceSize BufferSize = sizeof(VkDrawIndexedIndirectCommand) * DrawCommands.size();

		GPUBuffer StagingBuffer = CreateGPUBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* Data;
		vkMapMemory(LogicalDevice, StagingBuffer.Memory, 0, BufferSize, 0, &Data);
		memcpy(Data, DrawCommands.data(), (size_t)BufferSize);
		vkUnmapMemory(LogicalDevice, StagingBuffer.Memory);

		GPUBuffer indirectBuffer = CreateGPUBuffer(BufferSize, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// Copy from staging buffer to GPU access buffer
		CopyGPUBuffer(StagingBuffer, indirectBuffer);
		DestroyGPUBuffer(StagingBuffer);

		return indirectBuffer;
	}

	GPUBuffer VulkanRenderingSystem::CreateGPUBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties)
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
		memoryAllocInfo.memoryTypeIndex = GetMemoryTypeIndex(memRequirements.memoryTypeBits,		// Index of memory type on Physical Device that has required bit flags
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

	void VulkanRenderingSystem::DestroyGPUBuffer(GPUBuffer& buffer)
	{
		vkDestroyBuffer(LogicalDevice, buffer.Buffer, nullptr);
		vkFreeMemory(LogicalDevice, buffer.Memory, nullptr);
	}

	void VulkanRenderingSystem::CopyGPUBuffer(const GPUBuffer& srcBuffer, GPUBuffer& dstBuffer)
	{
		assert(srcBuffer.Size <= dstBuffer.Size);

		VkCommandBuffer transferCommandBuffer;

		VkCommandBufferAllocateInfo allocInfo = { };
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = MainPass.GraphicsCommandPool;
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

		vkQueueSubmit(GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		vkFreeCommandBuffers(LogicalDevice, MainPass.GraphicsCommandPool, 1, &transferCommandBuffer);
	}

	void VulkanRenderingSystem::CopyBufferToImage(VkBuffer SourceBuffer, VkImage Image, uint32_t Width, uint32_t Height)
	{
		// Todo: Use 1 CommandBuffer for all copy operations?
		VkCommandBuffer TransferCommandBuffer;

		VkCommandBufferAllocateInfo AllocInfo = { };
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = MainPass.GraphicsCommandPool;
		AllocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(LogicalDevice, &AllocInfo, &TransferCommandBuffer);

		VkCommandBufferBeginInfo BeginInfo = { };
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo);

		VkBufferImageCopy ImageRegion = { };
		ImageRegion.bufferOffset = 0;
		ImageRegion.bufferRowLength = 0;
		ImageRegion.bufferImageHeight = 0;
		ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageRegion.imageSubresource.mipLevel = 0;
		ImageRegion.imageSubresource.baseArrayLayer = 0;						// Starting array layer (if array)
		ImageRegion.imageSubresource.layerCount = 1;
		ImageRegion.imageOffset = { 0, 0, 0 };
		ImageRegion.imageExtent = { Width, Height, 1 };

		// Todo copy multiple regions at once?
		vkCmdCopyBufferToImage(TransferCommandBuffer, SourceBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);

		vkEndCommandBuffer(TransferCommandBuffer);

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		vkFreeCommandBuffers(LogicalDevice, MainPass.GraphicsCommandPool, 1, &TransferCommandBuffer);
	}

	VkImage VulkanRenderingSystem::CreateImage(uint32_t Width, uint32_t Height,
		VkFormat Format, VkImageTiling Tiling, VkImageUsageFlags UseFlags, VkMemoryPropertyFlags PropFlags,
		VkDeviceMemory* OutImageMemory)
	{
		VkImageCreateInfo ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;						// Type of image (1D, 2D, or 3D)
		ImageCreateInfo.extent.width = Width;								// Width of image extent
		ImageCreateInfo.extent.height = Height;								// Height of image extent
		ImageCreateInfo.extent.depth = 1;									// Depth of image (just 1, no 3D aspect)
		ImageCreateInfo.mipLevels = 1;										// Number of mipmap levels
		ImageCreateInfo.arrayLayers = 1;									// Number of levels in image array
		ImageCreateInfo.format = Format;									// Format type of image
		ImageCreateInfo.tiling = Tiling;									// How image data should be "tiled" (arranged for optimal reading)
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Layout of image data on creation
		ImageCreateInfo.usage = UseFlags;									// Bit flags defining what image will be used for
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples for multi-sampling
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Whether image can be shared between queues

		// Create image
		VkImage Image;
		VkResult Result = vkCreateImage(LogicalDevice, &ImageCreateInfo, nullptr, &Image);
		if (Result != VK_SUCCESS)
		{
			return nullptr;
		}

		// CREATE MEMORY FOR IMAGE

		// Get memory requirements for a type of image
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(LogicalDevice, Image, &MemoryRequirements);

		// Allocate memory using image requirements and user defined properties
		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocInfo.memoryTypeIndex = GetMemoryTypeIndex(MemoryRequirements.memoryTypeBits,
			PropFlags);

		Result = vkAllocateMemory(LogicalDevice, &MemoryAllocInfo, nullptr, OutImageMemory);
		if (Result != VK_SUCCESS)
		{
			return nullptr;
		}

		// Connect memory to image
		vkBindImageMemory(LogicalDevice, Image, *OutImageMemory, 0);

		return Image;
	}

	VkDevice VulkanRenderingSystem::CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		uint32_t DeviceExtensionsSize)
	{
		const float Priority = 1.0f;

		// One family can support graphics and presentation
		// In that case create multiple VkDeviceQueueCreateInfo
		VkDeviceQueueCreateInfo QueueCreateInfos[2] = { };
		uint32_t FamilyIndicesSize = 1;

		QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfos[0].queueFamilyIndex = static_cast<uint32_t>(Indices.GraphicsFamily);
		QueueCreateInfos[0].queueCount = 1;
		QueueCreateInfos[0].pQueuePriorities = &Priority;

		if (Indices.GraphicsFamily != Indices.PresentationFamily)
		{
			QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfos[1].queueFamilyIndex = static_cast<uint32_t>(Indices.PresentationFamily);
			QueueCreateInfos[1].queueCount = 1;
			QueueCreateInfos[1].pQueuePriorities = &Priority;

			++FamilyIndicesSize;
		}

		VkPhysicalDeviceFeatures DeviceFeatures = { };
		DeviceFeatures.samplerAnisotropy = VK_TRUE; // Todo: get from configs
		DeviceFeatures.multiViewport = VK_TRUE; // Todo: get from configs

		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = FamilyIndicesSize;
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
		DeviceCreateInfo.enabledExtensionCount = DeviceExtensionsSize;
		DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions;
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

		VkDevice LogicalDevice;
		// Queues are created at the same time as the Device
		VkResult Result = vkCreateDevice(Device.PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDevice result is {}", static_cast<int>(Result));
			assert(false);
		}

		return LogicalDevice;
	}

	VkSwapchainKHR VulkanRenderingSystem::CreateSwapchain(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
		VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
		PhysicalDeviceIndices DeviceIndices)
	{
		// How many images are in the swap chain
		// Get 1 more then the minimum to allow triple buffering
		uint32_t ImageCount = SurfaceCapabilities.minImageCount + 1;

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

		uint32_t Indices[] = {
			static_cast<uint32_t>(DeviceIndices.GraphicsFamily),
			static_cast<uint32_t>(DeviceIndices.PresentationFamily)
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

	VkDescriptorPool VulkanRenderingSystem::CreateSamplerDescriptorPool(uint32_t Count)
	{
		VkDescriptorPoolSize SamplerPoolSize = { };
		// Todo: support VK_DESCRIPTOR_TYPE_SAMPLER and VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE separately
		SamplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		SamplerPoolSize.descriptorCount = Count; // Todo: max objects or max textures?

		VkDescriptorPoolCreateInfo SamplerPoolCreateInfo = { };
		SamplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		SamplerPoolCreateInfo.maxSets = Count;
		SamplerPoolCreateInfo.poolSizeCount = 1;
		SamplerPoolCreateInfo.pPoolSizes = &SamplerPoolSize;

		VkDescriptorPool Pool;
		const VkResult Result = vkCreateDescriptorPool(LogicalDevice, &SamplerPoolCreateInfo, nullptr, &Pool);
		if (Result != VK_SUCCESS)
		{
			//TODO: LOG
			assert(false);
		}

		return Pool;
	}

	VkSurfaceFormatKHR VulkanRenderingSystem::GetBestSurfaceFormat(VkSurfaceKHR Surface)
	{
		std::vector<VkSurfaceFormatKHR> FormatsData;
		GetSurfaceFormats(Surface, FormatsData);

		VkSurfaceFormatKHR Format = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };

		// All formats available
		if (FormatsData.size() == 1 && FormatsData[0].format == VK_FORMAT_UNDEFINED)
		{
			Format = { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
		else
		{
			for (uint32_t i = 0; i < FormatsData.size(); ++i)
			{
				VkSurfaceFormatKHR AvailableFormat = FormatsData[i];
				if ((AvailableFormat.format == VK_FORMAT_R8G8B8_UNORM || AvailableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
					&& AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					Format = AvailableFormat;
					break;
				}
			}
		}

		if (Format.format == VK_FORMAT_UNDEFINED)
		{
			Util::Log().Error("SurfaceFormat is undefined");
		}

		return Format;
	}

	VkPresentModeKHR VulkanRenderingSystem::GetBestPresentationMode(VkSurfaceKHR Surface)
	{
		std::vector<VkPresentModeKHR> PresentModes;
		GetAvailablePresentModes(Surface, PresentModes);

		VkPresentModeKHR Mode = VK_PRESENT_MODE_FIFO_KHR;
		for (uint32_t i = 0; i < PresentModes.size(); ++i)
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

	VkSampler VulkanRenderingSystem::CreateTextureSampler()
	{
		VkSamplerCreateInfo SamplerCreateInfo = { };
		SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
		SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
		SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in U (x) direction
		SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in V (y) direction
		SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in W (z) direction
		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
		SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
		SamplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
		SamplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
		SamplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
		SamplerCreateInfo.anisotropyEnable = VK_TRUE;						// Enable Anisotropy
		SamplerCreateInfo.maxAnisotropy = 16; // Todo: support in config

		VkSampler Sampler;
		VkResult Result = vkCreateSampler(LogicalDevice, &SamplerCreateInfo, nullptr, &Sampler);
		if (Result != VK_SUCCESS)
		{
			assert(false);
		}

		return Sampler;
	}

	bool VulkanRenderingSystem::SetupQueues()
	{
		vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(Device.Indices.GraphicsFamily), 0, &GraphicsQueue);
		vkGetDeviceQueue(LogicalDevice, static_cast<uint32_t>(Device.Indices.PresentationFamily), 0, &PresentationQueue);

		if (GraphicsQueue == nullptr && PresentationQueue == nullptr)
		{
			return false;
		}
	}

	bool VulkanRenderingSystem::IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags)
	{
		VkFormatProperties Properties;
		vkGetPhysicalDeviceFormatProperties(Device.PhysicalDevice, Format, &Properties);

		if (Tiling == VK_IMAGE_TILING_LINEAR && (Properties.linearTilingFeatures & FeatureFlags) == FeatureFlags)
		{
			return true;
		}
		else if (Tiling == VK_IMAGE_TILING_OPTIMAL && (Properties.optimalTilingFeatures & FeatureFlags) == FeatureFlags)
		{
			return true;
		}
	}

	void VulkanRenderingSystem::InitImageView(ViewportInstance* OutViewport, const std::vector<VkImage>& Images)
	{
		OutViewport->SwapchainImagesCount = Images.size();
		OutViewport->ImageViews = static_cast<VkImageView*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(VkImageView)));

		for (uint32_t i = 0; i < OutViewport->SwapchainImagesCount; ++i)
		{
			VkImageView ImageView = CreateImageView(Images[i], SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
			if (ImageView == nullptr)
			{
				assert(false);
			}

			OutViewport->ImageViews[i] = ImageView;
		}
	}

	bool VulkanRenderingSystem::LoadMesh(Mesh Mesh)
	{
		assert(DrawableObjectsCount < MaxObjects);

		DrawableObjects[DrawableObjectsCount].VertexBuffer = CreateVertexBuffer(Mesh.MeshVertices, Mesh.MeshVerticesCount);
		DrawableObjects[DrawableObjectsCount].VerticesCount = Mesh.MeshVerticesCount;

		DrawableObjects[DrawableObjectsCount].IndexBuffer = CreateIndexBuffer(Mesh.MeshIndices, Mesh.MeshIndicesCount);
		DrawableObjects[DrawableObjectsCount].IndicesCount = Mesh.MeshIndicesCount;

		DrawableObjects[DrawableObjectsCount].Model = glm::mat4(1.0f);

		++DrawableObjectsCount;

		return true;
	}

	void VulkanRenderingSystem::RemoveViewport(GLFWwindow* Window)
	{
		assert(Window != MainViewport.Window);

		for (int ViewportIndex = 1; ViewportIndex < ViewportsCount; ++ViewportIndex)
		{
			if (Viewports[ViewportIndex]->Window == Window)
			{
				DeinitViewport(Viewports[ViewportIndex]);
				Util::Memory::Deallocate(Viewports[ViewportIndex]);
				--ViewportsCount;
				return;
			}
		}

		assert(false);
	}

	bool VulkanRenderingSystem::Draw()
	{
		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkRenderPassBeginInfo RenderPassBeginInfo = { };
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.renderPass = MainPass.RenderPass;							// Render Pass to begin
		RenderPassBeginInfo.renderArea.offset = { 0, 0 };

		const uint32_t ClearValuesSize = 3;
		VkClearValue ClearValues[ClearValuesSize];
		// Todo: do not forget about position in array AttachmentDescriptions
		ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		ClearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		ClearValues[2].depthStencil.depth = 1.0f;

		RenderPassBeginInfo.pClearValues = ClearValues;
		RenderPassBeginInfo.clearValueCount = ClearValuesSize;

		// -- SUBMIT COMMAND BUFFER TO RENDER --
		// Queue submission information
		VkSubmitInfo submitInfo = { };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		submitInfo.pWaitDstStageMask = waitStages;						// Stages to check semaphores at
		submitInfo.commandBufferCount = 1;								// Number of command buffers to submit
		submitInfo.signalSemaphoreCount = 1;							// Number of semaphores to signal

		// -- PRESENT RENDERED IMAGE TO SCREEN --
		VkPresentInfoKHR presentInfo = { };
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		presentInfo.swapchainCount = 1;											// Number of swapchains to present to

		for (int ViewportIndex = 0; ViewportIndex < ViewportsCount; ++ViewportIndex)
		{
			ViewportInstance* ProcessedViewport = Viewports[ViewportIndex];

			vkWaitForFences(LogicalDevice, 1, &MainPass.DrawFences[CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
			vkResetFences(LogicalDevice, 1, &MainPass.DrawFences[CurrentFrame]);

			// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
			uint32_t ImageIndex;
			vkAcquireNextImageKHR(LogicalDevice, ProcessedViewport->VulkanSwapchain, std::numeric_limits<uint64_t>::max(),
				MainPass.ImageAvailable[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

			// Start point of render pass in pixels
			RenderPassBeginInfo.renderArea.extent = ProcessedViewport->SwapExtent;				// Size of region to run render pass on (starting at offset)
			RenderPassBeginInfo.framebuffer = ProcessedViewport->SwapchainFramebuffers[ImageIndex];

			VkResult Result = vkBeginCommandBuffer(ProcessedViewport->CommandBuffers[ImageIndex], &CommandBufferBeginInfo);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
				assert(false);
			}

			vkCmdBeginRenderPass(ProcessedViewport->CommandBuffers[ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(ProcessedViewport->CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.GraphicsPipeline);

			// TODO: Support rework to not create identical index buffers
			for (uint32_t j = 0; j < DrawableObjectsCount; ++j)
			{
				VkBuffer VertexBuffers[] = { DrawableObjects[j].VertexBuffer.Buffer };					// Buffers to bind
				VkDeviceSize Offsets[] = { 0 };												// Offsets into buffers being bound
				vkCmdBindVertexBuffers(ProcessedViewport->CommandBuffers[ImageIndex], 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(ProcessedViewport->CommandBuffers[ImageIndex], DrawableObjects[j].IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

				// UNIFORM BUFFER SETUP CODE
				//uint32_t DynamicOffset = ModelUniformAlignment * j;

				vkCmdPushConstants(ProcessedViewport->CommandBuffers[ImageIndex], MainPass.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
					0, sizeof(Model), &DrawableObjects[j].Model);

				// Todo: do not record textureId on each frame?
				const uint32_t DescriptorSetGroupCount = 2;
				VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] = { ProcessedViewport->DescriptorSets[ImageIndex],
					SamplerDescriptorSets[DrawableObjects[j].TextureId] };

				vkCmdBindDescriptorSets(ProcessedViewport->CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				// Execute pipeline
				//vkCmdDraw(ProcessedViewport->CommandBuffers[i], DrawableObject.VerticesCount, 1, 0, 0);
				vkCmdDrawIndexed(ProcessedViewport->CommandBuffers[ImageIndex], DrawableObjects[j].IndicesCount, 1, 0, 0, 0);
			}

			vkCmdNextSubpass(ProcessedViewport->CommandBuffers[ImageIndex], VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(ProcessedViewport->CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.SecondPipeline);
			vkCmdBindDescriptorSets(ProcessedViewport->CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.SecondPipelineLayout,
				0, 1, &ProcessedViewport->InputDescriptorSets[ImageIndex], 0, nullptr);
			vkCmdDraw(ProcessedViewport->CommandBuffers[ImageIndex], 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass

			// End Render Pass
			vkCmdEndRenderPass(ProcessedViewport->CommandBuffers[ImageIndex]);

			Result = vkEndCommandBuffer(ProcessedViewport->CommandBuffers[ImageIndex]);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
				return false;
			}
			// Function end RecordCommands

			//UpdateUniformBuffer
			void* Data;
			vkMapMemory(LogicalDevice, ProcessedViewport->VpUniformBuffers[ImageIndex].Memory, 0, sizeof(UboViewProjection),
				0, &Data);
			std::memcpy(Data, &ProcessedViewport->ViewProjection, sizeof(UboViewProjection));
			vkUnmapMemory(LogicalDevice, ProcessedViewport->VpUniformBuffers[ImageIndex].Memory);

			// UNIFORM BUFFER SETUP CODE
			//for (uint32_t i = 0; i < DrawableObjectsCount; ++i)
			//{
			//	UboModel* ThisModel = (UboModel*)((uint64_t)ModelTransferSpace + (i * ModelUniformAlignment));
			//	*ThisModel = DrawableObjects[i].Model;
			//}

			//vkMapMemory(LogicalDevice, ModelDynamicUniformBuffers[ImageIndex].BufferMemory, 0,
			//	ModelUniformAlignment * DrawableObjectsCount, 0, &Data);

			//std::memcpy(Data, ModelTransferSpace, ModelUniformAlignment * DrawableObjectsCount);
			//vkUnmapMemory(LogicalDevice, ModelDynamicUniformBuffers[ImageIndex].BufferMemory);


			submitInfo.pWaitSemaphores = &MainPass.ImageAvailable[CurrentFrame];				// List of semaphores to wait on
			submitInfo.pCommandBuffers = &ProcessedViewport->CommandBuffers[ImageIndex];		// Command buffer to submit
			submitInfo.pSignalSemaphores = &MainPass.RenderFinished[CurrentFrame];	// Semaphores to signal when command buffer finishes

			// Submit command buffer to queue
			Result = vkQueueSubmit(GraphicsQueue, 1, &submitInfo, MainPass.DrawFences[CurrentFrame]);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkQueueSubmit result is {}", static_cast<int>(Result));
				return false;
			}

			presentInfo.pWaitSemaphores = &MainPass.RenderFinished[CurrentFrame];			// Semaphores to wait on
			presentInfo.pSwapchains = &ProcessedViewport->VulkanSwapchain;									// Swapchains to present images to
			presentInfo.pImageIndices = &ImageIndex;								// Index of images in swapchains to present

			// Present image
			Result = vkQueuePresentKHR(PresentationQueue, &presentInfo);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkQueuePresentKHR result is {}", static_cast<int>(Result));
				assert(false);
			}
		}

		CurrentFrame = (CurrentFrame + 1) % MaxFrameDraws;

		return true;
	}

	void VulkanRenderingSystem::DeinitViewport(ViewportInstance* Viewport)
	{
		vkDeviceWaitIdle(LogicalDevice);

		for (uint32_t i = 0; i < Viewport->SwapchainImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, Viewport->DepthBuffers[i].ImageView, nullptr);
			vkDestroyImage(LogicalDevice, Viewport->DepthBuffers[i].Image, nullptr);
			vkFreeMemory(LogicalDevice, Viewport->DepthBuffers[i].Memory, nullptr);

			vkDestroyImageView(LogicalDevice, Viewport->ColorBuffers[i].ImageView, nullptr);
			vkDestroyImage(LogicalDevice, Viewport->ColorBuffers[i].Image, nullptr);
			vkFreeMemory(LogicalDevice, Viewport->ColorBuffers[i].Memory, nullptr);

			DestroyGPUBuffer(Viewport->VpUniformBuffers[i]);
			vkDestroyFramebuffer(LogicalDevice, Viewport->SwapchainFramebuffers[i], nullptr);
			vkDestroyImageView(LogicalDevice, Viewport->ImageViews[i], nullptr);
			// UNIFORM BUFFER SETUP CODE
			//Core::DestroyGenericBuffer(RenderInstance, ModelDynamicUniformBuffers[i]);
		}

		vkDestroyDescriptorPool(LogicalDevice, Viewport->InputDescriptorPool, nullptr);
		vkDestroyDescriptorPool(LogicalDevice, Viewport->DescriptorPool, nullptr);

		vkDestroySwapchainKHR(LogicalDevice, Viewport->VulkanSwapchain, nullptr);

		Util::Memory::Deallocate(Viewport->SwapchainFramebuffers);
		Util::Memory::Deallocate(Viewport->DepthBuffers);
		Util::Memory::Deallocate(Viewport->ColorBuffers);
		Util::Memory::Deallocate(Viewport->DescriptorSets);
		Util::Memory::Deallocate(Viewport->InputDescriptorSets);
		Util::Memory::Deallocate(Viewport->VpUniformBuffers);
		Util::Memory::Deallocate(Viewport->CommandBuffers);
		Util::Memory::Deallocate(Viewport->ImageViews);
	}

	// TODO: CHECK
	void VulkanRenderingSystem::CreateImageBuffer(stbi_uc* TextureData, int Width, int Height, VkDeviceSize ImageSize)
	{
		assert(TextureImagesCount < MaxTextures);

		GPUBuffer StagingBuffer = CreateGPUBuffer(ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* Data;
		vkMapMemory(LogicalDevice, StagingBuffer.Memory, 0, ImageSize, 0, &Data);
		memcpy(Data, TextureData, static_cast<size_t>(ImageSize));
		vkUnmapMemory(LogicalDevice, StagingBuffer.Memory);

		ImageBuffer ImageBuffeObject;
		ImageBuffeObject.Image = CreateImage(Width, Height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ImageBuffeObject.Memory);

		// Todo: Create beginCommandBuffer function?
		VkCommandBuffer CommandBuffer;

		VkCommandBufferAllocateInfo AllocInfo = { };
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = MainPass.GraphicsCommandPool;
		AllocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(LogicalDevice, &AllocInfo, &CommandBuffer);

		VkCommandBufferBeginInfo BeginInfo = { };
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

		VkImageMemoryBarrier ImageMemoryBarrier = { };
		ImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;									// Layout to transition from
		ImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;									// Layout to transition to
		ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
		ImageMemoryBarrier.image = ImageBuffeObject.Image;											// Image being accessed and modified as part of barrier
		ImageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
		ImageMemoryBarrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
		ImageMemoryBarrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from baseMipLevel
		ImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;						// First layer to start alterations on
		ImageMemoryBarrier.subresourceRange.layerCount = 1;							// Number of layers to alter starting from baseArrayLayer

		ImageMemoryBarrier.srcAccessMask = 0;								// Memory access stage transition must after...
		ImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// Memory access stage transition must before...

		VkPipelineStageFlags SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		vkCmdPipelineBarrier(
			CommandBuffer,
			SrcStage, DstStage,		// Pipeline stages (match to src and dst AccessMasks)
			0,						// Dependency flags
			0, nullptr,				// Memory Barrier count + data
			0, nullptr,				// Buffer Memory Barrier count + data
			1, &ImageMemoryBarrier	// Image Memory Barrier count + data
		);

		vkEndCommandBuffer(CommandBuffer);

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffer;

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		// Copy image data
		CopyBufferToImage(StagingBuffer.Buffer, ImageBuffeObject.Image, Width, Height);

		ImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;									// Layout to transition from
		ImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		ImageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

		vkCmdPipelineBarrier(CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrier);

		vkEndCommandBuffer(CommandBuffer);

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		vkFreeCommandBuffers(LogicalDevice, MainPass.GraphicsCommandPool, 1, &CommandBuffer);
		DestroyGPUBuffer(StagingBuffer);

		//CreateImageView
		ImageBuffeObject.ImageView = CreateImageView(ImageBuffeObject.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		//CreateTextureDescriptor
		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = SamplerDescriptorPool;
		SetAllocInfo.descriptorSetCount = 1;
		SetAllocInfo.pSetLayouts = &MainPass.SamplerSetLayout;

		VkResult result = vkAllocateDescriptorSets(LogicalDevice, &SetAllocInfo, &SamplerDescriptorSets[TextureImagesCount]);
		if (result != VK_SUCCESS)
		{
			//return -1
		}

		VkDescriptorImageInfo ImageInfo = { };
		ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	// Image layout when in use
		ImageInfo.imageView = ImageBuffeObject.ImageView;									// Image to bind to set
		ImageInfo.sampler = TextureSampler;									// Sampler to use for set

		// Descriptor Write Info
		VkWriteDescriptorSet DescriptorWrite = { };
		DescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		DescriptorWrite.dstSet = SamplerDescriptorSets[TextureImagesCount];
		DescriptorWrite.dstBinding = 0;
		DescriptorWrite.dstArrayElement = 0;
		DescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		DescriptorWrite.descriptorCount = 1;
		DescriptorWrite.pImageInfo = &ImageInfo;

		// Todo: create descriptor sets for multiple textures?
		vkUpdateDescriptorSets(LogicalDevice, 1, &DescriptorWrite, 0, nullptr);

		TextureImageBuffer[TextureImagesCount] = ImageBuffeObject;
		++TextureImagesCount;
	}

	void VulkanRenderingSystem::InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport, VkExtent2D SwapExtent)
	{
		OutViewport->Window = Window;
		OutViewport->Surface = Surface;
		OutViewport->SwapExtent = SwapExtent;

		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };
		VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device.PhysicalDevice, OutViewport->Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		OutViewport->VulkanSwapchain = CreateSwapchain(SurfaceCapabilities, Surface, SurfaceFormat, OutViewport->SwapExtent,
			PresentationMode, Device.Indices);

		std::vector<VkImage> Images;
		GetSwapchainImages(OutViewport->VulkanSwapchain, Images);

		InitImageView(OutViewport, Images);

		OutViewport->DepthBuffers = static_cast<ImageBuffer*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(ImageBuffer)));
		OutViewport->ColorBuffers = static_cast<ImageBuffer*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(ImageBuffer)));

		for (uint32_t i = 0; i < OutViewport->SwapchainImagesCount; ++i)
		{
			// Function CreateDepthBuffer
			OutViewport->DepthBuffers[i].Image = CreateImage(OutViewport->SwapExtent.width, OutViewport->SwapExtent.height,
				DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&OutViewport->DepthBuffers[i].Memory);

			OutViewport->DepthBuffers[i].ImageView = CreateImageView(OutViewport->DepthBuffers[i].Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
			// Function end CreateDepthBuffer

			// Function CreateColorBuffer
			// VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT says that image can be used only as attachment. Used only for sub pass
			OutViewport->ColorBuffers[i].Image = CreateImage(OutViewport->SwapExtent.width, OutViewport->SwapExtent.height,
				ColorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&OutViewport->ColorBuffers[i].Memory);

			OutViewport->ColorBuffers[i].ImageView = CreateImageView(OutViewport->ColorBuffers[i].Image, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			// Function end CreateDepthBuffer
		}

		OutViewport->SwapchainFramebuffers = static_cast<VkFramebuffer*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(VkFramebuffer)));

		// Function CreateFrameBuffers
		// Create a framebuffer for each swap chain image
		for (uint32_t i = 0; i < OutViewport->SwapchainImagesCount; i++)
		{
			const uint32_t AttachmentsCount = 3;
			VkImageView Attachments[AttachmentsCount] = {
				OutViewport->ImageViews[i],
				// Todo: do not forget about position in array AttachmentDescriptions
				OutViewport->ColorBuffers[i].ImageView,
				OutViewport->DepthBuffers[i].ImageView
			};

			VkFramebufferCreateInfo FramebufferCreateInfo = { };
			FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			FramebufferCreateInfo.renderPass = MainPass.RenderPass;								// Render Pass layout the Framebuffer will be used with
			FramebufferCreateInfo.attachmentCount = AttachmentsCount;
			FramebufferCreateInfo.pAttachments = Attachments;							// List of attachments (1:1 with Render Pass)
			FramebufferCreateInfo.width = OutViewport->SwapExtent.width;								// Framebuffer width
			FramebufferCreateInfo.height = OutViewport->SwapExtent.height;							// Framebuffer height
			FramebufferCreateInfo.layers = 1;											// Framebuffer layers

			Result = vkCreateFramebuffer(LogicalDevice, &FramebufferCreateInfo, nullptr, &OutViewport->SwapchainFramebuffers[i]);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkCreateFramebuffer result is {}", static_cast<int>(Result));
				assert(false);
			}
		}
		// Function end CreateFrameBuffers

		// Function CreateCommandBuffers
		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = { };
		CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CommandBufferAllocateInfo.commandPool = MainPass.GraphicsCommandPool;
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		CommandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(OutViewport->SwapchainImagesCount);

		// Allocate command buffers and place handles in array of buffers
		OutViewport->CommandBuffers = static_cast<VkCommandBuffer*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(VkCommandBuffer)));
		Result = vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, OutViewport->CommandBuffers);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateCommandBuffers result is {}", static_cast<int>(Result));
			assert(false);
		}
		// Function end CreateCommandBuffers

		// Function CreateUniformBuffers
		const VkDeviceSize VpBufferSize = sizeof(UboViewProjection);
		//const VkDeviceSize ModelBufferSize = ModelUniformAlignment * MaxObjects;

		OutViewport->VpUniformBuffers = static_cast<GPUBuffer*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(GPUBuffer)));
		// UNIFORM BUFFER SETUP CODE
		//ModelDynamicUniformBuffers = static_cast<GenericBuffer*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(GenericBuffer)));

		// Create Uniform buffers
		for (uint32_t i = 0; i < OutViewport->SwapchainImagesCount; i++)
		{
			OutViewport->VpUniformBuffers[i] = CreateUniformBuffer(VpBufferSize);
			// UNIFORM BUFFER SETUP CODE
			//CreateGenericBuffer(RenderInstance, ModelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ModelDynamicUniformBuffers[i]);
		}

		// Function end CreateUniformBuffers

		// Function CreateDescriptorPool
		VkDescriptorPoolSize VpPoolSize = { };
		VpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VpPoolSize.descriptorCount = OutViewport->SwapchainImagesCount;

		// UNIFORM BUFFER SETUP CODE
		//VkDescriptorPoolSize ModelPoolSize = {};
		//ModelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		//ModelPoolSize.descriptorCount = OutViewport->SwapchainImagesCount;

		//const uint32_t PoolSizeCount = 2;
		//VkDescriptorPoolSize PoolSizes[PoolSizeCount] = { VpPoolSize, ModelPoolSize };

		const uint32_t PoolSizeCount = 1;
		VkDescriptorPoolSize PoolSizes[PoolSizeCount] = { VpPoolSize };

		// Data to create Descriptor Pool
		VkDescriptorPoolCreateInfo PoolCreateInfo = { };
		PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolCreateInfo.maxSets = OutViewport->SwapchainImagesCount;	// Maximum number of Descriptor Sets that can be created from pool
		PoolCreateInfo.poolSizeCount = PoolSizeCount;										// Amount of Pool Sizes being passed
		PoolCreateInfo.pPoolSizes = PoolSizes;									// Pool Sizes to create pool with

		// Create Descriptor Pool
		Result = vkCreateDescriptorPool(LogicalDevice, &PoolCreateInfo, nullptr, &OutViewport->DescriptorPool);
		if (Result != VK_SUCCESS)
		{
			assert(false);
		}

		// CREATE INPUT ATTACHMENT DESCRIPTOR POOL
		VkDescriptorPoolSize ColourInputPoolSize = { };
		ColourInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		ColourInputPoolSize.descriptorCount = OutViewport->SwapchainImagesCount;

		VkDescriptorPoolSize DepthInputPoolSize = { };
		DepthInputPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		DepthInputPoolSize.descriptorCount = OutViewport->SwapchainImagesCount;

		// Todo: do not copy VkDescriptorPoolSize
		const uint32_t InputPoolSizesCount = 2;
		VkDescriptorPoolSize InputPoolSizes[InputPoolSizesCount] = { ColourInputPoolSize, DepthInputPoolSize };

		VkDescriptorPoolCreateInfo InputPoolCreateInfo = { };
		InputPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		InputPoolCreateInfo.maxSets = OutViewport->SwapchainImagesCount;
		InputPoolCreateInfo.poolSizeCount = InputPoolSizesCount;
		InputPoolCreateInfo.pPoolSizes = InputPoolSizes;

		Result = vkCreateDescriptorPool(LogicalDevice, &InputPoolCreateInfo, nullptr, &OutViewport->InputDescriptorPool);
		if (Result != VK_SUCCESS)
		{
			assert(false);
		}
		// Function end CreateDescriptorPool
		// 
		// Function CreateDescriptorSets
		OutViewport->DescriptorSets = static_cast<VkDescriptorSet*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(VkDescriptorSet)));
		OutViewport->InputDescriptorSets = static_cast<VkDescriptorSet*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(VkDescriptorSet)));

		VkDescriptorSetLayout* SetLayouts = static_cast<VkDescriptorSetLayout*>(Util::Memory::Allocate(OutViewport->SwapchainImagesCount * sizeof(VkDescriptorSetLayout)));
		for (uint32_t i = 0; i < OutViewport->SwapchainImagesCount; i++)
		{
			SetLayouts[i] = MainPass.DescriptorSetLayout;
		}

		// Descriptor Set Allocation Info
		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = OutViewport->DescriptorPool;									// Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = OutViewport->SwapchainImagesCount;	// Number of sets to allocate
		SetAllocInfo.pSetLayouts = SetLayouts;									// Layouts to use to allocate sets (1:1 relationship)

		// Allocate descriptor sets (multiple)
		Result = vkAllocateDescriptorSets(LogicalDevice, &SetAllocInfo, OutViewport->DescriptorSets);
		if (Result != VK_SUCCESS)
		{
			assert(false);
		}

		VkDescriptorBufferInfo VpBufferInfo = { };
		// UNIFORM BUFFER SETUP CODE
		//VkDescriptorBufferInfo ModelBufferInfo = {};
		// Update all of descriptor set buffer bindings
		for (uint32_t i = 0; i < OutViewport->SwapchainImagesCount; i++)
		{
			// Todo: validate
			VpBufferInfo.buffer = OutViewport->VpUniformBuffers[i].Buffer;
			VpBufferInfo.offset = 0;
			VpBufferInfo.range = sizeof(UboViewProjection);

			VkWriteDescriptorSet VpSetWrite = { };
			VpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			VpSetWrite.dstSet = OutViewport->DescriptorSets[i];
			VpSetWrite.dstBinding = 0;
			VpSetWrite.dstArrayElement = 0;
			VpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			VpSetWrite.descriptorCount = 1;
			VpSetWrite.pBufferInfo = &VpBufferInfo;

			// UNIFORM BUFFER SETUP CODE
			//ModelBufferInfo.buffer = ModelDynamicUniformBuffers[i].Buffer;
			//ModelBufferInfo.offset = 0;
			//ModelBufferInfo.range = ModelUniformAlignment;

			//VkWriteDescriptorSet ModelSetWrite = {};
			//ModelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			//ModelSetWrite.dstSet = OutViewport->DescriptorSets[i];
			//ModelSetWrite.dstBinding = 1;
			//ModelSetWrite.dstArrayElement = 0;
			//ModelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			//ModelSetWrite.descriptorCount = 1;
			//ModelSetWrite.pBufferInfo = &ModelBufferInfo;

			//const uint32_t WriteSetsCount = 2;
			//VkWriteDescriptorSet WriteSets[WriteSetsCount] = { VpSetWrite, ModelSetWrite };

			const uint32_t WriteSetsCount = 1;
			// Todo: do not copy
			VkWriteDescriptorSet WriteSets[WriteSetsCount] = { VpSetWrite };

			// Todo: use one vkUpdateDescriptorSets call to update all descriptors
			vkUpdateDescriptorSets(LogicalDevice, WriteSetsCount, WriteSets, 0, nullptr);
		}

		for (uint32_t i = 0; i < OutViewport->SwapchainImagesCount; i++)
		{
			SetLayouts[i] = MainPass.InputSetLayout;
		}

		SetAllocInfo.descriptorPool = OutViewport->InputDescriptorPool;
		SetAllocInfo.descriptorSetCount = OutViewport->SwapchainImagesCount;

		Result = vkAllocateDescriptorSets(LogicalDevice, &SetAllocInfo, OutViewport->InputDescriptorSets);
		if (Result != VK_SUCCESS)
		{
			assert(false);
		}

		// Todo: move this and previus loop to function?
		for (size_t i = 0; i < OutViewport->SwapchainImagesCount; i++)
		{
			// Todo: move from loop?
			VkDescriptorImageInfo ColourAttachmentDescriptor = { };
			ColourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColourAttachmentDescriptor.imageView = OutViewport->ColorBuffers[i].ImageView;
			ColourAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkWriteDescriptorSet ColourWrite = { };
			ColourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ColourWrite.dstSet = OutViewport->InputDescriptorSets[i];
			ColourWrite.dstBinding = 0;
			ColourWrite.dstArrayElement = 0;
			ColourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			ColourWrite.descriptorCount = 1;
			ColourWrite.pImageInfo = &ColourAttachmentDescriptor;

			VkDescriptorImageInfo DepthAttachmentDescriptor = { };
			DepthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthAttachmentDescriptor.imageView = OutViewport->DepthBuffers[i].ImageView;
			DepthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkWriteDescriptorSet DepthWrite = { };
			DepthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DepthWrite.dstSet = OutViewport->InputDescriptorSets[i];
			DepthWrite.dstBinding = 1;
			DepthWrite.dstArrayElement = 0;
			DepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			DepthWrite.descriptorCount = 1;
			DepthWrite.pImageInfo = &DepthAttachmentDescriptor;


			// Todo: do not copy
			const uint32_t CetWritesCount = 2;
			VkWriteDescriptorSet SetWrites[CetWritesCount] = { ColourWrite, DepthWrite };

			// Update descriptor sets
			vkUpdateDescriptorSets(LogicalDevice, CetWritesCount, SetWrites, 0, nullptr);
		}

		Util::Memory::Deallocate(SetLayouts);
		// Function end CreateDescriptorSets
	}
}
