#include "VulkanRenderingSystem.h"

#include <algorithm>

#include "VulkanHelper.h"
#include "Util/Util.h"

namespace Core
{
	bool VulkanRenderingSystem::Init(GLFWwindow* Window, const RenderConfig& InConfig)
	{
		SetConfig(InConfig);

		assert(Config.ShadersCount == ShaderNames::ShadersCount);

		const char* ValidationLayers[] = {
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_monitor"
		};
		const u32 ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

		const char* ValidationExtensions[] = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		const u32 ValidationExtensionsSize = Util::EnableValidationLayers ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

		const char* DeviceExtensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		const u32 DeviceExtensionsSize = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

		Memory::FrameArray<VkExtensionProperties> AvailableExtensions = GetAvailableExtensionProperties();
		Memory::FrameArray<const char*> RequiredExtensions = GetRequiredInstanceExtensions(ValidationExtensions, ValidationExtensionsSize);

		if (!CheckRequiredInstanceExtensionsSupport(AvailableExtensions.Pointer.Data, AvailableExtensions.Count,
			RequiredExtensions.Pointer.Data, RequiredExtensions.Count))
		{
			return false;
		}

		if (Util::EnableValidationLayers)
		{
			Memory::FrameArray<VkLayerProperties> LayerPropertiesData = GetAvailableInstanceLayerProperties();

			if (!CheckValidationLayersSupport(LayerPropertiesData.Pointer.Data, LayerPropertiesData.Count,
				ValidationLayers, ValidationLayersSize))
			{
				return false;
			}
		}

		Instance = MainInstance::CreateMainInstance(RequiredExtensions.Pointer.Data, RequiredExtensions.Count,
			Util::EnableValidationLayers, ValidationLayers, ValidationLayersSize);

		VkSurfaceKHR Surface = nullptr;
		if (glfwCreateWindowSurface(Instance.VulkanInstance, Window, nullptr, &Surface) != VK_SUCCESS)
		{
			Util::Log().GlfwLogError();
			assert(false);
		}
		
		Device.Init(Instance.VulkanInstance, Surface, DeviceExtensions, DeviceExtensionsSize);

		LogicalDevice = CreateLogicalDevice(Device.Indices, DeviceExtensions, DeviceExtensionsSize);

		VkSurfaceFormatKHR SurfaceFormat = GetBestSurfaceFormat(Surface);

		const u32 FormatPrioritySize = 3;
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
		CreateCommandPool(LogicalDevice, Device.Indices.GraphicsFamily);

		SwapchainInstance SwapInstance1 = SwapchainInstance::CreateSwapchainInstance(Device.PhysicalDevice, Device.Indices,
			LogicalDevice, Surface, SurfaceFormat, Extent1);

		const u32 PoolSizeCount = 9;
		auto TotalPassPoolSizes = Memory::FramePointer<VkDescriptorPoolSize>::Create(PoolSizeCount);
		u32 TotalDescriptorLayouts = 20;
		// Layout 1
		TotalPassPoolSizes[0] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance1.ImagesCount };
		// Layout 2
		TotalPassPoolSizes[1] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance1.ImagesCount };
		
		TotalPassPoolSizes[2] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance1.ImagesCount };
		
		TotalPassPoolSizes[3] = { .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = SwapInstance1.ImagesCount };
		// Layout 3
		TotalPassPoolSizes[4] = { .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = SwapInstance1.ImagesCount };
		TotalPassPoolSizes[5] = { .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = SwapInstance1.ImagesCount };
		// Textures
		TotalPassPoolSizes[6] = { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = Config.MaxTextures };

		TotalPassPoolSizes[7] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance1.ImagesCount };
		TotalPassPoolSizes[8] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance1.ImagesCount };

		u32 TotalDescriptorCount = TotalDescriptorLayouts * SwapInstance1.ImagesCount;
		TotalDescriptorCount += Config.MaxTextures;

		MemorySourceDevice MemoryDevice;
		MemoryDevice.PhysicalDevice = Device.PhysicalDevice;
		MemoryDevice.LogicalDevice = LogicalDevice;
		MemoryDevice.TransferCommandPool = GraphicsCommandPool;
		MemoryDevice.TransferQueue = GraphicsQueue;

		auto VulkanMemoryManagement = VulkanMemoryManagementSystem::Get();

		VulkanMemoryManagement->Init(MemoryDevice);
		MainPool = VulkanMemoryManagement->AllocateDescriptorPool(TotalPassPoolSizes.Data, PoolSizeCount, TotalDescriptorCount);

		ShaderInput ShaderInputs[ShaderNames::ShadersCount];
		for (u32 i = 0; i < Config.ShadersCount; ++i)
		{
			ShaderInputs[i].ShaderName = Config.RenderShaders[i].Name;
			CreateShader(LogicalDevice, Config.RenderShaders[i].Code, Config.RenderShaders[i].CodeSize, ShaderInputs[i].Module);
		}

		VertexBuffer = VulkanMemoryManagement->CreateBuffer(Mb64, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		IndexBuffer = VulkanMemoryManagement->CreateBuffer(Mb64,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		MainPass.CreateVulkanPass(LogicalDevice, ColorFormat, DepthFormat, SurfaceFormat);
		MainPass.SetupPushConstants();
		MainPass.CreateSamplerSetLayout(LogicalDevice);
		MainPass.CreateTerrainSetLayout(LogicalDevice);
		MainPass.CreateEntitySetLayout(LogicalDevice);
		MainPass.CreateDeferredSetLayout(LogicalDevice);
		MainPass.CreatePipelineLayouts(LogicalDevice);
		MainPass.CreatePipelines(LogicalDevice, Extent1, ShaderInputs, ShaderNames::ShadersCount);
		MainPass.CreateAttachments(Device.PhysicalDevice, LogicalDevice, SwapInstance1.ImagesCount, Extent1, DepthFormat, ColorFormat);
		MainPass.CreateUniformBuffers(Device.PhysicalDevice, LogicalDevice, SwapInstance1.ImagesCount);
		MainPass.CreateSets(MainPool, LogicalDevice, SwapInstance1.ImagesCount);

		for (int i = 0; i < ShaderNames::ShadersCount; ++i)
		{
			vkDestroyShaderModule(LogicalDevice, ShaderInputs[i].Module, nullptr);
		}

		InitViewport(Window, Surface, &MainViewport, SwapInstance1, MainPass.ColorBufferViews, MainPass.DepthBufferViews);

		TextureUnit.TextureSampler = CreateTextureSampler();

		return true;
	}

	void VulkanRenderingSystem::DeInit()
	{
		vkDeviceWaitIdle(LogicalDevice);
		
		DestroySynchronisation();

		vkDestroySampler(LogicalDevice, TextureUnit.TextureSampler, nullptr);

		for (u32 i = 0; i < TextureUnit.ImagesViewsCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, TextureUnit.ImageViews[i], nullptr);
		}

		for (u32 i = 0; i < TextureUnit.ImageArraysCount; ++i)
		{
			VulkanMemoryManagementSystem::Get()->DestroyImageBuffer(TextureUnit.Images[i]);
		}

		vkDestroyDescriptorPool(LogicalDevice, MainPool, nullptr);

		VulkanMemoryManagementSystem::Get()->DestroyBuffer(VertexBuffer);
		VulkanMemoryManagementSystem::Get()->DestroyBuffer(IndexBuffer);
		VulkanMemoryManagementSystem::Get()->Deinit();

		vkDestroyCommandPool(LogicalDevice, GraphicsCommandPool, nullptr);
		MainPass.ClearResources(LogicalDevice, MainViewport.ViewportSwapchain.ImagesCount);

		DeinitViewport(&MainViewport);

		vkDestroyDevice(LogicalDevice, nullptr);

		MainInstance::DestroyMainInstance(Instance);
	}

	void VulkanRenderingSystem::LoadTextures(TextureArrayInfo* ArrayInfos, u32 InfosCount, u32* ResourceIndices)
	{
		assert(InfosCount > 0);
		assert(TextureUnit.ImageArraysCount == 0);
		assert(InfosCount < MAX_IMAGES);

		TextureUnit.ImageArraysCount = InfosCount;

		auto System = Memory::MemoryManagementSystem::Get();
		auto Barriers = System->FrameAlloc<VkImageMemoryBarrier>(TextureUnit.ImageArraysCount);

		auto VulkanMemorySystem = VulkanMemoryManagementSystem::Get();

		for (u32 i = 0; i < TextureUnit.ImageArraysCount; ++i)
		{
			VkImageCreateInfo ImageCreateInfo = { };
			ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D; // Type of image (1D, 2D, or 3D)
			ImageCreateInfo.extent.width = ArrayInfos[i].Width;
			ImageCreateInfo.extent.height = ArrayInfos[i].Height;
			ImageCreateInfo.extent.depth = 1; // Depth of image (just 1, no 3D aspect)
			ImageCreateInfo.mipLevels = 1;
			ImageCreateInfo.arrayLayers = ArrayInfos[i].LayersCount;
			ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // Format type of image
			ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // How image data should be "tiled" (arranged for optimal reading)
			ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout of image data on creation
			ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // Bit flags defining what image will be used for
			ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; // Number of samples for multi-sampling
			ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Whether image can be shared between queues

			TextureUnit.Images[i] = VulkanMemoryManagementSystem::Get()->CreateImageBuffer(&ImageCreateInfo);

			Barriers[i] = { };
			Barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			Barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;									// Layout to transition from
			Barriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;									// Layout to transition to
			Barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			Barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			Barriers[i].image = TextureUnit.Images[i].Image;											// Image being accessed and modified as part of barrier
			Barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
			Barriers[i].subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
			Barriers[i].subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from baseMipLevel
			Barriers[i].subresourceRange.baseArrayLayer = 0;						// First layer to start alterations on
			Barriers[i].subresourceRange.layerCount = ArrayInfos[i].LayersCount; // Number of layers to alter starting from baseArrayLayer

			Barriers[i].srcAccessMask = 0;								// Memory access stage transition must after...
			Barriers[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// Memory access stage transition must before...

			TextureUnit.ImagesViewsCount += ArrayInfos[i].LayersCount;
		}

		// Todo: Create beginCommandBuffer function?
		VkCommandBuffer CommandBuffer;

		VkCommandBufferAllocateInfo AllocInfo = { };
		AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = GraphicsCommandPool;
		AllocInfo.commandBufferCount = 1;

		VkCommandBufferBeginInfo BeginInfo = { };
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkPipelineStageFlags SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffer;

		vkAllocateCommandBuffers(LogicalDevice, &AllocInfo, &CommandBuffer);
		vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

		vkCmdPipelineBarrier(
			CommandBuffer,
			SrcStage, DstStage,		// Pipeline stages (match to src and dst AccessMasks)
			0,						// Dependency flags
			0, nullptr,				// Memory Barrier count + data
			0, nullptr,				// Buffer Memory Barrier count + data
			TextureUnit.ImageArraysCount, Barriers	// Image Memory Barrier count + data
		);

		vkEndCommandBuffer(CommandBuffer);

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		for (u32 i = 0; i < TextureUnit.ImageArraysCount; ++i)
		{
			VkMemoryRequirements TextureMemoryRequirements;
			vkGetImageMemoryRequirements(LogicalDevice, TextureUnit.Images[i].Image, &TextureMemoryRequirements);

			VulkanMemorySystem->CopyDataToImage(TextureUnit.Images[i].Image, ArrayInfos[i].Width, ArrayInfos[i].Height,
				TextureMemoryRequirements.size, ArrayInfos[i].LayersCount, ArrayInfos[i].Data);

			Barriers[i].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // Layout to transition from
			Barriers[i].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			Barriers[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			Barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}

		SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
		vkCmdPipelineBarrier(CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, TextureUnit.ImageArraysCount, Barriers);
		vkEndCommandBuffer(CommandBuffer);

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		vkFreeCommandBuffers(LogicalDevice, GraphicsCommandPool, 1, &CommandBuffer);

		auto WriteData = System->FrameAlloc<VkWriteDescriptorSet>(TextureUnit.ImagesViewsCount);
		auto ImageInfos = System->FrameAlloc<VkDescriptorImageInfo>(TextureUnit.ImagesViewsCount);
		auto layouts = System->FrameAlloc<VkDescriptorSetLayout>(TextureUnit.ImagesViewsCount);

		for (u32 i = 0; i < TextureUnit.ImagesViewsCount; ++i)
		{
			layouts[i] = MainPass.SamplerSetLayout;
		}

		VulkanMemoryManagementSystem::Get()->AllocateSets(MainPool, layouts,
			TextureUnit.ImagesViewsCount, TextureUnit.SamplerDescriptorSets);

		u32 DescriptorIndex = 0;
		for (u32 i = 0; i < TextureUnit.ImageArraysCount; ++i)
		{
			for (u32 j = 0; j < ArrayInfos[i].LayersCount; ++j)
			{
				TextureUnit.ImageViews[DescriptorIndex] = CreateImageView(LogicalDevice, TextureUnit.Images[i].Image,
					VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

				ImageInfos[DescriptorIndex] = { };
				ImageInfos[DescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				ImageInfos[DescriptorIndex].imageView = TextureUnit.ImageViews[DescriptorIndex];
				ImageInfos[DescriptorIndex].sampler = TextureUnit.TextureSampler;

				WriteData[DescriptorIndex] = { };
				WriteData[DescriptorIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				WriteData[DescriptorIndex].dstSet = TextureUnit.SamplerDescriptorSets[DescriptorIndex];
				WriteData[DescriptorIndex].dstBinding = 0;
				WriteData[DescriptorIndex].dstArrayElement = 0;
				WriteData[DescriptorIndex].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				WriteData[DescriptorIndex].descriptorCount = 1;
				WriteData[DescriptorIndex].pImageInfo = &ImageInfos[DescriptorIndex];
				
				ResourceIndices[DescriptorIndex] = DescriptorIndex;
				++DescriptorIndex;
			}
		}

		vkUpdateDescriptorSets(LogicalDevice, TextureUnit.ImagesViewsCount, WriteData, 0, nullptr);
	}

	bool VulkanRenderingSystem::CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
		const char** RequiredExtensions, u32 RequiredExtensionsCount)
	{
		for (u32 i = 0; i < RequiredExtensionsCount; ++i)
		{
			bool IsExtensionSupported = false;
			for (u32 j = 0; j < AvailableExtensionsCount; ++j)
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

	bool VulkanRenderingSystem::CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
		const char** ValidationLeyersToCheck, u32 ValidationLeyersToCheckSize)
	{
		for (u32 i = 0; i < ValidationLeyersToCheckSize; ++i)
		{
			bool IsLayerAvalible = false;
			for (u32 j = 0; j < PropertiesSize; ++j)
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
		if (SurfaceCapabilities.currentExtent.width != std::numeric_limits<u32>::max())
		{
			return SurfaceCapabilities.currentExtent;
		}
		else
		{
			int Width;
			int Height;
			glfwGetFramebufferSize(Window, &Width, &Height);

			Width = std::clamp(static_cast<u32>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
			Height = std::clamp(static_cast<u32>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

			return { static_cast<u32>(Width), static_cast<u32>(Height) };
		}
	}

	Memory::FrameArray<VkExtensionProperties> VulkanRenderingSystem::GetAvailableExtensionProperties()
	{
		u32 Count;
		const VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceExtensionProperties result is {}", static_cast<int>(Result));
			assert(false);
		}

		auto Data = Memory::FrameArray<VkExtensionProperties>::Create(Count);
		vkEnumerateInstanceExtensionProperties(nullptr, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkLayerProperties> VulkanRenderingSystem::GetAvailableInstanceLayerProperties()
	{
		u32 Count;
		const VkResult Result = vkEnumerateInstanceLayerProperties(&Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateInstanceLayerProperties result is {}", static_cast<int>(Result));
			assert(false);
		}

		auto Data = Memory::FrameArray<VkLayerProperties>::Create(Count);
		vkEnumerateInstanceLayerProperties(&Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<const char*> VulkanRenderingSystem::GetRequiredInstanceExtensions(const char** ValidationExtensions,
		u32 ValidationExtensionsCount)
	{
		u32 RequiredExtensionsCount = 0;
		const char** RequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&RequiredExtensionsCount);
		if (RequiredExtensionsCount == 0)
		{
			Util::Log().GlfwLogError();
			assert(false);
		}

		auto Data = Memory::FrameArray<const char*>::Create(RequiredExtensionsCount + ValidationExtensionsCount);

		for (u32 i = 0; i < RequiredExtensionsCount; ++i)
		{
			Data[i] = RequiredInstanceExtensions[i];
			Util::Log().Info("Requested {} extension", Data[i]);
		}

		for (u32 i = 0; i < ValidationExtensionsCount; ++i)
		{
			Data[i + RequiredExtensionsCount] = ValidationExtensions[i];
			Util::Log().Info("Requested {} extension", Data[i]);
		}

		return Data;
	}

	Memory::FrameArray<VkSurfaceFormatKHR> VulkanRenderingSystem::GetSurfaceFormats(VkSurfaceKHR Surface)
	{
		u32 Count;
		const VkResult Result = vkGetPhysicalDeviceSurfaceFormatsKHR(Device.PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfaceFormatsKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		auto Data = Memory::FrameArray<VkSurfaceFormatKHR>::Create(Count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device.PhysicalDevice, Surface, &Count, Data.Pointer.Data);

		return Data;
	}

	VkDevice VulkanRenderingSystem::CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		u32 DeviceExtensionsSize)
	{
		const float Priority = 1.0f;

		// One family can support graphics and presentation
		// In that case create multiple VkDeviceQueueCreateInfo
		VkDeviceQueueCreateInfo QueueCreateInfos[2] = { };
		u32 FamilyIndicesSize = 1;

		QueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		QueueCreateInfos[0].queueFamilyIndex = static_cast<u32>(Indices.GraphicsFamily);
		QueueCreateInfos[0].queueCount = 1;
		QueueCreateInfos[0].pQueuePriorities = &Priority;

		if (Indices.GraphicsFamily != Indices.PresentationFamily)
		{
			QueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfos[1].queueFamilyIndex = static_cast<u32>(Indices.PresentationFamily);
			QueueCreateInfos[1].queueCount = 1;
			QueueCreateInfos[1].pQueuePriorities = &Priority;

			++FamilyIndicesSize;
		}

		VkPhysicalDeviceFeatures DeviceFeatures = { };
		DeviceFeatures.fillModeNonSolid = VK_TRUE; // Todo: get from configs
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

	VkSurfaceFormatKHR VulkanRenderingSystem::GetBestSurfaceFormat(VkSurfaceKHR Surface)
	{
		Memory::FrameArray<VkSurfaceFormatKHR> FormatsData = GetSurfaceFormats(Surface);

		VkSurfaceFormatKHR Format = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };

		// All formats available
		if (FormatsData.Count == 1 && FormatsData[0].format == VK_FORMAT_UNDEFINED)
		{
			Format = { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
		else
		{
			for (u32 i = 0; i < FormatsData.Count; ++i)
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
		vkGetDeviceQueue(LogicalDevice, static_cast<u32>(Device.Indices.GraphicsFamily), 0, &GraphicsQueue);
		vkGetDeviceQueue(LogicalDevice, static_cast<u32>(Device.Indices.PresentationFamily), 0, &PresentationQueue);

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

	void VulkanRenderingSystem::CreateDrawEntities(Mesh* Meshes, u32 MeshesCount, DrawEntity* OutEntities)
	{
		auto VulkanMemorySystem = VulkanMemoryManagementSystem::Get();
		VkDeviceSize TotalVertexSize = 0;
		VkDeviceSize TotalIndexSize = 0;

		for (u32 i = 0; i < MeshesCount; ++i)
		{
			const VkDeviceSize MeshVerticesSize = sizeof(EntityVertex) * Meshes[i].MeshVerticesCount;
			const VkDeviceSize MeshIndicesSize = sizeof(u32) * Meshes[i].MeshIndicesCount;

			OutEntities[i].VertexOffset = TotalVertexSize + VertexBufferOffset;
			OutEntities[i].IndexOffset = TotalIndexSize + IndexBufferOffset;

			TotalVertexSize += VulkanMemorySystem->CalculateBufferAlignedSize(MeshVerticesSize);
			TotalIndexSize += VulkanMemorySystem->CalculateBufferAlignedSize(MeshIndicesSize);
		}

		EntityVertex* AllVertices = Memory::MemoryManagementSystem::Allocate<EntityVertex>(TotalVertexSize);
		u32* AllIndices = Memory::MemoryManagementSystem::Allocate<u32>(TotalIndexSize);

		char* VertexCopyPointer = reinterpret_cast<char*>(AllVertices);
		char* IndexCopyPointer = reinterpret_cast<char*>(AllIndices);

		for (u32 i = 0; i < MeshesCount; ++i)
		{
			const VkDeviceSize MeshVerticesSize = sizeof(EntityVertex) * Meshes[i].MeshVerticesCount;
			const VkDeviceSize MeshIndicesSize = sizeof(u32) * Meshes[i].MeshIndicesCount;

			std::memcpy(VertexCopyPointer, Meshes[i].MeshVertices, MeshVerticesSize);
			std::memcpy(IndexCopyPointer, Meshes[i].MeshIndices, MeshIndicesSize);

			VertexCopyPointer += VulkanMemorySystem->CalculateBufferAlignedSize(MeshVerticesSize);
			IndexCopyPointer += VulkanMemorySystem->CalculateBufferAlignedSize(MeshIndicesSize);

			OutEntities[i].Model = Meshes[i].Model;
			OutEntities[i].IndicesCount = Meshes[i].MeshIndicesCount;
		}

		VulkanMemorySystem->CopyDataToBuffer(VertexBuffer.Buffer, VertexBufferOffset, TotalVertexSize, AllVertices);
		VertexBufferOffset += TotalVertexSize;
		VulkanMemorySystem->CopyDataToBuffer(IndexBuffer.Buffer, IndexBufferOffset, TotalIndexSize, AllIndices);
		IndexBufferOffset += TotalIndexSize;

		Memory::MemoryManagementSystem::Deallocate(AllVertices);
		Memory::MemoryManagementSystem::Deallocate(AllIndices);
	}

	void VulkanRenderingSystem::CreateTerrainIndices(u32* Indices, u32 IndicesCount)
	{
		assert(TerrainIndicesCount == 0);

		auto VulkanMemorySystem = VulkanMemoryManagementSystem::Get();

		TerrainIndexOffset = IndexBufferOffset;
		TerrainIndicesCount = IndicesCount;

		const VkDeviceSize MeshIndexSize = sizeof(u32) * TerrainIndicesCount;

		VulkanMemorySystem->CopyDataToBuffer(IndexBuffer.Buffer, IndexBufferOffset, MeshIndexSize, Indices);
		IndexBufferOffset += VulkanMemorySystem->CalculateBufferAlignedSize(MeshIndexSize);
	}

	void VulkanRenderingSystem::CreateTerrainDrawEntity(TerrainVertex* TerrainVertices, u32 TerrainVerticesCount, DrawTerrainEntity& OutTerrain)
	{
		auto VulkanMemorySystem = VulkanMemoryManagementSystem::Get();
		OutTerrain.VertexOffset = VertexBufferOffset;

		const VkDeviceSize MeshVerticesSize = sizeof(TerrainVertex) * TerrainVerticesCount;

		VulkanMemorySystem->CopyDataToBuffer(VertexBuffer.Buffer, VertexBufferOffset, MeshVerticesSize, TerrainVertices);
		VertexBufferOffset += VulkanMemorySystem->CalculateBufferAlignedSize(MeshVerticesSize);
	}

	void VulkanRenderingSystem::CreateSynchronisation()
	{
		VkSemaphoreCreateInfo SemaphoreCreateInfo = { };
		SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// Fence creation information
		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (u64 i = 0; i < MAX_DRAW_FRAMES; i++)
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
		for (u64 i = 0; i < MAX_DRAW_FRAMES; i++)
		{
			vkDestroySemaphore(LogicalDevice, RenderFinished[i], nullptr);
			vkDestroySemaphore(LogicalDevice, ImageAvailable[i], nullptr);
			vkDestroyFence(LogicalDevice, DrawFences[i], nullptr);
		}
	}

	void VulkanRenderingSystem::Draw(const DrawScene& Scene)
	{
		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		const u32 ClearValuesSize = 3;
		VkClearValue ClearValues[ClearValuesSize];
		// Todo: do not forget about position in array AttachmentDescriptions
		ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		ClearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		ClearValues[2].depthStencil.depth = 1.0f;

		VkRenderPassBeginInfo RenderPassBeginInfo = { };
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.renderPass = MainPass.RenderPass; // Render Pass to begin
		RenderPassBeginInfo.renderArea.offset = { 0, 0 };
		RenderPassBeginInfo.pClearValues = ClearValues;
		RenderPassBeginInfo.clearValueCount = ClearValuesSize;

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		vkWaitForFences(LogicalDevice, 1, &DrawFences[CurrentFrame], VK_TRUE, std::numeric_limits<u64>::max());
		vkResetFences(LogicalDevice, 1, &DrawFences[CurrentFrame]);

		// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
		u32 ImageIndex;
		vkAcquireNextImageKHR(LogicalDevice, MainViewport.ViewportSwapchain.VulkanSwapchain, std::numeric_limits<u64>::max(),
			ImageAvailable[CurrentFrame], VK_NULL_HANDLE, &ImageIndex);

		// Start point of render pass in pixels
		RenderPassBeginInfo.renderArea.extent = MainViewport.ViewportSwapchain.SwapExtent; // Size of region to run render pass on (starting at offset)
		RenderPassBeginInfo.framebuffer = MainViewport.SwapchainFramebuffers[ImageIndex];

		VkResult Result = vkBeginCommandBuffer(MainViewport.CommandBuffers[ImageIndex], &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			assert(false);
		}

		vkCmdBeginRenderPass(MainViewport.CommandBuffers[ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		auto VulkanMemoryManagement = VulkanMemoryManagementSystem::Get();

		{
			vkCmdBindPipeline(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.TerrainPass.Pipeline);

			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			VkBuffer TerrainVertexBuffers[] = { VertexBuffer.Buffer };
			VkDeviceSize TerrainBuffersOffsets[] = { Scene.DrawTerrainEntities[0].VertexOffset };

			const u32 TerrainDescriptorSetGroupCount = 2;
			VkDescriptorSet TerrainDescriptorSetGroup[TerrainDescriptorSetGroupCount] = {
				MainPass.TerrainPass.TerrainSets[ImageIndex], TextureUnit.SamplerDescriptorSets[0] };

			vkCmdBindDescriptorSets(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.TerrainPass.PipelineLayout,
				0, TerrainDescriptorSetGroupCount, TerrainDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

			vkCmdBindVertexBuffers(MainViewport.CommandBuffers[ImageIndex], 0, 1, TerrainVertexBuffers, TerrainBuffersOffsets);
			vkCmdBindIndexBuffer(MainViewport.CommandBuffers[ImageIndex],
				IndexBuffer.Buffer, TerrainIndexOffset, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(MainViewport.CommandBuffers[ImageIndex], TerrainIndicesCount, 1, 0, 0, 0);
		}

		vkCmdNextSubpass(MainViewport.CommandBuffers[ImageIndex], VK_SUBPASS_CONTENTS_INLINE);

		{
			vkCmdBindPipeline(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.EntityPass.Pipeline);

			// TODO: Support rework to not create identical index buffers
			for (u32 j = 0; j < Scene.DrawEntitiesCount; ++j)
			{
				VkBuffer VertexBuffers[] = { VertexBuffer.Buffer };
				VkDeviceSize Offsets[] = { Scene.DrawEntities[j].VertexOffset };

				// Todo: do not record textureId on each frame?
				const u32 DescriptorSetGroupCount = 4;
				VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] = 
				{
					MainPass.EntityPass.EntitySets[ImageIndex],
					TextureUnit.SamplerDescriptorSets[Scene.DrawEntities[j].TextureId],
					MainPass.LightingSets[ImageIndex],
					MainPass.MaterialSet
				};

				vkCmdPushConstants(MainViewport.CommandBuffers[ImageIndex], MainPass.EntityPass.PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
					0, sizeof(Model), &Scene.DrawEntities[j].Model);
				vkCmdBindDescriptorSets(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.EntityPass.PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(MainViewport.CommandBuffers[ImageIndex], 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(MainViewport.CommandBuffers[ImageIndex], IndexBuffer.Buffer,
					Scene.DrawEntities[j].IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(MainViewport.CommandBuffers[ImageIndex], Scene.DrawEntities[j].IndicesCount, 1, 0, 0, 0);
			}
		}

		vkCmdNextSubpass(MainViewport.CommandBuffers[ImageIndex], VK_SUBPASS_CONTENTS_INLINE);

		{
			vkCmdBindPipeline(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.DeferredPass.Pipeline);

			vkCmdBindDescriptorSets(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.DeferredPass.PipelineLayout,
				0, 1, &MainPass.DeferredPass.DeferredSets[ImageIndex], 0, nullptr);

			vkCmdDraw(MainViewport.CommandBuffers[ImageIndex], 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass
		}

		vkCmdEndRenderPass(MainViewport.CommandBuffers[ImageIndex]);

		Result = vkEndCommandBuffer(MainViewport.CommandBuffers[ImageIndex]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
		}

		//UpdateUniformBuffer
		VulkanMemoryManagementSystem::Get()->CopyDataToMemory(MainPass.VpUniformBuffers[ImageIndex].Memory, 0,
			sizeof(UboViewProjection), &Scene.ViewProjection);

		DrawPointLightEntity TestData = { glm::vec3(1.0f, 1.0f, 1.0f)};
		TestData.Position = glm::vec3(0.0f, 0.0f, 10.0f);
		TestData.Ambient = glm::vec3(0.2f, 0.2f, 0.2f);
		TestData.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		TestData.Specular = glm::vec3(1.0f, 1.0f, 1.0f);

		VulkanMemoryManagementSystem::Get()->CopyDataToMemory(MainPass.LightBuffers[ImageIndex].Memory, 0,
			sizeof(DrawPointLightEntity), &TestData);

		Material Mat;
		Mat.Ambient = glm::vec3(0.24725f, 0.1995f, 0.0745f);
		Mat.Diffuse = glm::vec3(0.75164f, 0.60648f, 0.22648f);
		Mat.Specular = glm::vec3(0.628281f, 0.555802f, 0.366065f);
		Mat.Shininess = 0.4f;

		VulkanMemoryManagementSystem::Get()->CopyDataToMemory(MainPass.MaterialBuffer.Memory, 0,
			sizeof(Material), &Mat);

		// Submit command buffer to queue
		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
		SubmitInfo.pWaitDstStageMask = WaitStages;						// Stages to check semaphores at
		SubmitInfo.commandBufferCount = 1;								// Number of command buffers to submit
		SubmitInfo.signalSemaphoreCount = 1;							// Number of semaphores to signal
		SubmitInfo.pWaitSemaphores = &ImageAvailable[CurrentFrame];				// List of semaphores to wait on
		SubmitInfo.pCommandBuffers = &MainViewport.CommandBuffers[ImageIndex];		// Command buffer to submit
		SubmitInfo.pSignalSemaphores = &RenderFinished[CurrentFrame];	// Semaphores to signal when command buffer finishes
		Result = vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, DrawFences[CurrentFrame]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueueSubmit result is {}", static_cast<int>(Result));
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

		CurrentFrame = (CurrentFrame + 1) % MAX_DRAW_FRAMES;
	}

	void VulkanRenderingSystem::DeinitViewport(ViewportInstance* Viewport)
	{
		for (u32 i = 0; i < Viewport->ViewportSwapchain.ImagesCount; ++i)
		{
			vkDestroyFramebuffer(LogicalDevice, Viewport->SwapchainFramebuffers[i], nullptr);
		}

		SwapchainInstance::DestroySwapchainInstance(LogicalDevice, Viewport->ViewportSwapchain);
		vkDestroySurfaceKHR(Instance.VulkanInstance, Viewport->Surface, nullptr);
	}

	void VulkanRenderingSystem::InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport,
		SwapchainInstance SwapInstance, VkImageView* ColorBuffers, VkImageView* DepthBuffers)
	{
		OutViewport->Window = Window;
		OutViewport->Surface = Surface;
		OutViewport->ViewportSwapchain = SwapInstance;

		// Function CreateFrameBuffers
		// Create a framebuffer for each swap chain image
		for (u32 i = 0; i < OutViewport->ViewportSwapchain.ImagesCount; i++)
		{
			const u32 AttachmentsCount = 3;
			VkImageView Attachments[AttachmentsCount] = {
				OutViewport->ViewportSwapchain.ImageViews[i],
				// Todo: do not forget about position in array AttachmentDescriptions
				ColorBuffers[i],
				DepthBuffers[i]
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
		CommandBufferAllocateInfo.commandPool = GraphicsCommandPool;
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		CommandBufferAllocateInfo.commandBufferCount = static_cast<u32>(OutViewport->ViewportSwapchain.ImagesCount);

		// Allocate command buffers and place handles in array of buffers
		VkResult Result = vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, OutViewport->CommandBuffers);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateCommandBuffers result is {}", static_cast<int>(Result));
			assert(false);
		}
		// Function end CreateCommandBuffers
	}

	void VulkanRenderingSystem::CreateCommandPool(VkDevice LogicalDevice, u32 FamilyIndex)
	{
		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = FamilyIndex;	// Queue Family type that buffers from this command pool will use

		VkResult Result = vkCreateCommandPool(LogicalDevice, &PoolInfo, nullptr, &GraphicsCommandPool);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateCommandPool result is {}", static_cast<int>(Result));
			assert(false);
		}
	}
}
