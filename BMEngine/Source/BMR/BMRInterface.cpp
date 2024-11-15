#include "BMRInterface.h"

#include <Windows.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "Memory/MemoryManagmentSystem.h"
#include "VulkanMemoryManagementSystem.h"
#include "VulkanResourceManagementSystem.h"

#include "VulkanCoreTypes.h"
#include "MainRenderPass.h"
#include "VulkanHelper.h"

#include "Util/Settings.h"

namespace BMR
{
	static u32 UpdateTextureDescriptor(u32* ImageViewIndices, u32 Count, DescriptorLayoutHandles LayoutHandle);

	static Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties();
	static Memory::FrameArray<VkLayerProperties> GetAvailableInstanceLayerProperties();
	static Memory::FrameArray<const char*> GetRequiredInstanceExtensions(const char** ValidationExtensions, u32 ValidationExtensionsCount);
	static Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR Surface);

	static VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, HWND WindowHandler);

	static bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
		const char** RequiredExtensions, u32 RequiredExtensionsCount);
	static bool CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
		const char** ValidationLeyersToCheck, u32 ValidationLeyersToCheckSize);
	static VkSurfaceFormatKHR GetBestSurfaceFormat(VkSurfaceKHR Surface);

	static VkDevice CreateLogicalDevice(BMRPhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		u32 DeviceExtensionsSize);
	static VkSampler CreateTextureSampler(f32 MaxAnisotropy, VkSamplerAddressMode AddressMode);

	static bool SetupQueues();
	static bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

	static void CreateSynchronisation();
	static void DestroySynchronisation();

	static void InitViewport(VkSurfaceKHR Surface, BMRViewportInstance* OutViewport,
		BMRSwapchainInstance SwapInstance, VkImageView* ColorBuffers, VkImageView* DepthBuffers);
	static void DeinitViewport(BMRViewportInstance* Viewport);

	static void CreateCommandPool(VkDevice LogicalDevice, u32 FamilyIndex);

	static void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection);
	static void UpdateLightBuffer(const BMRLightBuffer* Buffer);
	static void UpdateLightSpaceBuffer(const BMRLightSpaceMatrix* LightSpaceMatrix);

	static VkBuffer CreateBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags Usage,
		VkMemoryPropertyFlags Properties);

	VkDeviceMemory CreateDeviceMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);

	static VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);

	static const u32 RequiredExtensionsCount = 2;
	const char* RequiredInstanceExtensions[RequiredExtensionsCount] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};

	static BMRConfig Config;

	static BMRMainInstance Instance;
	static VkDevice LogicalDevice = nullptr;
	static BMRDeviceInstance Device;
	static BMRMainRenderPass MainRenderPass;

	static BMRViewportInstance MainViewport;

	static VkQueue GraphicsQueue = nullptr;
	static VkQueue PresentationQueue = nullptr;

	static u32 CurrentFrame = 0;

	static VkSemaphore ImagesAvailable[MAX_DRAW_FRAMES];
	static VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
	static VkFence DrawFences[MAX_DRAW_FRAMES];

	static VkCommandPool GraphicsCommandPool = nullptr;

	static BMRUniformBuffer VertexBuffer;
	static u32 VertexBufferOffset = 0;
	static BMRUniformBuffer IndexBuffer;
	static u32 IndexBufferOffset = 0;

	static VkDescriptorPool MainPool = nullptr;

	static VkSampler Sampler[SamplerType::SamplerType_Count];

	static BMRUniformImage TextureBuffers[MAX_IMAGES];
	static u32 ImageArraysCount = 0;
	static VkImageView TextureImageViews[MAX_IMAGES];

	static VkSurfaceFormatKHR SurfaceFormat;





	// TODO!!!!!!!!!!!!!!!!!!!!!!!!!
	BMRRenderPass MainPass;
	BMRRenderPass DepthPass;
	BMR::BMRUniformBuffer* VpBuffer;
	BMR::BMRUniformLayout VpLayout;
	BMR::BMRUniformSet* VpSet;
	BMRUniformBuffer* EntityLight;
	BMRUniformLayout EntityLightLayout;
	BMRUniformSet* EntityLightSet;
	BMRUniformBuffer* LightSpaceBuffer;
	BMRUniformLayout LightSpaceLayout;
	BMRUniformSet* LightSpaceSet;

	VkExtent2D Extent1;
	BMRPipelineShaderInputDepr ShaderInputs[BMR::BMRShaderNames::ShaderNamesCount];
	BMRSwapchainInstance SwapInstance1;
	VkSurfaceKHR Surface;









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

	bool Init(HWND WindowHandler, const BMRConfig& InConfig)
	{
		Config = InConfig;

		SetLogHandler(Config.LogHandler);

		const char* ValidationLayers[] = {
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_monitor",
		};
		const u32 ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

		const char* ValidationExtensions[] = {
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		const u32 ValidationExtensionsSize = Config.EnableValidationLayers ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

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

		if (Config.EnableValidationLayers)
		{
			Memory::FrameArray<VkLayerProperties> LayerPropertiesData = GetAvailableInstanceLayerProperties();

			if (!CheckValidationLayersSupport(LayerPropertiesData.Pointer.Data, LayerPropertiesData.Count,
				ValidationLayers, ValidationLayersSize))
			{
				assert(false);
			}
		}

		Instance = BMRMainInstance::CreateMainInstance(RequiredExtensions.Pointer.Data, RequiredExtensions.Count,
			Config.EnableValidationLayers, ValidationLayers, ValidationLayersSize);

		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { };
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hwnd = WindowHandler;
		surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

		Surface = nullptr;
		VkResult Result = vkCreateWin32SurfaceKHR(Instance.VulkanInstance, &surfaceCreateInfo, nullptr, &Surface);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateWin32SurfaceKHR result is %d", Result);
		}

		Device.Init(Instance.VulkanInstance, Surface, DeviceExtensions, DeviceExtensionsSize);

		LogicalDevice = CreateLogicalDevice(Device.Indices, DeviceExtensions, DeviceExtensionsSize);

		SurfaceFormat = GetBestSurfaceFormat(Surface);

		const u32 FormatPrioritySize = 3;
		VkFormat FormatPriority[FormatPrioritySize] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };

		bool IsSupportedFormatFound = false;
		for (u32 i = 0; i < FormatPrioritySize; ++i)
		{
			VkFormat FormatToCheck = FormatPriority[i];
			if (IsFormatSupported(FormatToCheck, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
			{
				IsSupportedFormatFound = true;
				break;
			}

			HandleLog(BMRLogType::LogType_Warning, "Format %d is not supported", Result);
		}

		assert(IsSupportedFormatFound);


		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };

		Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device.PhysicalDevice, Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Warning, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is %d", Result);
			return false;
		}

		Extent1 = GetBestSwapExtent(SurfaceCapabilities, WindowHandler);

		CreateSynchronisation();
		SetupQueues();
		CreateCommandPool(LogicalDevice, Device.Indices.GraphicsFamily);

		SwapInstance1 = BMRSwapchainInstance::CreateSwapchainInstance(Device.PhysicalDevice, Device.Indices,
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

		VulkanResourceManagementSystem::Init(LogicalDevice, 16);

		VulkanMemoryManagementSystem::BMRMemorySourceDevice MemoryDevice;
		MemoryDevice.PhysicalDevice = Device.PhysicalDevice;
		MemoryDevice.LogicalDevice = LogicalDevice;
		MemoryDevice.TransferCommandPool = GraphicsCommandPool;
		MemoryDevice.TransferQueue = GraphicsQueue;

		VulkanMemoryManagementSystem::Init(MemoryDevice);
		MainPool = VulkanMemoryManagementSystem::AllocateDescriptorPool(TotalPassPoolSizes.Data, PoolSizeCount, TotalDescriptorCount);

		VertexBuffer = CreateUniformBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, MB64);

		IndexBuffer = CreateUniformBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, MB64);

		
		for (u32 i = 0; i < BMR::BMRShaderNames::ShaderNamesCount; ++i)
		{
			ShaderInputs[i].Code = Config.RenderShaders[i].Code;
			ShaderInputs[i].CodeSize = Config.RenderShaders[i].CodeSize;
			ShaderInputs[i].EntryPoint = "main";
			ShaderInputs[i].Handle = Config.RenderShaders[i].Handle;
			ShaderInputs[i].Stage = Config.RenderShaders[i].Stage;
		}

		Sampler[SamplerType::SamplerType_Diffuse] = CreateTextureSampler(16, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		Sampler[SamplerType::SamplerType_Specular] = CreateTextureSampler(1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		Sampler[SamplerType::SamplerType_ShadowMap] = CreateTextureSampler(1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

		

		return true;
	}

	void DeInit()
	{
		vkDeviceWaitIdle(LogicalDevice);

		DestroySynchronisation();

		for (u32 i = 0; i < SamplerType::SamplerType_Count; ++i)
		{
			vkDestroySampler(LogicalDevice, Sampler[i], nullptr);
		}

		for (u32 i = 0; i < ImageArraysCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, TextureImageViews[i], nullptr);
		}

		for (u32 i = 0; i < ImageArraysCount; ++i)
		{
			VulkanMemoryManagementSystem::DestroyImageBuffer(TextureBuffers[i]);
		}

		vkDestroyDescriptorPool(LogicalDevice, MainPool, nullptr);

		DestroyUniformBuffer(VertexBuffer);
		DestroyUniformBuffer(IndexBuffer);
		VulkanMemoryManagementSystem::Deinit();

		VulkanResourceManagementSystem::DeInit();

		vkDestroyCommandPool(LogicalDevice, GraphicsCommandPool, nullptr);
		MainRenderPass.ClearResources(LogicalDevice, MainViewport.ViewportSwapchain.ImagesCount);

		DeinitViewport(&MainViewport);

		vkDestroyDevice(LogicalDevice, nullptr);

		BMRMainInstance::DestroyMainInstance(Instance);
	}

	void TestSetRendeRpasses(BMRRenderPass Main, BMRRenderPass Depth,
		BMRUniformBuffer* inVpBuffer, BMRUniformLayout iVpLayout, BMRUniformSet* iVpSet,
		BMRUniformBuffer* inEntityLight, BMRUniformLayout iEntityLightLayout, BMRUniformSet* iEntityLightSet,
		BMRUniformBuffer* iLightSpaceBuffer, BMRUniformLayout iLightSpaceLayout, BMRUniformSet* iLightSpaceSet)
	{
		MainPass = Main;
		DepthPass = Depth;
		VpBuffer = inVpBuffer;
		VpLayout = iVpLayout;
		VpSet = iVpSet;
		EntityLight = inEntityLight;
		EntityLightLayout = iEntityLightLayout;
		EntityLightSet = iEntityLightSet;
		LightSpaceBuffer = iLightSpaceBuffer;
		LightSpaceLayout = iLightSpaceLayout;
		LightSpaceSet = iLightSpaceSet;

		MainRenderPass.SetupPushConstants();
		MainRenderPass.CreateDescriptorLayouts(LogicalDevice);
		MainRenderPass.CreatePipelineLayouts(LogicalDevice, VpLayout, EntityLightLayout, LightSpaceLayout);
		MainRenderPass.CreatePipelines(LogicalDevice, Extent1, ShaderInputs, MainPass, DepthPass);
		MainRenderPass.CreateImages(Device.PhysicalDevice, LogicalDevice, SwapInstance1.ImagesCount, Extent1, DepthFormat, ColorFormat);
		MainRenderPass.CreateUniformBuffers(Device.PhysicalDevice, LogicalDevice, SwapInstance1.ImagesCount);
		MainRenderPass.CreateSets(MainPool, LogicalDevice, SwapInstance1.ImagesCount, Sampler[SamplerType::SamplerType_ShadowMap]);
		MainRenderPass.CreateFrameBuffer(LogicalDevice, Extent1, SwapInstance1.ImagesCount, SwapInstance1.ImageViews, MainPass, DepthPass);

		InitViewport(Surface, &MainViewport, SwapInstance1, MainRenderPass.ColorBufferViews, MainRenderPass.DepthBufferViews);
	}

	u32 GetImageCount()
	{
		return SwapInstance1.ImagesCount;
	}

	BMRRenderPass CreateRenderPass(const BMRRenderPassSettings* Settings)
	{
		HandleLog(BMRLogType::LogType_Info, "Initializing RenderPass, Name: %s, Subpass count: %d, Attachments count: %d",
			Settings->RenderPassName, Settings->SubpassSettingsCount, Settings->AttachmentDescriptionsCount);

		auto Subpasses = Memory::BmMemoryManagementSystem::FrameAlloc<VkSubpassDescription>(Settings->SubpassSettingsCount);
		for (u32 SubpassIndex = 0; SubpassIndex < Settings->SubpassSettingsCount; ++SubpassIndex)
		{
			const BMRSubpassSettings* SubpassSettings = Settings->SubpassesSettings + SubpassIndex;
			VkSubpassDescription* Subpass = Subpasses + SubpassIndex;

			HandleLog(BMRLogType::LogType_Info, "Initializing Subpass, Name: %s, Color attachments count: %d, "
				"Depth attachment state: %d, Input attachments count: %d",
				SubpassSettings->SubpassName, SubpassSettings->ColorAttachmentsReferencesCount,
				SubpassSettings->DepthAttachmentReferences ? 1 : 0, SubpassSettings->InputAttachmentsReferencesCount);

			*Subpass = { };
			Subpass->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			Subpass->pColorAttachments = SubpassSettings->ColorAttachmentsReferences;
			Subpass->colorAttachmentCount = SubpassSettings->ColorAttachmentsReferencesCount;
			Subpass->pDepthStencilAttachment = SubpassSettings->DepthAttachmentReferences;

			if (SubpassIndex != 0)
			{
				Subpass->pInputAttachments = SubpassSettings->InputAttachmentsReferences;
				Subpass->inputAttachmentCount = SubpassSettings->InputAttachmentsReferencesCount;
			}
			else if (SubpassSettings->InputAttachmentsReferences != nullptr || SubpassSettings->InputAttachmentsReferencesCount != 0)
			{
				HandleLog(BMRLogType::LogType_Warning, "Attempting to set Input attachment to first subpass");
			}
		}

		for (u32 AttachmentDescriptionIndex = 0; AttachmentDescriptionIndex < Settings->AttachmentDescriptionsCount; ++AttachmentDescriptionIndex)
		{
			if (Settings->AttachmentDescriptions[AttachmentDescriptionIndex].format == VK_FORMAT_UNDEFINED)
			{
				HandleLog(BMRLogType::LogType_Info, "Setting new %d format for attachment description with index %d",
					SurfaceFormat.format, AttachmentDescriptionIndex);
				Settings->AttachmentDescriptions[AttachmentDescriptionIndex].format = SurfaceFormat.format;
			}
		}

		VkRenderPassCreateInfo RenderPassCreateInfo = { };
		RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.attachmentCount = Settings->AttachmentDescriptionsCount;
		RenderPassCreateInfo.pAttachments = Settings->AttachmentDescriptions;
		RenderPassCreateInfo.subpassCount = Settings->SubpassSettingsCount;
		RenderPassCreateInfo.pSubpasses = Subpasses;
		RenderPassCreateInfo.dependencyCount = Settings->SubpassDependenciesCount;
		RenderPassCreateInfo.pDependencies = Settings->SubpassDependencies;

		BMRRenderPass RenderPass;
		const VkResult Result = vkCreateRenderPass(LogicalDevice, &RenderPassCreateInfo, nullptr, &RenderPass);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateRenderPass result is %d", Result);
		}

		return RenderPass;
	}

	void DestroyRenderPass(BMRRenderPass Pass)
	{
		vkDestroyRenderPass(LogicalDevice, Pass, nullptr);
	}

	BMRUniformBuffer CreateUniformBuffer(VkBufferUsageFlags Type, VkMemoryPropertyFlags Usage, VkDeviceSize Size)
	{
		BMRUniformBuffer UniformBuffer;
		UniformBuffer.Buffer = CreateBuffer(Size, Type, Usage);

		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(LogicalDevice, (VkBuffer)UniformBuffer.Buffer, &MemoryRequirements);

		const u32 MemoryTypeIndex = GetMemoryTypeIndex(Device.PhysicalDevice, MemoryRequirements.memoryTypeBits, Usage);

		if (Size != MemoryRequirements.size)
		{
			HandleLog(BMRLogType::LogType_Warning, "Buffer memory requirement size is %d, allocating %d more then buffer size",
				MemoryRequirements.size, MemoryRequirements.size - Size);
		}

		UniformBuffer.Memory = CreateDeviceMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindBufferMemory(LogicalDevice, UniformBuffer.Buffer, UniformBuffer.Memory, 0);

		return UniformBuffer;
	}

	void UpdateUniformBuffer(BMRUniformBuffer Buffer, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data)
	{
		void* MappedMemory;
		vkMapMemory(LogicalDevice, Buffer.Memory, Offset, DataSize, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, DataSize);
		vkUnmapMemory(LogicalDevice, Buffer.Memory);
	}

	BMRUniformImage CreateUniformImage(const VkImageCreateInfo* ImageCreateInfo)
	{
		BMRUniformImage Buffer;
		VkResult Result = vkCreateImage(LogicalDevice, ImageCreateInfo, nullptr, &Buffer.Image);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "CreateImage result is %d", Result);
		}

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(LogicalDevice, Buffer.Image, &MemoryRequirements);

		const u32 MemoryTypeIndex = GetMemoryTypeIndex(Device.PhysicalDevice, MemoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Buffer.Memory = AllocateMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindImageMemory(LogicalDevice, Buffer.Image, Buffer.Memory, 0);

		return Buffer;
	}

	void DestroyUniformImage(BMRUniformImage Image)
	{
		vkDestroyImage(LogicalDevice, Image.Image, nullptr);
		vkFreeMemory(LogicalDevice, Image.Memory, nullptr);
	}

	BMRUniformLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages, u32 Count)
	{
		auto LayoutBindings = Memory::BmMemoryManagementSystem::FrameAlloc<VkDescriptorSetLayoutBinding>(Count);
		for (u32 BindingIndex = 0; BindingIndex < Count; ++BindingIndex)
		{
			VkDescriptorSetLayoutBinding* LayoutBinding = LayoutBindings + BindingIndex;
			*LayoutBinding = { };
			LayoutBinding->binding = BindingIndex;
			LayoutBinding->descriptorType = Types[BindingIndex];
			LayoutBinding->descriptorCount = 1;
			LayoutBinding->stageFlags = Stages[BindingIndex];
			LayoutBinding->pImmutableSamplers = nullptr; // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout	
		}

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = Count;
		LayoutCreateInfo.pBindings = LayoutBindings;

		BMRUniformLayout Layout;
		const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo, nullptr, &Layout);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateDescriptorSetLayout result is %d", Result);
		}

		return Layout;
	}

	void DestroyUniformBuffer(BMRUniformBuffer Buffer)
	{
		vkDeviceWaitIdle(LogicalDevice); // TODO!!!!!!!!!!!



		vkDestroyBuffer(LogicalDevice, Buffer.Buffer, nullptr);
		vkFreeMemory(LogicalDevice, Buffer.Memory, nullptr);
	}

	void CreateUniformSets(const BMRUniformLayout* Layouts, u32 Count, BMRUniformSet* OutSets)
	{
		HandleLog(BMRLogType::LogType_Info, "Allocating descriptor sets. Size count: %d", Count);

		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = MainPool; // Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = Count; // Number of sets to allocate
		SetAllocInfo.pSetLayouts = Layouts; // Layouts to use to allocate sets (1:1 relationship)

		const VkResult Result = vkAllocateDescriptorSets(LogicalDevice, &SetAllocInfo, (VkDescriptorSet*)OutSets);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkAllocateDescriptorSets result is %d", Result);
		}
	}

	BMRImageInterface CreateImageInterface(const VkImageViewCreateInfo* ViewCreateInfo)
	{
		VkImageView ImageView;
		const VkResult Result = vkCreateImageView(LogicalDevice, ViewCreateInfo, nullptr, &ImageView);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateImageView result is %d", Result);
		}

		return ImageView;
	}

	void DestroyUniformLayout(BMRUniformLayout Layout)
	{
		vkDestroyDescriptorSetLayout(LogicalDevice, Layout, nullptr);
	}

	void DestroyImageInterface(BMRImageInterface Interface)
	{
		vkDestroyImageView(LogicalDevice, Interface, nullptr);
	}

	void AttachUniformsToSet(BMRUniformSet Set, const BMRUniformBuffer* Buffers, const VkDeviceSize* BuffersSizes, u32 BufferCount)
	{
		auto BufferInfos = Memory::BmMemoryManagementSystem::FrameAlloc<VkDescriptorBufferInfo>(BufferCount);
		auto SetWrites = Memory::BmMemoryManagementSystem::FrameAlloc<VkWriteDescriptorSet>(BufferCount);
		for (u32 BufferIndex = 0; BufferIndex < BufferCount; ++BufferIndex)
		{
			const BMRUniformBuffer* Buffer = Buffers + BufferIndex;

			VkDescriptorBufferInfo* BufferInfo = BufferInfos + BufferIndex;
			BufferInfo->buffer = Buffer->Buffer;
			BufferInfo->offset = 0;
			BufferInfo->range = BuffersSizes[BufferIndex];

			VkWriteDescriptorSet* SetWrite = SetWrites + BufferIndex;
			*SetWrite = { };
			SetWrite->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			SetWrite->dstSet = Set;
			SetWrite->dstBinding = 0;
			SetWrite->dstArrayElement = 0;
			SetWrite->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			SetWrite->descriptorCount = 1;
			SetWrite->pBufferInfo = BufferInfo;
		}

		vkUpdateDescriptorSets(LogicalDevice, BufferCount, SetWrites, 0, nullptr);
	}

	u32 LoadTexture(BMRTextureArrayInfo Info, BMRTextureType TextureType)
	{
		const VkImageViewType TextureImageType = ImageViewTypeTable[TextureType];

		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo = { };
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
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

		BMRUniformImage ImageBuffer = VulkanMemoryManagementSystem::CreateImageBuffer(&ImageCreateInfo);

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
		return UpdateTextureDescriptor(TextureIndices, 2, DescriptorLayoutHandles::EntitySampler);
	}

	u32 LoadTerrainMaterial(u32 DiffuseTextureIndex)
	{
		return UpdateTextureDescriptor(&DiffuseTextureIndex, 1, DescriptorLayoutHandles::TerrainSampler);
	}

	u32 LoadSkyBoxMaterial(u32 CubeTextureIndex)
	{
		return UpdateTextureDescriptor(&CubeTextureIndex, 1, DescriptorLayoutHandles::TerrainSampler);
	}

	u64 LoadVertices(const void* Vertices, u32 VertexSize, u64 VerticesCount)
	{
		assert(Vertices);

		const VkDeviceSize MeshVerticesSize = VertexSize * VerticesCount;
		const VkDeviceSize AlignedSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshVerticesSize);

		VulkanMemoryManagementSystem::CopyDataToBuffer(VertexBuffer.Buffer, VertexBufferOffset, MeshVerticesSize, Vertices);

		const VkDeviceSize CurrentOffset = VertexBufferOffset;
		VertexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	u64 LoadIndices(const u32* Indices, u32 IndicesCount)
	{
		assert(Indices);

		VkDeviceSize MeshIndicesSize = sizeof(u32) * IndicesCount;
		const VkDeviceSize AlignedSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(MeshIndicesSize);

		VulkanMemoryManagementSystem::CopyDataToBuffer(IndexBuffer.Buffer, IndexBufferOffset, MeshIndicesSize, Indices);

		const VkDeviceSize CurrentOffset = IndexBufferOffset;
		IndexBufferOffset += AlignedSize;

		return CurrentOffset;
	}

	void UpdateLightBuffer(const BMRLightBuffer* Buffer)
	{
		assert(Buffer);

		const u32 UpdateIndex = (MainRenderPass.ActiveLightSet + 1) % MainViewport.ViewportSwapchain.ImagesCount;

		UpdateUniformBuffer(EntityLight[UpdateIndex], sizeof(BMRLightBuffer), 0,
			Buffer);

		MainRenderPass.ActiveLightSet = UpdateIndex;
	}

	void UpdateMaterialBuffer(const BMRMaterial* Buffer)
	{
		assert(Buffer);
		VulkanMemoryManagementSystem::CopyDataToMemory(MainRenderPass.MaterialBuffer.Memory, 0,
			sizeof(BMRMaterial), Buffer);
	}

	void UpdateLightSpaceBuffer(const BMRLightSpaceMatrix* LightSpaceMatrix)
	{
		const u32 UpdateIndex = (MainRenderPass.ActiveLightSpaceMatrixSet + 1) % MainViewport.ViewportSwapchain.ImagesCount;

		UpdateUniformBuffer(LightSpaceBuffer[UpdateIndex], sizeof(BMRLightSpaceMatrix), 0,
			LightSpaceMatrix);

		MainRenderPass.ActiveLightSpaceMatrixSet = UpdateIndex;
	}

	void Draw(const BMRDrawScene& Scene)
	{
		// Todo Update only when changed
		UpdateLightBuffer(Scene.LightEntity);
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

		vkWaitForFences(LogicalDevice, 1, &Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(LogicalDevice, 1, &Fence);

		u32 ImageIndex;
		vkAcquireNextImageKHR(LogicalDevice, MainViewport.ViewportSwapchain.VulkanSwapchain, UINT64_MAX,
			ImageAvailable, VK_NULL_HANDLE, &ImageIndex);

		VkCommandBuffer CommandBuffer = MainViewport.CommandBuffers[ImageIndex];

		VkResult Result = vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkBeginCommandBuffer result is %d", Result);
		}

		{
			const BMRLightSpaceMatrix* LightViews[] =
			{
				&Scene.LightEntity->DirectionLight.LightSpaceMatrix,
				&Scene.LightEntity->SpotLight.LightSpaceMatrix,
			};

			const u32 FramebufferHandle[] =
			{
				FrameBuffersHandles::Tex1,
				FrameBuffersHandles::Tex2
			};

			for (u32 LightCaster = 0; LightCaster < MAX_LIGHT_SOURCES; ++LightCaster)
			{
				UpdateLightSpaceBuffer(LightViews[LightCaster]);

				VkRenderPassBeginInfo DepthRenderPassBeginInfo = { };
				DepthRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				DepthRenderPassBeginInfo.renderPass = DepthPass;
				DepthRenderPassBeginInfo.renderArea.offset = { 0, 0 };
				DepthRenderPassBeginInfo.pClearValues = DepthPassClearValues;
				DepthRenderPassBeginInfo.clearValueCount = DepthPassClearValuesSize;
				DepthRenderPassBeginInfo.renderArea.extent = DepthViewportExtent; // Size of region to run render pass on (starting at offset)
				DepthRenderPassBeginInfo.framebuffer = MainRenderPass.Framebuffers[FramebufferHandle[LightCaster]][ImageIndex];

				vkCmdBeginRenderPass(CommandBuffer, &DepthRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		
				vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Depth].Pipeline);

				for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
				{
					BMRDrawEntity* DrawEntity = Scene.DrawEntities + i;

					const VkBuffer VertexBuffers[] = { VertexBuffer.Buffer };
					const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

					const u32 DescriptorSetGroupCount = 1;
					const VkDescriptorSet DescriptorSetGroup[DescriptorSetGroupCount] =
					{
						LightSpaceSet[MainRenderPass.ActiveLightSpaceMatrixSet],
					};

					const VkPipelineLayout PipelineLayout = MainRenderPass.Pipelines[BMRPipelineHandles::Depth].PipelineLayout;

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
		}

		VkRenderPassBeginInfo MainRenderPassBeginInfo = { };
		MainRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		MainRenderPassBeginInfo.renderPass = MainPass;
		MainRenderPassBeginInfo.renderArea.offset = { 0, 0 };
		MainRenderPassBeginInfo.pClearValues = MainPassClearValues;
		MainRenderPassBeginInfo.clearValueCount = MainPassClearValuesSize;
		MainRenderPassBeginInfo.renderArea.extent = MainViewport.ViewportSwapchain.SwapExtent; // Size of region to run render pass on (starting at offset)
		MainRenderPassBeginInfo.framebuffer = MainRenderPass.Framebuffers[FrameBuffersHandles::Main][ImageIndex];

		vkCmdBeginRenderPass(CommandBuffer, &MainRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Terrain].Pipeline);

			for (u32 i = 0; i < Scene.DrawTerrainEntitiesCount; ++i)
			{
				BMRDrawTerrainEntity* DrawTerrainEntity = Scene.DrawTerrainEntities + i;

				const VkBuffer TerrainVertexBuffers[] = { VertexBuffer.Buffer };
				const VkDeviceSize TerrainBuffersOffsets[] = { DrawTerrainEntity->VertexOffset };

				const u32 TerrainDescriptorSetGroupCount = 2;
				const VkDescriptorSet TerrainDescriptorSetGroup[TerrainDescriptorSetGroupCount] = {
					VpSet[MainRenderPass.ActiveVpSet],
					MainRenderPass.SamplerDescriptors[DrawTerrainEntity->MaterialIndex]
				};

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Terrain].PipelineLayout,
					0, TerrainDescriptorSetGroupCount, TerrainDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, TerrainVertexBuffers, TerrainBuffersOffsets);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, DrawTerrainEntity->IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, DrawTerrainEntity->IndicesCount, 1, 0, 0, 0);
			}
		}

		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Entity].Pipeline);

			// TODO: Support rework to not create identical index buffers
			for (u32 i = 0; i < Scene.DrawEntitiesCount; ++i)
			{
				BMRDrawEntity* DrawEntity = Scene.DrawEntities + i;

				const VkBuffer VertexBuffers[] = { VertexBuffer.Buffer };
				const VkDeviceSize Offsets[] = { DrawEntity->VertexOffset };

				const VkDescriptorSet DescriptorSetGroup[] =
				{
					VpSet[MainRenderPass.ActiveVpSet],
					MainRenderPass.SamplerDescriptors[DrawEntity->MaterialIndex],
					EntityLightSet[MainRenderPass.ActiveLightSet],
					MainRenderPass.MaterialSet,
					MainRenderPass.DescriptorsToImages[DescriptorHandles::ShadowMapSampler][ImageIndex],
				};
				const u32 DescriptorSetGroupCount = sizeof(DescriptorSetGroup) / sizeof(DescriptorSetGroup[0]);

				const VkPipelineLayout PipelineLayout = MainRenderPass.Pipelines[BMRPipelineHandles::Entity].PipelineLayout;

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
				vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::SkyBox].Pipeline);

				const u32 SkyBoxDescriptorSetGroupCount = 2;
				const VkDescriptorSet SkyBoxDescriptorSetGroup[SkyBoxDescriptorSetGroupCount] = {
					VpSet[MainRenderPass.ActiveVpSet],
					MainRenderPass.SamplerDescriptors[Scene.SkyBox.MaterialIndex],
				};

				const VkPipelineLayout PipelineLayout = MainRenderPass.Pipelines[BMRPipelineHandles::SkyBox].PipelineLayout;

				vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout,
					0, SkyBoxDescriptorSetGroupCount, SkyBoxDescriptorSetGroup, 0, nullptr /*1, &DynamicOffset*/);

				vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertexBuffer.Buffer, &Scene.SkyBox.VertexOffset);
				vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer.Buffer, Scene.SkyBox.IndexOffset, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(CommandBuffer, Scene.SkyBox.IndicesCount, 1, 0, 0, 0);
			}
		}

		vkCmdNextSubpass(CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);

		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Deferred].Pipeline);

			vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderPass.Pipelines[BMRPipelineHandles::Deferred].PipelineLayout,
				0, 1, &MainRenderPass.DescriptorsToImages[DescriptorHandles::DeferredInputAttachments][ImageIndex], 0, nullptr);

			vkCmdDraw(CommandBuffer, 3, 1, 0, 0); // 3 hardcoded Indices for second "post processing" subpass
		}

		vkCmdEndRenderPass(CommandBuffer);

		Result = vkEndCommandBuffer(CommandBuffer);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkBeginCommandBuffer result is %d", Result);
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
			HandleLog(BMRLogType::LogType_Error, "vkQueueSubmit result is %d", Result);
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
			HandleLog(BMRLogType::LogType_Error, "vkQueuePresentKHR result is %d", Result);
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
				HandleLog(BMRLogType::LogType_Error, "Extension %s unsupported", RequiredExtensions[i]);
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
				HandleLog(BMRLogType::LogType_Error, "Validation layer %s unsupported", ValidationLeyersToCheck[i]);
				return false;
			}
		}

		return true;
	}

	VkExtent2D GetBestSwapExtent(const VkSurfaceCapabilitiesKHR& SurfaceCapabilities, HWND WindowHandler)
	{
		if (SurfaceCapabilities.currentExtent.width != UINT32_MAX)
		{
			return SurfaceCapabilities.currentExtent;
		}
		else
		{
			RECT Rect;
			if (!GetClientRect(WindowHandler, &Rect))
			{
				assert(false);
			}

			u32 Width = Rect.right - Rect.left;
			u32 Height = Rect.bottom - Rect.top;

			
			Width = glm::clamp(static_cast<u32>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
			Height = glm::clamp(static_cast<u32>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

			return { static_cast<u32>(Width), static_cast<u32>(Height) };
		}
	}

	Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties()
	{
		u32 Count;
		const VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkEnumerateInstanceExtensionProperties result is %d", static_cast<int>(Result));
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
			HandleLog(BMRLogType::LogType_Error, "vkEnumerateInstanceLayerProperties result is %d", Result);
		}

		auto Data = Memory::FrameArray<VkLayerProperties>::Create(Count);
		vkEnumerateInstanceLayerProperties(&Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<const char*> GetRequiredInstanceExtensions(const char** ValidationExtensions,
		u32 ValidationExtensionsCount)
	{
		auto Data = Memory::FrameArray<const char*>::Create(RequiredExtensionsCount + ValidationExtensionsCount);

		for (u32 i = 0; i < RequiredExtensionsCount; ++i)
		{
			Data[i] = RequiredInstanceExtensions[i];
			HandleLog(BMRLogType::LogType_Info, "Requested %s extension", Data[i]);
		}

		for (u32 i = 0; i < ValidationExtensionsCount; ++i)
		{
			Data[i + RequiredExtensionsCount] = ValidationExtensions[i];
			HandleLog(BMRLogType::LogType_Info, "Requested %s extension", Data[i]);
		}

		return Data;
	}

	Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR Surface)
	{
		u32 Count;
		const VkResult Result = vkGetPhysicalDeviceSurfaceFormatsKHR(Device.PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkGetPhysicalDeviceSurfaceFormatsKHR result is %d", Result);
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
			HandleLog(BMRLogType::LogType_Error, "vkCreateDevice result is %d", Result);
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
			HandleLog(BMRLogType::LogType_Error, "SurfaceFormat is undefined");
		}

		return Format;
	}

	VkSampler CreateTextureSampler(f32 MaxAnisotropy, VkSamplerAddressMode AddressMode)
	{
		VkSamplerCreateInfo SamplerCreateInfo = { };
		SamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
		SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
		SamplerCreateInfo.addressModeU = AddressMode;	// How to handle texture wrap in U (x) direction
		SamplerCreateInfo.addressModeV = AddressMode;	// How to handle texture wrap in V (y) direction
		SamplerCreateInfo.addressModeW = AddressMode;	// How to handle texture wrap in W (z) direction
		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
		SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
		SamplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
		SamplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
		SamplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
		SamplerCreateInfo.anisotropyEnable = VK_TRUE;
		SamplerCreateInfo.maxAnisotropy = MaxAnisotropy; // Todo: support in config

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
				HandleLog(BMRLogType::LogType_Error, "CreateSynchronisation error");
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

	void InitViewport(VkSurfaceKHR Surface, BMRViewportInstance* OutViewport,
		BMRSwapchainInstance SwapInstance, VkImageView* ColorBuffers, VkImageView* DepthBuffers)
	{
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
			HandleLog(BMRLogType::LogType_Error, "vkAllocateCommandBuffers result is %d", Result);
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
			HandleLog(BMRLogType::LogType_Error, "vkCreateCommandPool result is %d", Result);
		}
	}

	void UpdateVpBuffer(const BMRUboViewProjection& ViewProjection)
	{
		const u32 UpdateIndex = (MainRenderPass.ActiveVpSet + 1) % MainViewport.ViewportSwapchain.ImagesCount;

		UpdateUniformBuffer(VpBuffer[UpdateIndex], sizeof(BMRUboViewProjection), 0,
			&ViewProjection);

		MainRenderPass.ActiveVpSet = UpdateIndex;
	}

	VkBuffer CreateBuffer(VkDeviceSize BufferSize, VkBufferUsageFlags Usage,
		VkMemoryPropertyFlags Properties)
	{
		HandleLog(BMRLogType::LogType_Info, "Creating VkBuffer. Requested size: %d", BufferSize);

		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = BufferSize;
		BufferInfo.usage = Usage;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer Buffer;
		VkResult Result = vkCreateBuffer(LogicalDevice, &BufferInfo, nullptr, &Buffer);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkCreateBuffer result is %d", Result);
		}

		return Buffer;
	}

	VkDeviceMemory CreateDeviceMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex)
	{
		HandleLog(BMRLogType::LogType_Info, "Allocating Device memory. Buffer type: Image, Size count: %d, Index: %d",
			AllocationSize, MemoryTypeIndex);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = AllocationSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkDeviceMemory Memory;
		VkResult Result = vkAllocateMemory(LogicalDevice, &MemoryAllocInfo, nullptr, &Memory);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkAllocateMemory result is %d", Result);
		}

		return Memory;
	}

	VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex)
	{
		HandleLog(BMRLogType::LogType_Info, "Allocating Device memory. Buffer type: Image, Size count: %d, Index: %d",
			AllocationSize, MemoryTypeIndex);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = AllocationSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkDeviceMemory Memory;
		VkResult Result = vkAllocateMemory(LogicalDevice, &MemoryAllocInfo, nullptr, &Memory);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRLogType::LogType_Error, "vkAllocateMemory result is %d", Result);
		}

		return Memory;
	}
}
