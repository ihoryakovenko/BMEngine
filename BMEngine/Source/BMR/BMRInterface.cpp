#include "BMRInterface.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>

#include "Memory/MemoryManagmentSystem.h"
#include "VulkanMemoryManagementSystem.h"

#include "VulkanCoreTypes.h"
#include "MainRenderPass.h"
#include "VulkanHelper.h"
#include "Util/Util.h"

namespace BMR
{
	static u32 UpdateTextureDescriptor(u32* ImageViewIndices, u32 Count, DescriptorLayoutHandles LayoutHandle);

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

	static VkDevice CreateLogicalDevice(BMRPhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		u32 DeviceExtensionsSize);
	static VkSampler CreateTextureSampler();

	static bool SetupQueues();
	static bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

	static void CreateSynchronisation();
	static void DestroySynchronisation();

	static void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, BMRViewportInstance* OutViewport,
		BMRSwapchainInstance SwapInstance, VkImageView* ColorBuffers, VkImageView* DepthBuffers);
	static void DeinitViewport(BMRViewportInstance* Viewport);

	static void CreateCommandPool(VkDevice LogicalDevice, u32 FamilyIndex);

	static void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection);

	static BMRConfig Config;

	static BMRMainInstance Instance;
	static VkDevice LogicalDevice = nullptr;
	static BMRDeviceInstance Device;
	static BMRMainRenderPass MainRenderPass;

	static BMRViewportInstance MainViewport;

	static VkQueue GraphicsQueue = nullptr;
	static VkQueue PresentationQueue = nullptr;

	// Todo: pass as AddViewport params? 
	static VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
	static VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

	static int CurrentFrame = 0;

	static VkSemaphore ImagesAvailable[MAX_DRAW_FRAMES];
	static VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
	static VkFence DrawFences[MAX_DRAW_FRAMES];

	static VkCommandPool GraphicsCommandPool = nullptr;

	static BMRGPUBuffer VertexBuffer;
	static u32 VertexBufferOffset = 0;
	static BMRGPUBuffer IndexBuffer;
	static u32 IndexBufferOffset = 0;

	static VkDescriptorPool MainPool = nullptr;

	static VkSampler Sampler[SamplerType::SamplerType_Count];

	static BMRImageBuffer TextureBuffers[MAX_IMAGES];
	static u32 ImageArraysCount = 0;
	static VkImageView TextureImageViews[MAX_IMAGES];

	static VkImageViewType ImageViewTypeTable[] =
	{
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_VIEW_TYPE_CUBE
	};

	static VkImageCreateFlagBits ImageFlagsTable[] =
	{
		static_cast<VkImageCreateFlagBits>(0),
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	};

	bool Init(GLFWwindow* Window, const BMRConfig& InConfig)
	{
		Config = InConfig;

		const char* ValidationLayers[] = {
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_monitor",
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
				assert(false);
			}
		}

		Instance = BMRMainInstance::CreateMainInstance(RequiredExtensions.Pointer.Data, RequiredExtensions.Count,
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

		BMRSwapchainInstance SwapInstance1 = BMRSwapchainInstance::CreateSwapchainInstance(Device.PhysicalDevice, Device.Indices,
			LogicalDevice, Surface, SurfaceFormat, Extent1);

		const u32 PoolSizeCount = 13;
		auto TotalPassPoolSizes = Memory::FramePointer<VkDescriptorPoolSize>::Create(PoolSizeCount);
		u32 TotalDescriptorLayouts = 21;
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
		TotalPassPoolSizes[10] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance1.ImagesCount };
		TotalPassPoolSizes[11] = { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = Config.MaxTextures };
		TotalPassPoolSizes[12] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance1.ImagesCount };

		u32 TotalDescriptorCount = TotalDescriptorLayouts * SwapInstance1.ImagesCount;
		TotalDescriptorCount += Config.MaxTextures;

		VulkanMemoryManagementSystem::BMRMemorySourceDevice MemoryDevice;
		MemoryDevice.PhysicalDevice = Device.PhysicalDevice;
		MemoryDevice.LogicalDevice = LogicalDevice;
		MemoryDevice.TransferCommandPool = GraphicsCommandPool;
		MemoryDevice.TransferQueue = GraphicsQueue;

		VulkanMemoryManagementSystem::Init(MemoryDevice);
		MainPool = VulkanMemoryManagementSystem::AllocateDescriptorPool(TotalPassPoolSizes.Data, PoolSizeCount, TotalDescriptorCount);

		VertexBuffer = VulkanMemoryManagementSystem::CreateBuffer(MB64,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		IndexBuffer = VulkanMemoryManagementSystem::CreateBuffer(MB64,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		BMRPipelineShaderInput ShaderInputs[BMR::BMRShaderNames::ShaderNamesCount];
		for (u32 i = 0; i < BMR::BMRShaderNames::ShaderNamesCount; ++i)
		{
			ShaderInputs[i].Code = Config.RenderShaders[i].Code;
			ShaderInputs[i].CodeSize = Config.RenderShaders[i].CodeSize;
			ShaderInputs[i].EntryPoint = "main";
			ShaderInputs[i].Handle = Config.RenderShaders[i].Handle;
			ShaderInputs[i].Stage = Config.RenderShaders[i].Stage;
		}

		MainRenderPass.CreateVulkanPass(LogicalDevice, ColorFormat, DepthFormat, SurfaceFormat);
		MainRenderPass.SetupPushConstants();
		MainRenderPass.CreateDescriptorLayouts(LogicalDevice);
		MainRenderPass.CreatePipelineLayouts(LogicalDevice);
		MainRenderPass.CreatePipelines(LogicalDevice, Extent1, ShaderInputs);
		MainRenderPass.CreateAttachments(Device.PhysicalDevice, LogicalDevice, SwapInstance1.ImagesCount, Extent1, DepthFormat, ColorFormat);
		MainRenderPass.CreateUniformBuffers(Device.PhysicalDevice, LogicalDevice, SwapInstance1.ImagesCount);
		MainRenderPass.CreateSets(MainPool, LogicalDevice, SwapInstance1.ImagesCount);
		MainRenderPass.CreateFrameBuffer(LogicalDevice, Extent1, SwapInstance1.ImagesCount, SwapInstance1.ImageViews);

		InitViewport(Window, Surface, &MainViewport, SwapInstance1, MainRenderPass.ColorBufferViews, MainRenderPass.DepthBufferViews);

		Sampler[SamplerType::SamplerType_Diffuse] = CreateTextureSampler();
		Sampler[SamplerType::SamplerType_Specular] = CreateTextureSampler();

		return true;
	}

	void DeInit()
	{
		vkDeviceWaitIdle(LogicalDevice);

		DestroySynchronisation();

		vkDestroySampler(LogicalDevice, Sampler[SamplerType::SamplerType_Diffuse], nullptr);
		vkDestroySampler(LogicalDevice, Sampler[SamplerType::SamplerType_Specular], nullptr);

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
		MainRenderPass.ClearResources(LogicalDevice, MainViewport.ViewportSwapchain.ImagesCount);

		DeinitViewport(&MainViewport);

		vkDestroyDevice(LogicalDevice, nullptr);

		BMRMainInstance::DestroyMainInstance(Instance);
	}

	u32 LoadTexture(BMRTextureArrayInfo Info, BMRTextureType TextureType)
	{
		const VkImageViewType TextureImageType = ImageViewTypeTable[TextureType];

		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D; // Type of image (1D, 2D, or 3D)
		ImageCreateInfo.extent.width = Info.Width;
		ImageCreateInfo.extent.height = Info.Height;
		ImageCreateInfo.extent.depth = 1; // Depth of image (just 1, no 3D aspect)
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = Info.LayersCount;
		ImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB; // TODO: use VK_FORMAT_R8G8B8A8_UNORM for specular maps and normal maps
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // How image data should be "tiled" (arranged for optimal reading)
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout of image data on creation
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // Bit flags defining what image will be used for
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; // Number of samples for multi-sampling
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Whether image can be shared between queues
		ImageCreateInfo.flags = ImageFlagsTable[TextureType];

		BMRImageBuffer ImageBuffer = VulkanMemoryManagementSystem::CreateImageBuffer(&ImageCreateInfo);

		VkImageMemoryBarrier Barrier;
		Barrier = { };
		Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;									// Layout to transition from
		Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;									// Layout to transition to
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
		Barrier.image = ImageBuffer.Image;											// Image being accessed and modified as part of barrier
		Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
		Barrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
		Barrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from baseMipLevel
		Barrier.subresourceRange.baseArrayLayer = 0;
		Barrier.subresourceRange.layerCount = Info.LayersCount;

		Barrier.srcAccessMask = 0;								// Memory access stage transition must after...
		Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// Memory access stage transition must before...

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
			1, &Barrier	// Image Memory Barrier count + data
		);

		vkEndCommandBuffer(CommandBuffer);

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(LogicalDevice, ImageBuffer.Image, &MemoryRequirements);

		VulkanMemoryManagementSystem::CopyDataToImage(ImageBuffer.Image, Info.Width, Info.Height,
			Info.Format, MemoryRequirements.size / Info.LayersCount, Info.LayersCount, Info.Data);

		Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // Layout to transition from
		Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
		vkCmdPipelineBarrier(CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &Barrier);
		vkEndCommandBuffer(CommandBuffer);

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);

		vkFreeCommandBuffers(LogicalDevice, GraphicsCommandPool, 1, &CommandBuffer);

		VkImageView View = CreateImageView(LogicalDevice, ImageBuffer.Image,
			VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, TextureImageType,
			Info.LayersCount);

		TextureBuffers[ImageArraysCount] = ImageBuffer;
		TextureImageViews[ImageArraysCount] = View;

		const u32 CurrentIndex = ImageArraysCount;
		++ImageArraysCount;

		return CurrentIndex;
	}

	// Shit but works for now
	u32 UpdateTextureDescriptor(u32* ImageViewIndices, u32 Count, DescriptorLayoutHandles LayoutHandle)
	{
		VkWriteDescriptorSet TextureWriteData[SamplerType::SamplerType_Count];
		VkDescriptorImageInfo TextureImageInfo[SamplerType::SamplerType_Count];
		VkDescriptorSetLayout Layout = MainRenderPass.DescriptorLayouts[LayoutHandle];
		VkDescriptorSet& Descriptor = MainRenderPass.SamplerDescriptors[MainRenderPass.TextureDescriptorCount];

		VulkanMemoryManagementSystem::AllocateSets(MainPool, &Layout, 1, &Descriptor);

		for (u32 i = 0; i < Count; ++i)
		{
			TextureImageInfo[i] = { };
			TextureImageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			TextureImageInfo[i].imageView = TextureImageViews[ImageViewIndices[i]];
			TextureImageInfo[i].sampler = Sampler[i];

			TextureWriteData[i] = { };
			TextureWriteData[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			TextureWriteData[i].dstSet = Descriptor;
			TextureWriteData[i].dstBinding = i;
			TextureWriteData[i].dstArrayElement = 0;
			TextureWriteData[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			TextureWriteData[i].descriptorCount = 1;
			TextureWriteData[i].pImageInfo = TextureImageInfo + i;
		}

		vkUpdateDescriptorSets(LogicalDevice, Count, TextureWriteData, 0, nullptr);

		const u32 CurrentIndex = MainRenderPass.TextureDescriptorCount;
		++MainRenderPass.TextureDescriptorCount;

		return CurrentIndex;
	}

	u32 LoadEntityMaterial(u32 DiffuseTextureIndex, u32 SpecularTextureIndex)
	{
		u32 TextureIndices[SamplerType::SamplerType_Count];
		TextureIndices[SamplerType::SamplerType_Diffuse] = DiffuseTextureIndex;
		TextureIndices[SamplerType::SamplerType_Specular] = SpecularTextureIndex;
		return UpdateTextureDescriptor(TextureIndices, SamplerType::SamplerType_Count, DescriptorLayoutHandles::EntitySampler);
	}

	u32 LoadTerrainMaterial(u32 DiffuseTextureIndex)
	{
		return UpdateTextureDescriptor(&DiffuseTextureIndex, 1, DescriptorLayoutHandles::TerrainSampler);
	}

	u32 LoadSkyBoxMaterial(u32 CubeTextureIndex)
	{
		return UpdateTextureDescriptor(&CubeTextureIndex, 1, DescriptorLayoutHandles::TerrainSampler);
	}

	u64 LoadVertices(const void* Vertices, u32 VertexSize, VkDeviceSize VerticesCount)
	{
		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		const VkDeviceSize AlignedSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshVerticesSize);

		VulkanMemoryManagementSystem::CopyDataToBuffer(VertexBuffer.Buffer, VertexBufferOffset, MeshVerticesSize, Vertices);

		const VkDeviceSize CurrentOffset = VertexBufferOffset;
		VertexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	u64 LoadIndices(const u32* Indices, u32 IndicesCount)
	{
		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshIndicesSize);

		VulkanMemoryManagementSystem::CopyDataToBuffer(IndexBuffer.Buffer, IndexBufferOffset, MeshIndicesSize, Indices);

		const VkDeviceSize CurrentOffset = IndexBufferOffset;
		IndexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	void UpdateLightBuffer(const BMRLightBuffer& Buffer)
	{
		const u32 UpdateIndex = (MainRenderPass.ActiveLightSet + 1) % MainViewport.ViewportSwapchain.ImagesCount;

		VulkanMemoryManagementSystem::CopyDataToMemory(MainRenderPass.LightBuffers[UpdateIndex].Memory, 0,
			sizeof(BMRLightBuffer), &Buffer);

		MainRenderPass.ActiveLightSet = UpdateIndex;
	}

	void UpdateMaterialBuffer(const BMRMaterial& Buffer)
	{
		VulkanMemoryManagementSystem::CopyDataToMemory(MainRenderPass.MaterialBuffer.Memory, 0,
			sizeof(BMRMaterial), &Buffer);
	}

	void UpdateLightSpaceBuffer(const BMRLightSpaceMatrix* LightSpaceMatrix)
	{
		const u32 UpdateIndex = (MainRenderPass.ActiveLightSpaceMatrixSet + 1) % MainViewport.ViewportSwapchain.ImagesCount;

		VulkanMemoryManagementSystem::CopyDataToMemory(MainRenderPass.DepthLightSpaceBuffers[UpdateIndex].Memory, 0,
			sizeof(BMRLightSpaceMatrix), LightSpaceMatrix);

		MainRenderPass.ActiveLightSpaceMatrixSet = UpdateIndex;
	}

	void Draw(const BMRDrawScene& Scene)
	{
		UpdateVpBuffer(Scene.ViewProjection);

		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		const u32 MainPassClearValuesSize = 3;
		VkClearValue MainPassClearValues[MainPassClearValuesSize];
		// Do not forget about position in array AttachmentDescriptions
		MainPassClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		MainPassClearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		MainPassClearValues[2].depthStencil.depth = 1.0f;

		const u32 DepthPassClearValuesSize = 1;
		VkClearValue DepthPassClearValues[DepthPassClearValuesSize];
		// Do not forget about position in array AttachmentDescriptions
		DepthPassClearValues[0].depthStencil.depth = 1.0f;

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		VkFence Fence = DrawFences[CurrentFrame];
		VkSemaphore ImageAvailable = ImagesAvailable[CurrentFrame];

		vkWaitForFences(LogicalDevice, 1, &Fence, VK_TRUE, std::numeric_limits<u64>::max());
		vkResetFences(LogicalDevice, 1, &Fence);

		u32 ImageIndex;
		vkAcquireNextImageKHR(LogicalDevice, MainViewport.ViewportSwapchain.VulkanSwapchain, std::numeric_limits<u64>::max(),
			ImageAvailable, VK_NULL_HANDLE, &ImageIndex);

		VkCommandBuffer CommandBuffer = MainViewport.CommandBuffers[ImageIndex];

		VkResult Result = vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			assert(false);
		}

		{
			VkExtent2D SwapExtent;
			SwapExtent.height = 1024;
			SwapExtent.width = 1024;


			VkRenderPassBeginInfo DepthRenderPassBeginInfo = { };
			DepthRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			DepthRenderPassBeginInfo.renderPass = MainRenderPass.DepthRenderPass;
			DepthRenderPassBeginInfo.renderArea.offset = { 0, 0 };
			DepthRenderPassBeginInfo.pClearValues = DepthPassClearValues;
			DepthRenderPassBeginInfo.clearValueCount = DepthPassClearValuesSize;
			DepthRenderPassBeginInfo.renderArea.extent = SwapExtent; // Size of region to run render pass on (starting at offset)
			DepthRenderPassBeginInfo.framebuffer = MainRenderPass.DepthPassFramebuffers[ImageIndex];

			vkCmdBeginRenderPass(CommandBuffer, &DepthRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Depth]);

			// TODO: Support rework to not create identical index buffers
			for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
			{
				BMRDrawEntity* DrawEntity = Scene.DrawEntities + i;

				const VkBuffer VertexBuffers[] = { VertexBuffer.Buffer };
				const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

				const u32 DescriptorSetGroupCount = 1;
				const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
				{
					MainRenderPass.DescriptorsToImages[DescriptorHandles::DepthLightSpace][MainRenderPass.ActiveLightSpaceMatrixSet],
				};

				const VkPipelineLayout PipelineLayout = MainRenderPass.PipelineLayouts[BMRPipelineHandles::Depth];

				vkCmdPushConstants(CommandBuffer, PipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BMRModel), &DrawEntity->Model);

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
			}

			vkCmdEndRenderPass(CommandBuffer);
		}

		VkRenderPassBeginInfo MainRenderPassBeginInfo = { };
		MainRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		MainRenderPassBeginInfo.renderPass = MainRenderPass.RenderPass;
		MainRenderPassBeginInfo.renderArea.offset = { 0, 0 };
		MainRenderPassBeginInfo.pClearValues = MainPassClearValues;
		MainRenderPassBeginInfo.clearValueCount = MainPassClearValuesSize;
		MainRenderPassBeginInfo.renderArea.extent = MainViewport.ViewportSwapchain.SwapExtent; // Size of region to run render pass on (starting at offset)
		MainRenderPassBeginInfo.framebuffer = MainRenderPass.Framebuffers[ImageIndex];

		vkCmdBeginRenderPass(CommandBuffer, &MainRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		{

			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Terrain]);

			for (u32 i = 0; i < Scene.DrawTerrainEntitiesCount; ++i)
			{
				BMRDrawTerrainEntity* DrawTerrainEntity = Scene.DrawTerrainEntities + i;

				const VkBuffer TerrainVertexBuffers[] = { VertexBuffer.Buffer };
				const VkDeviceSize TerrainBuffersOffsets[] = { DrawTerrainEntity->VertexOffset };

				const u32 TerrainDescriptorSetGroupCount = 2;
				const VkDescriptorSet TerrainDescriptorSetGroup[TerrainDescriptorSetGroupCount] = {
					MainRenderPass.DescriptorsToImages[DescriptorHandles::TerrainVp][MainRenderPass.ActiveVpSet],
					MainRenderPass.SamplerDescriptors[DrawTerrainEntity->MaterialIndex]
				};

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.PipelineLayouts[BMRPipelineHandles::Terrain],
					0, TerrainDescriptorSetGroupCount, TerrainDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, TerrainVertexBuffers, TerrainBuffersOffsets);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, DrawTerrainEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, DrawTerrainEntity->IndicesCount, 1, 0, 0, 0);
			}
		}

		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Entity]);

			// TODO: Support rework to not create identical index buffers
			for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
			{
				BMRDrawEntity* DrawEntity = Scene.DrawEntities + i;

				const VkBuffer VertexBuffers[] = { VertexBuffer.Buffer };
				const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

				const u32 DescriptorSetGroupCount = 4;
				const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
				{
					MainRenderPass.DescriptorsToImages[DescriptorHandles::EntityVp][MainRenderPass.ActiveVpSet],
					MainRenderPass.SamplerDescriptors[DrawEntity->MaterialIndex],
					MainRenderPass.DescriptorsToImages[DescriptorHandles::EntityLigh][MainRenderPass.ActiveLightSet],
					MainRenderPass.MaterialSet
				};

				const VkPipelineLayout PipelineLayout = MainRenderPass.PipelineLayouts[BMRPipelineHandles::Entity];

				vkCmdPushConstants(CommandBuffer, PipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BMRModel), &DrawEntity->Model);

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, DescriptorSetGroupCount, DescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, VertexBuffers, Offsets);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, DrawEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, DrawEntity->IndicesCount, 1, 0, 0, 0);
			}
		}

		{
			if (Scene.DrawSkyBox)
			{
				vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::SkyBox]);

				const u32 SkyBoxDescriptorSetGroupCount = 2;
				const VkDescriptorSet SkyBoxDescriptorSetGroup[SkyBoxDescriptorSetGroupCount] = {
					MainRenderPass.DescriptorsToImages[DescriptorHandles::SkyBoxVp][MainRenderPass.ActiveVpSet],
					MainRenderPass.SamplerDescriptors[Scene.SkyBox.MaterialIndex],
				};

				const VkPipelineLayout PipelineLayout = MainRenderPass.PipelineLayouts[BMRPipelineHandles::SkyBox];

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, SkyBoxDescriptorSetGroupCount, SkyBoxDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Scene.SkyBox.VertexOffset);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, Scene.SkyBox.IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, Scene.SkyBox.IndicesCount, 1, 0, 0, 0);
			}
		}

		vkCmdNextSubpass(CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);

		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Deferred]);

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.PipelineLayouts[BMRPipelineHandles::Deferred],
				0, 1, &MainRenderPass.DescriptorsToImages[DescriptorHandles::DeferredInput][ImageIndex], 0, nullptr);

			vkCmdDraw(CommandBuffer, 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass
		}

		vkCmdEndRenderPass(CommandBuffer);

		Result = vkEndCommandBuffer(CommandBuffer);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkBeginCommandBuffer result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitDstStageMask = WaitStages;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &ImageAvailable;
		SubmitInfo.pCommandBuffers = &CommandBuffer; // Command buffer to submit
		SubmitInfo.pSignalSemaphores = &RenderFinished[CurrentFrame]; // Semaphores to signal when command buffer finishes
		Result = vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, Fence);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueueSubmit result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkPresentInfoKHR PresentInfo = { };
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pWaitSemaphores = &RenderFinished[CurrentFrame];
		PresentInfo.pSwapchains = &MainViewport.ViewportSwapchain.VulkanSwapchain; // Swapchains to present images to
		PresentInfo.pImageIndices = &ImageIndex; // Index of images in swapchains to present
		Result = vkQueuePresentKHR(PresentationQueue, &PresentInfo);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkQueuePresentKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		CurrentFrame = (CurrentFrame + 1) % MAX_DRAW_FRAMES;
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

	VkDevice CreateLogicalDevice(BMRPhysicalDeviceIndices Indices, const char* DeviceExtensions[],
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
		SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;	// How to handle texture wrap in U (x) direction
		SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;	// How to handle texture wrap in V (y) direction
		SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;	// How to handle texture wrap in W (z) direction
		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
		SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
		SamplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
		SamplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
		SamplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
		SamplerCreateInfo.anisotropyEnable = VK_TRUE;
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

	void CreateSynchronisation()
	{
		VkSemaphoreCreateInfo SemaphoreCreateInfo = { };
		SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo FenceCreateInfo = { };
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (u64 i = 0; i < MAX_DRAW_FRAMES; i++)
		{
			if (vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &ImagesAvailable[i]) != VK_SUCCESS ||
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
			vkDestroySemaphore(LogicalDevice, ImagesAvailable[i], nullptr);
			vkDestroyFence(LogicalDevice, DrawFences[i], nullptr);
		}
	}

	void DeinitViewport(BMRViewportInstance* Viewport)
	{
		BMRSwapchainInstance::DestroySwapchainInstance(LogicalDevice, Viewport->ViewportSwapchain);
		vkDestroySurfaceKHR(Instance.VulkanInstance, Viewport->Surface, nullptr);
	}

	void InitViewport(GLFWwindow* Window, VkSurfaceKHR Surface, BMRViewportInstance* OutViewport,
		BMRSwapchainInstance SwapInstance, VkImageView* ColorBuffers, VkImageView* DepthBuffers)
	{
		OutViewport->Window = Window;
		OutViewport->Surface = Surface;
		OutViewport->ViewportSwapchain = SwapInstance;

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = { };
		CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CommandBufferAllocateInfo.commandPool = GraphicsCommandPool;
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		CommandBufferAllocateInfo.commandBufferCount = static_cast<u32>(OutViewport->ViewportSwapchain.ImagesCount);

		VkResult Result = vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, OutViewport->CommandBuffers);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkAllocateCommandBuffers result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void CreateCommandPool(VkDevice LogicalDevice, u32 FamilyIndex)
	{
		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = FamilyIndex;

		VkResult Result = vkCreateCommandPool(LogicalDevice, &PoolInfo, nullptr, &GraphicsCommandPool);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateCommandPool result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection)
	{
		const u32 UpdateIndex = (MainRenderPass.ActiveVpSet + 1) % MainViewport.ViewportSwapchain.ImagesCount;

		VulkanMemoryManagementSystem::CopyDataToMemory(MainRenderPass.VpUniformBuffers[UpdateIndex].Memory, 0,
			sizeof(BMRUboViewProjection), &ViewProjection);

		MainRenderPass.ActiveVpSet = UpdateIndex;
	}
}
