#include "VulkanRenderingSystem.h"

#include <algorithm>

#include "VulkanHelper.h"
#include "Util/Util.h"

namespace Core
{
	bool VulkanRenderingSystem::Init(GLFWwindow* Window)
	{
		const char* ValidationLayers[] = {
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_monitor"
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
		
		Device.Init(Instance.VulkanInstance, Surface, DeviceExtensions, DeviceExtensionsSize);

		LogicalDevice = CreateLogicalDevice(Device.Indices, DeviceExtensions, DeviceExtensionsSize);

		VkSurfaceFormatKHR SurfaceFormat = GetBestSurfaceFormat(Surface); // TODO: Check for surface 2

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



		CreateSynchronisation();
		SetupQueues();


		
		SwapchainInstance SwapInstance1 = SwapchainInstance::CreateSwapchainInstance(Device.PhysicalDevice, Device.Indices,
			LogicalDevice, Surface, SurfaceFormat, Extent1);


		std::vector<VkDescriptorPoolSize> TotalPassPoolSizes;
		uint32_t TotalDescriptorCount;
		MainRenderPass::GetPoolSizes(SwapInstance1.ImagesCount, TotalPassPoolSizes, TotalDescriptorCount);

		DescriptorPool = CreateDescriptorPool(LogicalDevice, TotalPassPoolSizes.data(), TotalPassPoolSizes.size(), TotalDescriptorCount);

		MainPass.CreateVulkanPass(LogicalDevice, ColorFormat, DepthFormat, SurfaceFormat);
		MainPass.SetupPushConstants();
		MainPass.CreateSamplerSetLayout(LogicalDevice);
		MainPass.CreateCommandPool(LogicalDevice, Device.Indices.GraphicsFamily);
		MainPass.CreateTerrainSetLayout(LogicalDevice);
		MainPass.CreateEntitySetLayout(LogicalDevice);
		MainPass.CreateDeferredSetLayout(LogicalDevice);
		MainPass.CreatePipelineLayouts(LogicalDevice);
		MainPass.CreatePipelines(LogicalDevice, Extent1);
		MainPass.CreateAttachments(Device.PhysicalDevice, LogicalDevice, SwapInstance1.ImagesCount, Extent1, DepthFormat, ColorFormat);
		MainPass.CreateUniformBuffers(Device.PhysicalDevice, LogicalDevice, SwapInstance1.ImagesCount);
		MainPass.CreateSets(LogicalDevice, DescriptorPool, SwapInstance1.ImagesCount);


		TextureSampler = CreateTextureSampler();
		SamplerDescriptorSets = static_cast<VkDescriptorSet*>(Util::Memory::Allocate(MaxTextures * sizeof(VkDescriptorSet)));
		SamplerDescriptorPool = CreateSamplerDescriptorPool(528); // TODO: Check 528

		InitViewport(Window, Surface, &MainViewport, DescriptorPool, SwapInstance1, MainPass.ColorBuffers, MainPass.DepthBuffers);

		return true;
	}

	void VulkanRenderingSystem::DeInit()
	{
		vkDeviceWaitIdle(LogicalDevice);
		
		DestroySynchronisation();

		vkDestroySampler(LogicalDevice, TextureSampler, nullptr);

		for (uint32_t i = 0; i < TextureImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, TextureImageBuffer[i].ImageView, nullptr);
			vkDestroyImage(LogicalDevice, TextureImageBuffer[i].Image, nullptr);
			vkFreeMemory(LogicalDevice, TextureImageBuffer[i].Memory, nullptr);
		}

		vkDestroyDescriptorPool(LogicalDevice, SamplerDescriptorPool, nullptr);
		vkDestroyDescriptorPool(LogicalDevice, DescriptorPool, nullptr);

		// UNIFORM BUFFER SETUP CODE
		//Util::Memory::Deallocate(ModelDynamicUniformBuffers);

		for (uint32_t i = 0; i < DrawableObjectsCount; ++i)
		{
			GPUBuffer::DestroyGPUBuffer(LogicalDevice, DrawableObjects[i].VertexBuffer);
			GPUBuffer::DestroyGPUBuffer(LogicalDevice, DrawableObjects[i].IndexBuffer);
		}

		MainPass.ClearResources(LogicalDevice, MainViewport.ViewportSwapchain.ImagesCount);

		DeinitViewport(&MainViewport);

		vkDestroyDevice(LogicalDevice, nullptr);

		Util::Memory::Deallocate(SamplerDescriptorSets);
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

	bool VulkanRenderingSystem::LoadMesh(Mesh Mesh)
	{
		assert(DrawableObjectsCount < MaxObjects);

		DrawableObjects[DrawableObjectsCount].VertexBuffer = GPUBuffer::CreateVertexBuffer(Device.PhysicalDevice, LogicalDevice, MainPass.GraphicsCommandPool, GraphicsQueue, Mesh.MeshVertices, Mesh.MeshVerticesCount);
		DrawableObjects[DrawableObjectsCount].VerticesCount = Mesh.MeshVerticesCount;

		DrawableObjects[DrawableObjectsCount].IndexBuffer = GPUBuffer::CreateIndexBuffer(Device.PhysicalDevice, LogicalDevice, MainPass.GraphicsCommandPool, GraphicsQueue, Mesh.MeshIndices, Mesh.MeshIndicesCount);
		DrawableObjects[DrawableObjectsCount].IndicesCount = Mesh.MeshIndicesCount;

		DrawableObjects[DrawableObjectsCount].Model = glm::mat4(1.0f);

		++DrawableObjectsCount;

		return true;
	}

	void VulkanRenderingSystem::CreateSynchronisation()
	{
		ImageAvailable = static_cast<VkSemaphore*>(Util::Memory::Allocate(MaxFrameDraws * sizeof(VkSemaphore)));
		RenderFinished = static_cast<VkSemaphore*>(Util::Memory::Allocate(MaxFrameDraws * sizeof(VkSemaphore)));
		DrawFences = static_cast<VkFence*>(Util::Memory::Allocate(MaxFrameDraws * sizeof(VkFence)));

		VkSemaphoreCreateInfo SemaphoreCreateInfo = { };
		SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// Fence creation information
		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MaxFrameDraws; i++)
		{
			if (vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &ImageAvailable[i]) != VK_SUCCESS ||
				vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderFinished[i]) != VK_SUCCESS ||
				vkCreateFence(LogicalDevice, &FenceCreateInfo, nullptr, &DrawFences[i]) != VK_SUCCESS)
			{
				Util::Log().Error("CreateSynchronisation error");
				assert(false);
			}
		}
	}

	void VulkanRenderingSystem::DestroySynchronisation()
	{
		for (size_t i = 0; i < MaxFrameDraws; i++)
		{
			vkDestroySemaphore(LogicalDevice, RenderFinished[i], nullptr);
			vkDestroySemaphore(LogicalDevice, ImageAvailable[i], nullptr);
			vkDestroyFence(LogicalDevice, DrawFences[i], nullptr);
		}
	}

	bool VulkanRenderingSystem::Draw()
	{
		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		const uint32_t ClearValuesSize = 3;
		VkClearValue ClearValues[ClearValuesSize];
		// Todo: do not forget about position in array AttachmentDescriptions
		ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		ClearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		ClearValues[2].depthStencil.depth = 1.0f;

		VkRenderPassBeginInfo RenderPassBeginInfo = { };
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.renderPass = MainPass.RenderPass;							// Render Pass to begin
		RenderPassBeginInfo.renderArea.offset = { 0, 0 };
		RenderPassBeginInfo.pClearValues = ClearValues;
		RenderPassBeginInfo.clearValueCount = ClearValuesSize;

		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		vkWaitForFences(LogicalDevice, 1, &DrawFences[CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(LogicalDevice, 1, &DrawFences[CurrentFrame]);

		// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
		uint32_t ImageIndex;
		vkAcquireNextImageKHR(LogicalDevice, MainViewport.ViewportSwapchain.VulkanSwapchain, std::numeric_limits<uint64_t>::max(),
			ImageAvailable[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

		// Start point of render pass in pixels
		RenderPassBeginInfo.renderArea.extent = MainViewport.ViewportSwapchain.SwapExtent;				// Size of region to run render pass on (starting at offset)
		RenderPassBeginInfo.framebuffer = MainViewport.SwapchainFramebuffers[ImageIndex];

		VkResult Result = vkBeginCommandBuffer(MainViewport.CommandBuffers[ImageIndex], &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			assert(false);
		}

		vkCmdBeginRenderPass(MainViewport.CommandBuffers[ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// TEST Skip terrain pipeline
		vkCmdNextSubpass(MainViewport.CommandBuffers[ImageIndex], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.EntityPass.Pipeline);

		// TODO: Support rework to not create identical index buffers
		for (uint32_t j = 0; j < DrawableObjectsCount; ++j)
		{
			VkBuffer VertexBuffers[] = { DrawableObjects[j].VertexBuffer.Buffer };					// Buffers to bind
			VkDeviceSize Offsets[] = { 0 };												// Offsets into buffers being bound
			vkCmdBindVertexBuffers(MainViewport.CommandBuffers[ImageIndex], 0, 1, VertexBuffers, Offsets);
			vkCmdBindIndexBuffer(MainViewport.CommandBuffers[ImageIndex], DrawableObjects[j].IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

			// UNIFORM BUFFER SETUP CODE
			//uint32_t DynamicOffset = ModelUniformAlignment * j;

			vkCmdPushConstants(MainViewport.CommandBuffers[ImageIndex], MainPass.EntityPass.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
				0, sizeof(Model), &DrawableObjects[j].Model);

			// Todo: do not record textureId on each frame?
			const uint32_t DescriptorSetGroupCount = 2;
			VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] = { MainPass.EntityPass.EntitySets[ImageIndex],
				SamplerDescriptorSets[DrawableObjects[j].TextureId] };

			vkCmdBindDescriptorSets(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.EntityPass.PipelineLayout,
				0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

			// Execute pipeline
			//vkCmdDraw(MainViewport.CommandBuffers[i], DrawableObject.VerticesCount, 1, 0, 0);
			vkCmdDrawIndexed(MainViewport.CommandBuffers[ImageIndex], DrawableObjects[j].IndicesCount, 1, 0, 0, 0);
		}

		vkCmdNextSubpass(MainViewport.CommandBuffers[ImageIndex], VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.DeferredPass.Pipeline);
		vkCmdBindDescriptorSets(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.DeferredPass.PipelineLayout,
			0, 1, &MainPass.DeferredPass.DeferredSets[ImageIndex], 0, nullptr);
		vkCmdDraw(MainViewport.CommandBuffers[ImageIndex], 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass

		// End Render Pass
		vkCmdEndRenderPass(MainViewport.CommandBuffers[ImageIndex]);

		Result = vkEndCommandBuffer(MainViewport.CommandBuffers[ImageIndex]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			return false;
		}
		// Function end RecordCommands

		//UpdateUniformBuffer
		void* Data;
		vkMapMemory(LogicalDevice, MainPass.VpUniformBuffers[ImageIndex].Memory, 0, sizeof(UboViewProjection),
			0, &Data);
		std::memcpy(Data, &MainViewport.ViewProjection, sizeof(UboViewProjection));
		vkUnmapMemory(LogicalDevice, MainPass.VpUniformBuffers[ImageIndex].Memory);

		// Submit command buffer to queue
		VkSubmitInfo submitInfo = { };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		submitInfo.pWaitDstStageMask = waitStages;						// Stages to check semaphores at
		submitInfo.commandBufferCount = 1;								// Number of command buffers to submit
		submitInfo.signalSemaphoreCount = 1;							// Number of semaphores to signal
		submitInfo.pWaitSemaphores = &ImageAvailable[CurrentFrame];				// List of semaphores to wait on
		submitInfo.pCommandBuffers = &MainViewport.CommandBuffers[ImageIndex];		// Command buffer to submit
		submitInfo.pSignalSemaphores = &RenderFinished[CurrentFrame];	// Semaphores to signal when command buffer finishes
		Result = vkQueueSubmit(GraphicsQueue, 1, &submitInfo, DrawFences[CurrentFrame]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueueSubmit result is {}", static_cast<int>(Result));
			return false;
		}

		// -- PRESENT RENDERED IMAGE TO SCREEN --
		VkPresentInfoKHR presentInfo = { };
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		presentInfo.swapchainCount = 1;											// Number of swapchains to present to
		presentInfo.pWaitSemaphores = &RenderFinished[CurrentFrame];			// Semaphores to wait on
		presentInfo.pSwapchains = &MainViewport.ViewportSwapchain.VulkanSwapchain;									// Swapchains to present images to
		presentInfo.pImageIndices = &ImageIndex;								// Index of images in swapchains to present
		Result = vkQueuePresentKHR(PresentationQueue, &presentInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueuePresentKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		CurrentFrame = (CurrentFrame + 1) % MaxFrameDraws;

		return true;
	}

	void VulkanRenderingSystem::DeinitViewport(ViewportInstance* Viewport)
	{
		vkDeviceWaitIdle(LogicalDevice);

		for (uint32_t i = 0; i < Viewport->ViewportSwapchain.ImagesCount; ++i)
		{
			vkDestroyFramebuffer(LogicalDevice, Viewport->SwapchainFramebuffers[i], nullptr);
			// UNIFORM BUFFER SETUP CODE
			//Core::DestroyGenericBuffer(RenderInstance, ModelDynamicUniformBuffers[i]);
		}

		SwapchainInstance::DestroySwapchainInstance(LogicalDevice, Viewport->ViewportSwapchain);

		Util::Memory::Deallocate(Viewport->SwapchainFramebuffers);
		Util::Memory::Deallocate(Viewport->CommandBuffers);
	}

	// TODO: CHECK
	void VulkanRenderingSystem::CreateImageBuffer(stbi_uc* TextureData, int Width, int Height, VkDeviceSize ImageSize)
	{
		assert(TextureImagesCount < MaxTextures);

		GPUBuffer StagingBuffer = GPUBuffer::CreateGPUBuffer(Device.PhysicalDevice, LogicalDevice, ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* Data;
		vkMapMemory(LogicalDevice, StagingBuffer.Memory, 0, ImageSize, 0, &Data);
		memcpy(Data, TextureData, static_cast<size_t>(ImageSize));
		vkUnmapMemory(LogicalDevice, StagingBuffer.Memory);

		ImageBuffer ImageBuffeObject;
		ImageBuffeObject.Image = CreateImage(Device.PhysicalDevice, LogicalDevice, Width, Height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
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
		GPUBuffer::DestroyGPUBuffer(LogicalDevice, StagingBuffer);

		//CreateImageView
		ImageBuffeObject.ImageView = CreateImageView(LogicalDevice, ImageBuffeObject.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		//CreateTextureDescriptor
		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = SamplerDescriptorPool;
		SetAllocInfo.descriptorSetCount = 1;
		SetAllocInfo.pSetLayouts = &MainPass.EntityPass.SamplerSetLayout;

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

	void VulkanRenderingSystem::InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport,
		VkDescriptorPool DescriptorPool, SwapchainInstance SwapInstance, ImageBuffer* ColorBuffers, ImageBuffer* DepthBuffers)
	{
		OutViewport->Window = Window;
		OutViewport->Surface = Surface;
		OutViewport->ViewportSwapchain = SwapInstance;

		OutViewport->SwapchainFramebuffers = static_cast<VkFramebuffer*>(Util::Memory::Allocate(OutViewport->ViewportSwapchain.ImagesCount * sizeof(VkFramebuffer)));

		// Function CreateFrameBuffers
		// Create a framebuffer for each swap chain image
		for (uint32_t i = 0; i < OutViewport->ViewportSwapchain.ImagesCount; i++)
		{
			const uint32_t AttachmentsCount = 3;
			VkImageView Attachments[AttachmentsCount] = {
				OutViewport->ViewportSwapchain.ImageViews[i],
				// Todo: do not forget about position in array AttachmentDescriptions
				ColorBuffers[i].ImageView,
				DepthBuffers[i].ImageView
			};

			VkFramebufferCreateInfo FramebufferCreateInfo = { };
			FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			FramebufferCreateInfo.renderPass = MainPass.RenderPass;								// Render Pass layout the Framebuffer will be used with
			FramebufferCreateInfo.attachmentCount = AttachmentsCount;
			FramebufferCreateInfo.pAttachments = Attachments;							// List of attachments (1:1 with Render Pass)
			FramebufferCreateInfo.width = OutViewport->ViewportSwapchain.SwapExtent.width;								// Framebuffer width
			FramebufferCreateInfo.height = OutViewport->ViewportSwapchain.SwapExtent.height;							// Framebuffer height
			FramebufferCreateInfo.layers = 1;											// Framebuffer layers

			VkResult Result = vkCreateFramebuffer(LogicalDevice, &FramebufferCreateInfo, nullptr, &OutViewport->SwapchainFramebuffers[i]);
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
		CommandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(OutViewport->ViewportSwapchain.ImagesCount);

		// Allocate command buffers and place handles in array of buffers
		OutViewport->CommandBuffers = static_cast<VkCommandBuffer*>(Util::Memory::Allocate(OutViewport->ViewportSwapchain.ImagesCount * sizeof(VkCommandBuffer)));
		VkResult Result = vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, OutViewport->CommandBuffers);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateCommandBuffers result is {}", static_cast<int>(Result));
			assert(false);
		}
		// Function end CreateCommandBuffers
	}
}
