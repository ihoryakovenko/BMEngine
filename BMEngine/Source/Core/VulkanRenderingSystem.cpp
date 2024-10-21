#include "VulkanRenderingSystem.h"

#include <algorithm>

#include "Memory/MemoryManagmentSystem.h"
#include "VulkanMemoryManagementSystem.h"

#include "MainRenderPass.h"
#include "VulkanHelper.h"
#include "Util/Util.h"

namespace Core::VulkanRenderingSystemInterface
{
	static u32 CreateTextureDescriptorSets(VkImageView DiffuseTexture, VkImageView SpecularTexture);

	static Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties();
	static Memory::FrameArray<VkLayerProperties> GetAvailableInstanceLayerProperties();
	static Memory::FrameArray<const char*> GetRequiredInstanceExtensions(const char** ValidationExtensions, u32 ValidationExtensionsCount);
	static Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR Surface);

	static VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, GLFWwindow* Window);

	static bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
		const char** RequiredExtensions, u32 RequiredExtensionsCount);
	static bool CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
		const char** ValidationLeyersToCheck, u32 ValidationLeyersToCheckSize);
	static VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface);

	static VkDevice CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		u32 DeviceExtensionsSize);
	static VkSampler CreateTextureSampler();

	static bool SetupQueues();
	static bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

	static void CreateSynchronisation();
	static void DestroySynchronisation();

	static void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport,
		SwapchainInstance SwapInstance, VkImageView* ColorBuffers, VkImageView* DepthBuffers);
	static void DeinitViewport(ViewportInstance* Viewport);

	static void CreateCommandPool(VkDevice LogicalDevice, u32 FamilyIndex);

	static RenderConfig Config;

	static MainInstance Instance;
	static VkDevice LogicalDevice = nullptr;
	static DeviceInstance Device;
	static MainRenderPass MainPass;

	static ViewportInstance MainViewport;

	static VkQueue GraphicsQueue = nullptr;
	static VkQueue PresentationQueue = nullptr;

	// Todo: pass as AddViewport params? 
	static VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
	static VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

	static const u32 MAX_DRAW_FRAMES = 2;
	static int CurrentFrame = 0;

	static VkSemaphore ImageAvailable[MAX_DRAW_FRAMES];
	static VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
	static VkFence DrawFences[MAX_DRAW_FRAMES];

	static VkCommandPool GraphicsCommandPool = nullptr;

	static GPUBuffer VertexBuffer;
	static u32 VertexBufferOffset = 0;
	static GPUBuffer IndexBuffer;
	static u32 IndexBufferOffset = 0;

	static VkDescriptorPool MainPool = nullptr;

	static VkSampler DiffuseTextureSampler = nullptr;
	static VkSampler SpecularTextureSampler = nullptr;
	static inline ImageBuffer TextureBuffers[MAX_IMAGES];
	static u32 ImageArraysCount = 0;
	static inline VkImageView TextureImageViews[MAX_IMAGES];

	static u32 TerrainIndicesCount = 0;
	static VkDeviceSize TerrainIndexOffset = 0;

	bool Init(GLFWwindow* Window, const RenderConfig& InConfig)
	{
		Config = InConfig;

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

		const u32 PoolSizeCount = 10;
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

		TotalPassPoolSizes[9] = { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = Config.MaxTextures };

		u32 TotalDescriptorCount = TotalDescriptorLayouts * SwapInstance1.ImagesCount;
		TotalDescriptorCount += Config.MaxTextures;

		VulkanMemoryManagementSystem::MemorySourceDevice MemoryDevice;
		MemoryDevice.PhysicalDevice = Device.PhysicalDevice;
		MemoryDevice.LogicalDevice = LogicalDevice;
		MemoryDevice.TransferCommandPool = GraphicsCommandPool;
		MemoryDevice.TransferQueue = GraphicsQueue;

		VulkanMemoryManagementSystem::Init(MemoryDevice);
		MainPool = VulkanMemoryManagementSystem::AllocateDescriptorPool(TotalPassPoolSizes.Data, PoolSizeCount, TotalDescriptorCount);

		ShaderInput ShaderInputs[ShaderNames::ShadersCount];
		for (u32 i = 0; i < Config.ShadersCount; ++i)
		{
			ShaderInputs[i].ShaderName = Config.RenderShaders[i].Name;
			CreateShader(LogicalDevice, Config.RenderShaders[i].Code, Config.RenderShaders[i].CodeSize, ShaderInputs[i].Module);
		}

		VertexBuffer = VulkanMemoryManagementSystem::CreateBuffer(Mb64, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		IndexBuffer = VulkanMemoryManagementSystem::CreateBuffer(Mb64,
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

		DiffuseTextureSampler = CreateTextureSampler();
		SpecularTextureSampler = CreateTextureSampler();

		return true;
	}

	void DeInit()
	{
		vkDeviceWaitIdle(LogicalDevice);
		
		DestroySynchronisation();

		vkDestroySampler(LogicalDevice, DiffuseTextureSampler, nullptr);
		vkDestroySampler(LogicalDevice, SpecularTextureSampler, nullptr);

		for (u32 i = 0; i < ImageArraysCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, TextureImageViews[i], nullptr);
		}

		for (u32 i = 0; i < ImageArraysCount; ++i)
		{
			VulkanMemoryManagementSystem::DestroyImageBuffer(TextureBuffers[i]);
		}

		vkDestroyDescriptorPool(LogicalDevice, MainPool, nullptr);

		VulkanMemoryManagementSystem::DestroyBuffer(VertexBuffer);
		VulkanMemoryManagementSystem::DestroyBuffer(IndexBuffer);
		VulkanMemoryManagementSystem::Deinit();

		vkDestroyCommandPool(LogicalDevice, GraphicsCommandPool, nullptr);
		MainPass.ClearResources(LogicalDevice, MainViewport.ViewportSwapchain.ImagesCount);

		DeinitViewport(&MainViewport);

		vkDestroyDevice(LogicalDevice, nullptr);

		MainInstance::DestroyMainInstance(Instance);
	}

	u32 CreateTexture(TextureArrayInfo Info)
	{
		auto Barriers = Memory::MemoryManagementSystem::FrameAlloc<VkImageMemoryBarrier>(1);

		VkImageCreateInfo ImageCreateInfo[1];
		ImageCreateInfo[0] = { };
		ImageCreateInfo[0].sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo[0].imageType = VK_IMAGE_TYPE_2D; // Type of image (1D, 2D, or 3D)
		ImageCreateInfo[0].extent.width = Info.Width;
		ImageCreateInfo[0].extent.height = Info.Height;
		ImageCreateInfo[0].extent.depth = 1; // Depth of image (just 1, no 3D aspect)
		ImageCreateInfo[0].mipLevels = 1;
		ImageCreateInfo[0].arrayLayers = 1;
		ImageCreateInfo[0].format = VK_FORMAT_R8G8B8A8_UNORM; // Format type of image
		ImageCreateInfo[0].tiling = VK_IMAGE_TILING_OPTIMAL; // How image data should be "tiled" (arranged for optimal reading)
		ImageCreateInfo[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout of image data on creation
		ImageCreateInfo[0].usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // Bit flags defining what image will be used for
		ImageCreateInfo[0].samples = VK_SAMPLE_COUNT_1_BIT; // Number of samples for multi-sampling
		ImageCreateInfo[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Whether image can be shared between queues

		ImageBuffer ImageBuffer = VulkanMemoryManagementSystem::CreateImageBuffer(&(ImageCreateInfo[0]));

		Barriers[0] = { };
		Barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		Barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;									// Layout to transition from
		Barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;									// Layout to transition to
		Barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
		Barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
		Barriers[0].image = ImageBuffer.Image;											// Image being accessed and modified as part of barrier
		Barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
		Barriers[0].subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
		Barriers[0].subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from baseMipLevel
		Barriers[0].subresourceRange.baseArrayLayer = 0;						// First layer to start alterations on
		Barriers[0].subresourceRange.layerCount = 1; // Number of layers to alter starting from baseArrayLayer

		Barriers[0].srcAccessMask = 0;								// Memory access stage transition must after...
		Barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// Memory access stage transition must before...

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
			1, Barriers	// Image Memory Barrier count + data
		);

		vkEndCommandBuffer(CommandBuffer);

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(LogicalDevice, ImageBuffer.Image, &MemoryRequirements);

		VulkanMemoryManagementSystem::CopyDataToImage(ImageBuffer.Image, Info.Width, Info.Height,
			MemoryRequirements.size, 1, Info.Data);

		Barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // Layout to transition from
		Barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		Barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
		vkCmdPipelineBarrier(CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, Barriers);
		vkEndCommandBuffer(CommandBuffer);

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		vkFreeCommandBuffers(LogicalDevice, GraphicsCommandPool, 1, &CommandBuffer);

		VkImageView View = CreateImageView(LogicalDevice, ImageBuffer.Image,
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		TextureBuffers[ImageArraysCount] = ImageBuffer;
		TextureImageViews[ImageArraysCount] = View;

		const u32 CurrentIndex = ImageArraysCount;
		++ImageArraysCount;

		return CurrentIndex;
	}

	u32 CreateMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex)
	{
		return CreateTextureDescriptorSets(TextureImageViews[DiffuseTextureIndex],
			TextureImageViews[SpecularTextureIndex]);
	}

	u32 CreateTextureDescriptorSets(VkImageView DiffuseTexture, VkImageView SpecularTexture)
	{
		auto TextureWriteData = Memory::MemoryManagementSystem::FrameAlloc<VkWriteDescriptorSet>(3);
		auto TextureImageInfos = Memory::MemoryManagementSystem::FrameAlloc<VkDescriptorImageInfo>(2);
		auto Layouts = Memory::MemoryManagementSystem::FrameAlloc<VkDescriptorSetLayout>(2);

		Layouts[0] = MainPass.EntityPass.EntitySamplerSetLayout;
		Layouts[1] = MainPass.TerrainPass.TerrainSamplerSetLayout;

		VkDescriptorSet EntitySet;
		VkDescriptorSet TerrainSet;

		VulkanMemoryManagementSystem::AllocateSets(MainPool, &(Layouts[0]),
			1, &EntitySet);

		VulkanMemoryManagementSystem::AllocateSets(MainPool, &(Layouts[1]),
			1, &TerrainSet);

		TextureImageInfos[0] = { };
		TextureImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		TextureImageInfos[0].imageView = DiffuseTexture;
		TextureImageInfos[0].sampler = DiffuseTextureSampler;

		TextureImageInfos[1] = TextureImageInfos[0];
		TextureImageInfos[1].imageView = SpecularTexture;
		TextureImageInfos[1].sampler = SpecularTextureSampler;

		TextureWriteData[0] = { };
		TextureWriteData[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		TextureWriteData[0].dstSet = EntitySet;
		TextureWriteData[0].dstBinding = 0;
		TextureWriteData[0].dstArrayElement = 0;
		TextureWriteData[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		TextureWriteData[0].descriptorCount = 1;
		TextureWriteData[0].pImageInfo = &TextureImageInfos[0];

		TextureWriteData[1] = TextureWriteData[0];
		TextureWriteData[1].dstBinding = 1;
		TextureWriteData[1].pImageInfo = &TextureImageInfos[1];

		TextureWriteData[2] = TextureWriteData[0];
		TextureWriteData[2].dstSet = TerrainSet;
		TextureWriteData[2].dstBinding = 0;

		vkUpdateDescriptorSets(LogicalDevice, 3, TextureWriteData, 0, nullptr);

		MainPass.EntityPass.EntitySamplerDescriptorSets[MainPass.TextureDescriptorCountTest] = EntitySet;
		MainPass.TerrainPass.TerrainSamplerDescriptorSets[MainPass.TextureDescriptorCountTest] = TerrainSet;

		const u32 CurrentIndex = MainPass.TextureDescriptorCountTest;
		++MainPass.TextureDescriptorCountTest;

		return CurrentIndex;
	}

	bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
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

	bool CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
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

	VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
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

	Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties()
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

	Memory::FrameArray<VkLayerProperties> GetAvailableInstanceLayerProperties()
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

	Memory::FrameArray<const char*> GetRequiredInstanceExtensions(const char** ValidationExtensions,
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

	Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR Surface)
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

	VkDevice CreateLogicalDevice(PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		u32 DeviceExtensionsSize)
	{
		const f32 Priority = 1.0f;

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

	VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface)
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

	VkSampler CreateTextureSampler()
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

	bool SetupQueues()
	{
		vkGetDeviceQueue(LogicalDevice, static_cast<u32>(Device.Indices.GraphicsFamily), 0, &GraphicsQueue);
		vkGetDeviceQueue(LogicalDevice, static_cast<u32>(Device.Indices.PresentationFamily), 0, &PresentationQueue);

		if (GraphicsQueue == nullptr && PresentationQueue == nullptr)
		{
			return false;
		}
	}

	bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags)
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

	void CreateDrawEntities(Mesh* Meshes, u32 MeshesCount, DrawEntity* OutEntities)
	{
		VkDeviceSize TotalVertexSize = 0;
		VkDeviceSize TotalIndexSize = 0;

		for (u32 i = 0; i < MeshesCount; ++i)
		{
			const VkDeviceSize MeshVerticesSize = sizeof(EntityVertex) * Meshes[i].MeshVerticesCount;
			const VkDeviceSize MeshIndicesSize = sizeof(u32) * Meshes[i].MeshIndicesCount;

			OutEntities[i].VertexOffset = TotalVertexSize + VertexBufferOffset;
			OutEntities[i].IndexOffset = TotalIndexSize + IndexBufferOffset;

			TotalVertexSize += VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshVerticesSize);
			TotalIndexSize += VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshIndicesSize);
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

			VertexCopyPointer += VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshVerticesSize);
			IndexCopyPointer += VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshIndicesSize);

			OutEntities[i].Model = Meshes[i].Model;
			OutEntities[i].IndicesCount = Meshes[i].MeshIndicesCount;
			OutEntities[i].MaterialIndex = Meshes[i].MaterialIndex;
		}

		VulkanMemoryManagementSystem::CopyDataToBuffer(VertexBuffer.Buffer, VertexBufferOffset, TotalVertexSize, AllVertices);
		VertexBufferOffset += TotalVertexSize;
		VulkanMemoryManagementSystem::CopyDataToBuffer(IndexBuffer.Buffer, IndexBufferOffset, TotalIndexSize, AllIndices);
		IndexBufferOffset += TotalIndexSize;

		Memory::MemoryManagementSystem::Deallocate(AllVertices);
		Memory::MemoryManagementSystem::Deallocate(AllIndices);
	}

	void CreateTerrainIndices(u32* Indices, u32 IndicesCount)
	{
		assert(TerrainIndicesCount == 0);

		TerrainIndexOffset = IndexBufferOffset;
		TerrainIndicesCount = IndicesCount;

		const VkDeviceSize MeshIndexSize = sizeof(u32) * TerrainIndicesCount;

		VulkanMemoryManagementSystem::CopyDataToBuffer(IndexBuffer.Buffer, IndexBufferOffset, MeshIndexSize, Indices);
		IndexBufferOffset += VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshIndexSize);
	}

	void CreateTerrainDrawEntity(TerrainVertex* TerrainVertices, u32 TerrainVerticesCount, DrawTerrainEntity& OutTerrain)
	{
		OutTerrain.VertexOffset = VertexBufferOffset;

		const VkDeviceSize MeshVerticesSize = sizeof(TerrainVertex) * TerrainVerticesCount;

		VulkanMemoryManagementSystem::CopyDataToBuffer(VertexBuffer.Buffer, VertexBufferOffset, MeshVerticesSize, TerrainVertices);
		VertexBufferOffset += VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshVerticesSize);
	}

	void CreateSynchronisation()
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

	void DestroySynchronisation()
	{
		for (u64 i = 0; i < MAX_DRAW_FRAMES; i++)
		{
			vkDestroySemaphore(LogicalDevice, RenderFinished[i], nullptr);
			vkDestroySemaphore(LogicalDevice, ImageAvailable[i], nullptr);
			vkDestroyFence(LogicalDevice, DrawFences[i], nullptr);
		}
	}

	void Draw(const DrawScene& Scene)
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

		{
			vkCmdBindPipeline(MainViewport.CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, MainPass.TerrainPass.Pipeline);

			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			VkBuffer TerrainVertexBuffers[] = { VertexBuffer.Buffer };
			VkDeviceSize TerrainBuffersOffsets[] = { Scene.DrawTerrainEntities[0].VertexOffset };

			const u32 TerrainDescriptorSetGroupCount = 2;
			VkDescriptorSet TerrainDescriptorSetGroup[TerrainDescriptorSetGroupCount] = {
				MainPass.TerrainPass.TerrainSets[ImageIndex], MainPass.TerrainPass.TerrainSamplerDescriptorSets[0] };

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
					MainPass.EntityPass.EntitySamplerDescriptorSets[Scene.DrawEntities[j].MaterialIndex],
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
		VulkanMemoryManagementSystem::CopyDataToMemory(MainPass.VpUniformBuffers[ImageIndex].Memory, 0,
			sizeof(UboViewProjection), &Scene.ViewProjection);

		DrawPointLightEntity TestData;
		TestData.Position = glm::vec3(0.0f, 0.0f, 10.0f);
		TestData.Ambient = glm::vec3(0.2f, 0.2f, 0.2f);
		TestData.Diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		TestData.Specular = glm::vec3(1.0f, 1.0f, 1.0f);

		VulkanMemoryManagementSystem::CopyDataToMemory(MainPass.LightBuffers[ImageIndex].Memory, 0,
			sizeof(DrawPointLightEntity), &TestData);

		Material Mat;
		Mat.Shininess = 0.4f;

		VulkanMemoryManagementSystem::CopyDataToMemory(MainPass.MaterialBuffer.Memory, 0,
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

	void DeinitViewport(ViewportInstance* Viewport)
	{
		for (u32 i = 0; i < Viewport->ViewportSwapchain.ImagesCount; ++i)
		{
			vkDestroyFramebuffer(LogicalDevice, Viewport->SwapchainFramebuffers[i], nullptr);
		}

		SwapchainInstance::DestroySwapchainInstance(LogicalDevice, Viewport->ViewportSwapchain);
		vkDestroySurfaceKHR(Instance.VulkanInstance, Viewport->Surface, nullptr);
	}

	void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, ViewportInstance* OutViewport,
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

	void CreateCommandPool(VkDevice LogicalDevice, u32 FamilyIndex)
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
