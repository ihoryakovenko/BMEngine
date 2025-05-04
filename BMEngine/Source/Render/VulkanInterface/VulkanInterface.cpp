#include "VulkanInterface.h"

#include <cassert>
#include <cstring>
#include <stdio.h>
#include <stdarg.h>

#include <glm/glm.hpp>

#include "Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHalper.h"

namespace VulkanInterface
{
	// TYPES DECLARATION
	struct BMRMainInstance
	{
		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	};

	struct BMRPhysicalDeviceIndices
	{
		s32 GraphicsFamily = -1;
		s32 PresentationFamily = -1;
	};

	struct BMRDevice
	{
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;
		BMRPhysicalDeviceIndices Indices;
	};

	struct BMRSwapchain
	{
		VkSwapchainKHR VulkanSwapchain = nullptr;
		u32 ImagesCount = 0;
		VkImageView ImageViews[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkImage Images[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkExtent2D SwapExtent = { };
	};

	// INTERNAL FUNCTIONS DECLARATIONS
	static Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR Surface);
	static void SetupBestSurfaceFormat(VkSurfaceKHR Surface);
	static Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties();
	static Memory::FrameArray<VkLayerProperties> GetAvailableInstanceLayerProperties();
	static Memory::FrameArray<const char*> GetRequiredInstanceExtensions(const char** ValidationExtensions, u32 ValidationExtensionsCount);
	static Memory::FrameArray<VkPhysicalDevice> GetPhysicalDeviceList(VkInstance VulkanInstance);
	static Memory::FrameArray<VkExtensionProperties> GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice);
	static Memory::FrameArray<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice);
	static BMRPhysicalDeviceIndices GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
		VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	static void GetBestSwapExtent(Platform::BMRWindowHandler WindowHandler);
	static VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	static Memory::FrameArray<VkPresentModeKHR> GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface);
	static Memory::FrameArray<VkImage> GetSwapchainImages(VkDevice LogicalDevice, VkSwapchainKHR VulkanSwapchain);

	static bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
		const char** RequiredExtensions, u32 RequiredExtensionsCount);
	static bool CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
		const char** ValidationLeyersToCheck, u32 ValidationLeyersToCheckSize);
	static bool CheckFormats();
	static bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, BMRPhysicalDeviceIndices Indices,
		VkPhysicalDevice Device);
	static bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
		const char** ExtensionsToCheck, u32 ExtensionsToCheckSize);
	static bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

	static bool CreateMainInstance();
	static bool CreateDeviceInstance();
	static VkDevice CreateLogicalDevice(BMRPhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		u32 DeviceExtensionsSize);
	static void CreateSynchronisation();
	static bool SetupQueues();
	static bool CreateCommandPool(u32 FamilyIndex);
	static VkImageView CreateImageView(VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlags, VkImageViewType Type, u32 LayerCount, u32 BaseArrayLayer = 0);
	static bool CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
		BMRPhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
		VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent);
	static VkSwapchainKHR CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
		VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
		BMRPhysicalDeviceIndices DeviceIndices);
	bool CreateDrawCommandBuffers();
	static VkBuffer CreateBuffer(const VkBufferCreateInfo* BufferInfo);
	static VkDeviceMemory CreateDeviceMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);
	static bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger);

	static void DestroyMainInstance(BMRMainInstance& Instance);
	static void DestroySynchronisation();
	static void DestroySwapchainInstance(VkDevice LogicalDevice, BMRSwapchain& Instance);
	static bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
		const VkAllocationCallbacks* Allocator);

	static VkDescriptorPool AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 PoolSizeCount, u32 MaxDescriptorCount);
	

	static VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		void* UserData);

	static void HandleLog(LogType LogType, const char* Format, ...);

	// INTERNAL VARIABLES
	static BMRVkLogHandler LogHandler = nullptr;
	static VulkanInterfaceConfig Config;

	static const u32 RequiredExtensionsCount = 2;
	const char* RequiredInstanceExtensions[RequiredExtensionsCount] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		Platform::WinWindowExtension
	};

	static const u32 MAX_DRAW_FRAMES = 3;

	// Vulkan variables
	static BMRMainInstance Instance;
	static BMRDevice Device;
	static VkPhysicalDeviceProperties DeviceProperties;
	static VkSurfaceKHR Surface = nullptr;
	static VkSurfaceFormatKHR SurfaceFormat;
	static VkExtent2D SwapExtent;
	static VkSemaphore ImagesAvailable[MAX_DRAW_FRAMES];
	static VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
	static VkFence DrawFences[MAX_DRAW_FRAMES];
	static VkQueue GraphicsQueue = nullptr;
	static VkQueue PresentationQueue = nullptr;
	static VkCommandPool GraphicsCommandPool = nullptr;
	static BMRSwapchain SwapInstance;
	static VkDescriptorPool MainPool = nullptr;
	static VkCommandBuffer DrawCommandBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
	static VkCommandBuffer TransferCommandBuffer;
	static u32 CurrentFrame = 0;
	static u32 CurrentImageIndex = 0;

	// TODO: move StagingBuffer to external system
	static VulkanInterface::UniformBuffer StagingBuffer;
	

	// FUNCTIONS IMPLEMENTATIONS
	void Init(Platform::BMRWindowHandler WindowHandler, const VulkanInterfaceConfig& InConfig)
	{
		Config = InConfig;
		LogHandler = Config.LogHandler;

		CreateMainInstance();

		const VkResult Result = Platform::CreateSurface(WindowHandler, Instance.VulkanInstance, &Surface);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateWin32SurfaceKHR result is %d", Result);
		}

		CreateDeviceInstance();
		SetupBestSurfaceFormat(Surface);
		CheckFormats();
		GetBestSwapExtent(WindowHandler);
		CreateSynchronisation();
		SetupQueues();
		CreateCommandPool(Device.Indices.GraphicsFamily);
		CreateSwapchainInstance(Device.PhysicalDevice, Device.Indices, Device.LogicalDevice, Surface, SurfaceFormat, SwapExtent);
		CreateDrawCommandBuffers();

		// todo FIX!!!!!!!!!!!!!!!!
		const u32 PoolSizeCount = 13;
		auto TotalPassPoolSizes = Memory::FramePointer<VkDescriptorPoolSize>::Create(PoolSizeCount);
		u32 TotalDescriptorLayouts = 21;
		// Layout 1
		TotalPassPoolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapInstance.ImagesCount };
		// Layout 2
		TotalPassPoolSizes[1] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapInstance.ImagesCount };

		TotalPassPoolSizes[2] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapInstance.ImagesCount };

		TotalPassPoolSizes[3] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, SwapInstance.ImagesCount };
		// Layout 3
		TotalPassPoolSizes[4] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, SwapInstance.ImagesCount };
		TotalPassPoolSizes[5] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, SwapInstance.ImagesCount };
		// Textures
		TotalPassPoolSizes[6] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Config.MaxTextures };


		TotalPassPoolSizes[7] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapInstance.ImagesCount };
		TotalPassPoolSizes[8] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapInstance.ImagesCount };

		TotalPassPoolSizes[9] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Config.MaxTextures };
		TotalPassPoolSizes[10] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapInstance.ImagesCount };
		TotalPassPoolSizes[11] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Config.MaxTextures };
		TotalPassPoolSizes[12] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapInstance.ImagesCount };

		u32 TotalDescriptorCount = TotalDescriptorLayouts * SwapInstance.ImagesCount;
		TotalDescriptorCount += Config.MaxTextures;

		MainPool = AllocateDescriptorPool(TotalPassPoolSizes.Data, PoolSizeCount, TotalDescriptorCount);
		////////////////////

		VkBufferCreateInfo bufferInfo = { };
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = MB256;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		StagingBuffer = VulkanInterface::CreateUniformBufferInternal(&bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void DeInit()
	{
		vkDestroyDescriptorPool(Device.LogicalDevice, MainPool, nullptr);
		DestroyUniformBuffer(StagingBuffer);
		DestroySwapchainInstance(Device.LogicalDevice, SwapInstance);
		vkDestroyCommandPool(Device.LogicalDevice, GraphicsCommandPool, nullptr);
		DestroySynchronisation();
		vkDestroySurfaceKHR(Instance.VulkanInstance, Surface, nullptr);
		DestroyMainInstance(Instance);
	}

	VkPipelineLayout CreatePipelineLayout(const VkDescriptorSetLayout* DescriptorLayouts,
		u32 DescriptorLayoutsCount, const VkPushConstantRange* PushConstant, u32 PushConstantsCount)
	{
		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = DescriptorLayoutsCount;
		PipelineLayoutCreateInfo.pSetLayouts = DescriptorLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = PushConstantsCount;
		PipelineLayoutCreateInfo.pPushConstantRanges = PushConstant;

		VkPipelineLayout PipelineLayout;
		const VkResult Result = vkCreatePipelineLayout(Device.LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreatePipelineLayout result is %d", Result);
		}

		return PipelineLayout;
	}

	VkPipeline BatchPipelineCreation(const Shader* Shaders, u32 ShadersCount,
		const BMRVertexInputBinding* VertexInputBinding, u32 VertexInputBindingCount,
		const PipelineSettings* Settings, const PipelineResourceInfo* ResourceInfo)
	{
		HandleLog(BMRVkLogType_Info,
			"CREATING PIPELINE %s\n"
			"Extent - Width: %d, Height: %d\n"
			"DepthClampEnable: %d, RasterizerDiscardEnable: %d\n"
			"PolygonMode: %d, LineWidth: %f, CullMode: %d, FrontFace: %d, DepthBiasEnable: %d\n"
			"LogicOpEnable: %d, AttachmentCount: %d, ColorWriteMask: %d\n"
			"BlendEnable: %d, SrcColorBlendFactor: %d, DstColorBlendFactor: %d, ColorBlendOp: %d\n"
			"SrcAlphaBlendFactor: %d, DstAlphaBlendFactor: %d, AlphaBlendOp: %d\n"
			"DepthTestEnable: %d, DepthWriteEnable: %d, DepthCompareOp: %d\n"
			"DepthBoundsTestEnable: %d, StencilTestEnable: %d",
			Settings->PipelineName,
			Settings->Extent.width, Settings->Extent.height,
			Settings->DepthClampEnable, Settings->RasterizerDiscardEnable,
			Settings->PolygonMode, Settings->LineWidth, Settings->CullMode,
			Settings->FrontFace, Settings->DepthBiasEnable,
			Settings->LogicOpEnable, Settings->AttachmentCount, Settings->ColorWriteMask,
			Settings->BlendEnable, Settings->SrcColorBlendFactor,
			Settings->DstColorBlendFactor, Settings->ColorBlendOp,
			Settings->SrcAlphaBlendFactor, Settings->DstAlphaBlendFactor,
			Settings->AlphaBlendOp,
			Settings->DepthTestEnable, Settings->DepthWriteEnable,
			Settings->DepthCompareOp, Settings->DepthBoundsTestEnable,
			Settings->StencilTestEnable
		);

		HandleLog(BMRVkLogType_Info, "Creating VkPipelineShaderStageCreateInfo, ShadersCount: %d", ShadersCount);

		auto ShaderStageCreateInfos = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineShaderStageCreateInfo>(ShadersCount);
		for (u32 i = 0; i < ShadersCount; ++i)
		{
			HandleLog(BMRVkLogType_Info, "Shader #%d, Stage: %d", i, Shaders[i].Stage);

			VkPipelineShaderStageCreateInfo* ShaderStageCreateInfo = ShaderStageCreateInfos + i;
			*ShaderStageCreateInfo = { };
			ShaderStageCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			ShaderStageCreateInfo->stage = Shaders[i].Stage;
			ShaderStageCreateInfo->pName = "main";
			VulkanInterface::CreateShader((const u32*)Shaders[i].Code, Shaders[i].CodeSize, ShaderStageCreateInfo->module);
		}

		// INPUTASSEMBLY
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = { };
		InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		// MULTISAMPLING
		VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = { };
		MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
		MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

		// VERTEX INPUT
		auto VertexInputBindings = Memory::BmMemoryManagementSystem::FrameAlloc<VkVertexInputBindingDescription>(VertexInputBindingCount);
		auto VertexInputAttributes = Memory::BmMemoryManagementSystem::FrameAlloc<VkVertexInputAttributeDescription>(VertexInputBindingCount * MAX_VERTEX_INPUTS_ATTRIBUTES);
		u32 VertexInputAttributesIndex = 0;

		HandleLog(BMRVkLogType_Info, "Creating vertex input, VertexInputBindingCount: %d", VertexInputBindingCount);

		for (u32 BindingIndex = 0; BindingIndex < VertexInputBindingCount; ++BindingIndex)
		{
			const BMRVertexInputBinding* BMRBinding = VertexInputBinding + BindingIndex;
			VkVertexInputBindingDescription* VertexInputBinding = VertexInputBindings + BindingIndex;

			VertexInputBinding->binding = BindingIndex;
			VertexInputBinding->inputRate = BMRBinding->InputRate;
			VertexInputBinding->stride = BMRBinding->Stride;

			HandleLog(BMRVkLogType_Info, "Initialized VkVertexInputBindingDescription, BindingName: %s, "
				"BindingIndex: %d, VkInputRate: %d, Stride: %d, InputAttributesCount: %d",
				BMRBinding->VertexInputBindingName, VertexInputBinding->binding, VertexInputBinding->inputRate,
				VertexInputBinding->stride, BMRBinding->InputAttributesCount);

			for (u32 CurrentAttributeIndex = 0; CurrentAttributeIndex < BMRBinding->InputAttributesCount; ++CurrentAttributeIndex)
			{
				const VertexInputAttribute* BMRAttribute = BMRBinding->InputAttributes + CurrentAttributeIndex;
				VkVertexInputAttributeDescription* VertexInputAttribute = VertexInputAttributes + VertexInputAttributesIndex;

				VertexInputAttribute->binding = BindingIndex;
				VertexInputAttribute->location = CurrentAttributeIndex;
				VertexInputAttribute->format = BMRAttribute->Format;
				VertexInputAttribute->offset = BMRAttribute->AttributeOffset;

				HandleLog(BMRVkLogType_Info, "Initialized VkVertexInputAttributeDescription, "
					"AttributeName: %s, BindingIndex: %d, Location: %d, VkFormat: %d, Offset: %d, Index in creation array: %d",
					BMRAttribute->VertexInputAttributeName, BindingIndex, CurrentAttributeIndex,
					VertexInputAttribute->format, VertexInputAttribute->offset, VertexInputAttributesIndex);

				++VertexInputAttributesIndex;
			}
		}
		auto VertexInputInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineVertexInputStateCreateInfo>();
		*VertexInputInfo = { };
		VertexInputInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputInfo->vertexBindingDescriptionCount = VertexInputBindingCount;
		VertexInputInfo->pVertexBindingDescriptions = VertexInputBindings;
		VertexInputInfo->vertexAttributeDescriptionCount = VertexInputAttributesIndex;
		VertexInputInfo->pVertexAttributeDescriptions = VertexInputAttributes;

		// VIEWPORT
		auto Viewport = Memory::BmMemoryManagementSystem::FrameAlloc<VkViewport>();
		Viewport->width = Settings->Extent.width;
		Viewport->height = Settings->Extent.height;
		Viewport->minDepth = 0.0f;
		Viewport->maxDepth = 1.0f;
		Viewport->x = 0.0f;
		Viewport->y = 0.0f;

		auto Scissor = Memory::BmMemoryManagementSystem::FrameAlloc<VkRect2D>();
		Scissor->extent.width = Settings->Extent.width;
		Scissor->extent.height = Settings->Extent.height;
		Scissor->offset = { };

		auto ViewportStateCreateInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineViewportStateCreateInfo>();
		*ViewportStateCreateInfo = { };
		ViewportStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		ViewportStateCreateInfo->viewportCount = 1;
		ViewportStateCreateInfo->pViewports = Viewport;
		ViewportStateCreateInfo->scissorCount = 1;
		ViewportStateCreateInfo->pScissors = Scissor;

		// RASTERIZATION
		auto RasterizationStateCreateInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineRasterizationStateCreateInfo>();
		*RasterizationStateCreateInfo = { };
		RasterizationStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		RasterizationStateCreateInfo->depthClampEnable = Settings->DepthClampEnable;
		// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		RasterizationStateCreateInfo->rasterizerDiscardEnable = Settings->RasterizerDiscardEnable;
		RasterizationStateCreateInfo->polygonMode = Settings->PolygonMode;
		RasterizationStateCreateInfo->lineWidth = Settings->LineWidth;
		RasterizationStateCreateInfo->cullMode = Settings->CullMode;
		RasterizationStateCreateInfo->frontFace = Settings->FrontFace;
		// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
		RasterizationStateCreateInfo->depthBiasEnable = Settings->DepthBiasEnable;

		// COLOR BLENDING
		// Colors to apply blending to
		auto ColorBlendAttachmentState = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineColorBlendAttachmentState>();
		ColorBlendAttachmentState->colorWriteMask = Settings->ColorWriteMask;
		ColorBlendAttachmentState->blendEnable = Settings->BlendEnable;
		// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)// Enable blending
		ColorBlendAttachmentState->srcColorBlendFactor = Settings->SrcColorBlendFactor;
		ColorBlendAttachmentState->dstColorBlendFactor = Settings->DstColorBlendFactor;
		ColorBlendAttachmentState->colorBlendOp = Settings->ColorBlendOp;
		ColorBlendAttachmentState->srcAlphaBlendFactor = Settings->SrcAlphaBlendFactor;
		// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
//			   (new color alpha * new color) + ((1 - new color alpha) * old color)
		ColorBlendAttachmentState->dstAlphaBlendFactor = Settings->DstAlphaBlendFactor;
		ColorBlendAttachmentState->alphaBlendOp = Settings->AlphaBlendOp;
		// Summarised: (1 * new alpha) + (0 * old alpha) = new alpharesult != VK_SUCCESS

		auto ColorBlendInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineColorBlendStateCreateInfo>();
		*ColorBlendInfo = { };
		ColorBlendInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		ColorBlendInfo->logicOpEnable = Settings->LogicOpEnable; // Alternative to calculations is to use logical operations
		ColorBlendInfo->attachmentCount = Settings->AttachmentCount;
		ColorBlendInfo->pAttachments = Settings->AttachmentCount > 0 ? ColorBlendAttachmentState : nullptr;

		// DEPTH STENCIL
		auto DepthStencilInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineDepthStencilStateCreateInfo>();
		*DepthStencilInfo = { };
		DepthStencilInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		DepthStencilInfo->depthTestEnable = Settings->DepthTestEnable;
		DepthStencilInfo->depthWriteEnable = Settings->DepthWriteEnable;
		DepthStencilInfo->depthCompareOp = Settings->DepthCompareOp;
		DepthStencilInfo->depthBoundsTestEnable = Settings->DepthBoundsTestEnable;
		DepthStencilInfo->stencilTestEnable = Settings->StencilTestEnable;

		VkPipelineRenderingCreateInfo RenderingInfo = { };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		RenderingInfo.pNext = nullptr;
		RenderingInfo.colorAttachmentCount = ResourceInfo->PipelineAttachmentData.ColorAttachmentCount;
		RenderingInfo.pColorAttachmentFormats = ResourceInfo->PipelineAttachmentData.ColorAttachmentFormats;
		RenderingInfo.depthAttachmentFormat = ResourceInfo->PipelineAttachmentData.DepthAttachmentFormat;
		RenderingInfo.stencilAttachmentFormat = ResourceInfo->PipelineAttachmentData.DepthAttachmentFormat;

		// CREATE INFO
		auto PipelineCreateInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkGraphicsPipelineCreateInfo>();
		PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		PipelineCreateInfo->stageCount = ShadersCount;
		PipelineCreateInfo->pStages = ShaderStageCreateInfos;
		PipelineCreateInfo->pVertexInputState = VertexInputInfo;
		PipelineCreateInfo->pInputAssemblyState = &InputAssemblyStateCreateInfo;
		PipelineCreateInfo->pViewportState = ViewportStateCreateInfo;
		PipelineCreateInfo->pDynamicState = nullptr;
		PipelineCreateInfo->pRasterizationState = RasterizationStateCreateInfo;
		PipelineCreateInfo->pMultisampleState = &MultisampleStateCreateInfo;
		PipelineCreateInfo->pColorBlendState = ColorBlendInfo;
		PipelineCreateInfo->pDepthStencilState = DepthStencilInfo;
		PipelineCreateInfo->layout = ResourceInfo->PipelineLayout;
		PipelineCreateInfo->renderPass = nullptr;
		PipelineCreateInfo->subpass = 0;
		PipelineCreateInfo->pNext = &RenderingInfo;

		// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
		PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
		PipelineCreateInfo->basePipelineIndex = -1;

		VkPipeline Pipeline;
		const VkResult Result = vkCreateGraphicsPipelines(Device.LogicalDevice, VK_NULL_HANDLE, 1,
			PipelineCreateInfo, nullptr, &Pipeline);

		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateGraphicsPipelines result is %d", Result);
		}

		for (u32 j = 0; j < ShadersCount; ++j)
		{
			VulkanInterface::DestroyShader(ShaderStageCreateInfos[j].module);
		}

		return Pipeline;
	}

	u32 GetImageCount()
	{
		return SwapInstance.ImagesCount;
	}

	VkImageView* GetSwapchainImageViews()
	{
		return SwapInstance.ImageViews;
	}

	VkImage* GetSwapchainImages()
	{
		return SwapInstance.Images;
	}

	u32 AcquireNextImageIndex()
	{
		const VkFence* Fence = DrawFences + CurrentFrame;
		const VkSemaphore ImageAvailable = ImagesAvailable[CurrentFrame];

		vkWaitForFences(Device.LogicalDevice, 1, Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(Device.LogicalDevice, 1, Fence);

		vkAcquireNextImageKHR(Device.LogicalDevice, SwapInstance.VulkanSwapchain, UINT64_MAX,
			ImageAvailable, VK_NULL_HANDLE, &CurrentImageIndex);

		return CurrentImageIndex;
	}

	u32 TestGetImageIndex()
	{
		return CurrentImageIndex;
	}

	VkCommandBuffer BeginDraw(u32 ImageIndex)
	{
		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkCommandBuffer CommandBuffer = DrawCommandBuffers[ImageIndex];
		VkResult Result = vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkBeginCommandBuffer result is %d", Result);
		}

		return CommandBuffer;
	}

	void EndDraw(u32 ImageIndex)
	{
		VkResult Result = vkEndCommandBuffer(DrawCommandBuffers[ImageIndex]);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkBeginCommandBuffer result is %d", Result);
		}

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitDstStageMask = WaitStages;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = ImagesAvailable + CurrentFrame;
		SubmitInfo.pCommandBuffers = DrawCommandBuffers + ImageIndex;
		SubmitInfo.pSignalSemaphores = &RenderFinished[CurrentFrame];

		Result = vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, DrawFences[CurrentFrame]);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkQueueSubmit result is %d", Result);
			assert(false);
		}

		VkPresentInfoKHR PresentInfo = { };
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pWaitSemaphores = &RenderFinished[CurrentFrame];
		PresentInfo.pSwapchains = &SwapInstance.VulkanSwapchain; // Swapchains to present images to
		PresentInfo.pImageIndices = &ImageIndex; // Index of images in swapchains to present
		Result = vkQueuePresentKHR(PresentationQueue, &PresentInfo);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkQueuePresentKHR result is %d", Result);
		}

		CurrentFrame = (CurrentFrame + 1) % MAX_DRAW_FRAMES;
	}

	void DestroyPipelineLayout(VkPipelineLayout Layout)
	{
		vkDestroyPipelineLayout(Device.LogicalDevice, Layout, nullptr);
	}

	void DestroyPipeline(VkPipeline Pipeline)
	{
		vkDestroyPipeline(Device.LogicalDevice, Pipeline, nullptr);
	}

	UniformBuffer CreateUniformBufferInternal(const VkBufferCreateInfo* BufferInfo, VkMemoryPropertyFlags Properties)
	{
		UniformBuffer UniformBuffer;
		UniformBuffer.Buffer = CreateBuffer(BufferInfo);

		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device.LogicalDevice, UniformBuffer.Buffer, &MemoryRequirements);

		const u32 MemoryTypeIndex = VulkanHelper::GetMemoryTypeIndex(Device.PhysicalDevice, MemoryRequirements.memoryTypeBits, Properties);

		if (BufferInfo->size != MemoryRequirements.size)
		{
			HandleLog(BMRVkLogType_Warning, "Buffer memory requirement size is %d, allocating %d more then buffer size",
				MemoryRequirements.size, MemoryRequirements.size - BufferInfo->size);
		}

		UniformBuffer.Memory = CreateDeviceMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindBufferMemory(Device.LogicalDevice, UniformBuffer.Buffer, UniformBuffer.Memory, 0);

		UniformBuffer.Size = MemoryRequirements.size;
		return UniformBuffer;
	}

	void UpdateUniformBuffer(UniformBuffer Buffer, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data)
	{
		void* MappedMemory;
		vkMapMemory(Device.LogicalDevice, Buffer.Memory, Offset, DataSize, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, DataSize);
		vkUnmapMemory(Device.LogicalDevice, Buffer.Memory);
	}

	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, u32 LayersCount, void* Data, u32 Baselayer)
	{
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device.LogicalDevice, Image, &MemoryRequirements);

		//CopyDataToImage(Image, Width, Height,
		//	Format, MemoryRequirements.size / LayersCount, LayersCount, Data);

		CopyDataToImage(Image, Width, Height,
			Format, Width * Height * Format, LayersCount, Data, Baselayer);
	}

	void TransitImageLayout(VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout,
		VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask,
		VkPipelineStageFlags SrcStage, VkPipelineStageFlags DstStage,
		LayoutLayerTransitionData* LayerData, u32 LayerDataCount)
	{
		// TODO: Create system for commandBuffers and Barriers?
		auto Barriers = Memory::BmMemoryManagementSystem::FrameAlloc<VkImageMemoryBarrier>(LayerDataCount);

		for (u32 i = 0; i < LayerDataCount; ++i)
		{
			VkImageMemoryBarrier* Barrier = Barriers + i;
			*Barrier = { };
			Barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			Barrier->oldLayout = OldLayout;									// Layout to transition from
			Barrier->newLayout = NewLayout;									// Layout to transition to
			Barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
			Barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
			Barrier->image = Image;											// Image being accessed and modified as part of barrier
			Barrier->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
			Barrier->subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
			Barrier->subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from baseMipLevel
			Barrier->subresourceRange.baseArrayLayer = LayerData[i].BaseArrayLayer;
			Barrier->subresourceRange.layerCount = LayerData[i].LayerCount;
			Barrier->srcAccessMask = SrcAccessMask;								// Memory access stage transition must after...
			Barrier->dstAccessMask = DstAccessMask;		// Memory access stage transition must before...
		}


		VkCommandBufferBeginInfo BeginInfo = { };
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo);

		vkCmdPipelineBarrier(
			TransferCommandBuffer,
			SrcStage, DstStage,		// Pipeline stages (match to src and dst AccessMasks)
			0,						// Dependency flags
			0, nullptr,				// Memory Barrier count + data
			0, nullptr,				// Buffer Memory Barrier count + data
			LayerDataCount, Barriers	// Image Memory Barrier count + data
		);

		vkEndCommandBuffer(TransferCommandBuffer);

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);
	}

	void WaitDevice()
	{
		vkDeviceWaitIdle(Device.LogicalDevice);
	}

	VkSampler CreateSampler(const VkSamplerCreateInfo* CreateInfo)
	{
		VkSampler Sampler;
		VkResult Result = vkCreateSampler(Device.LogicalDevice, CreateInfo, nullptr, &Sampler);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateSampler result is %d", Result);
		}

		return Sampler;
	}

	UniformImage CreateUniformImage(const VkImageCreateInfo* ImageCreateInfo)
	{
		UniformImage Image;
		VkResult Result = vkCreateImage(Device.LogicalDevice, ImageCreateInfo, nullptr, &Image.Image);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "CreateImage result is %d", Result);
		}

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device.LogicalDevice, Image.Image, &MemoryRequirements);

		const u32 MemoryTypeIndex = VulkanHelper::GetMemoryTypeIndex(Device.PhysicalDevice, MemoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Image.Memory = AllocateMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindImageMemory(Device.LogicalDevice, Image.Image, Image.Memory, 0);

		Image.Size = MemoryRequirements.size;
		return Image;
	}

	void DestroyUniformImage(UniformImage Image)
	{
		vkDestroyImage(Device.LogicalDevice, Image.Image, nullptr);
		vkFreeMemory(Device.LogicalDevice, Image.Memory, nullptr);
	}

	VkDescriptorSetLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages,
		const VkDescriptorBindingFlags* BindingFlags, u32 BindingCount, u32 DescriptorCount)
	{
		auto LayoutBindings = Memory::BmMemoryManagementSystem::FrameAlloc<VkDescriptorSetLayoutBinding>(BindingCount);
		for (u32 BindingIndex = 0; BindingIndex < BindingCount; ++BindingIndex)
		{
			VkDescriptorSetLayoutBinding* LayoutBinding = LayoutBindings + BindingIndex;
			*LayoutBinding = { };
			LayoutBinding->binding = BindingIndex;
			LayoutBinding->descriptorType = Types[BindingIndex];
			LayoutBinding->descriptorCount = DescriptorCount;
			LayoutBinding->stageFlags = Stages[BindingIndex];
			LayoutBinding->pImmutableSamplers = nullptr; // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout	
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfo BindingFlagsInfo = { };
		BindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		BindingFlagsInfo.bindingCount = BindingCount;
		BindingFlagsInfo.pBindingFlags = BindingFlags;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = BindingCount;
		LayoutCreateInfo.pBindings = LayoutBindings;
		LayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

		VkDescriptorSetLayout Layout;
		const VkResult Result = vkCreateDescriptorSetLayout(Device.LogicalDevice, &LayoutCreateInfo, nullptr, &Layout);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateDescriptorSetLayout result is %d", Result);
		}

		return Layout;
	}

	void DestroyUniformBuffer(UniformBuffer Buffer)
	{
		vkDeviceWaitIdle(Device.LogicalDevice); // TODO FIX

		vkDestroyBuffer(Device.LogicalDevice, Buffer.Buffer, nullptr);
		vkFreeMemory(Device.LogicalDevice, Buffer.Memory, nullptr);
	}

	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32 Count, VkDescriptorSet* OutSets)
	{
		HandleLog(BMRVkLogType_Info, "Allocating descriptor sets. Size count: %d", Count);

		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = MainPool; // Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = Count; // Number of sets to allocate
		SetAllocInfo.pSetLayouts = Layouts; // Layouts to use to allocate sets (1:1 relationship)

		const VkResult Result = vkAllocateDescriptorSets(Device.LogicalDevice, &SetAllocInfo, (VkDescriptorSet*)OutSets);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkAllocateDescriptorSets result is %d", Result);
		}
	}

	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32* DescriptorCounts, u32 Count, VkDescriptorSet* OutSets)
	{
		HandleLog(BMRVkLogType_Info, "Allocating descriptor sets. Size count: %d", Count);

		VkDescriptorSetVariableDescriptorCountAllocateInfo CountInfo = { };
		CountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		CountInfo.descriptorSetCount = Count;
		CountInfo.pDescriptorCounts = DescriptorCounts;

		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = MainPool; // Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = Count; // Number of sets to allocate
		SetAllocInfo.pSetLayouts = Layouts; // Layouts to use to allocate sets (1:1 relationship)
		SetAllocInfo.pNext = &CountInfo;

		const VkResult Result = vkAllocateDescriptorSets(Device.LogicalDevice, &SetAllocInfo, (VkDescriptorSet*)OutSets);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkAllocateDescriptorSets result is %d", Result);
		}
	}

	VkImageView CreateImageInterface(const UniformImageInterfaceCreateInfo* InterfaceCreateInfo, VkImage Image)
	{
		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.image = Image;
		ViewCreateInfo.viewType = InterfaceCreateInfo->ViewType;
		ViewCreateInfo.format = InterfaceCreateInfo->Format;
		ViewCreateInfo.components = InterfaceCreateInfo->Components;
		ViewCreateInfo.subresourceRange = InterfaceCreateInfo->SubresourceRange;
		ViewCreateInfo.pNext = InterfaceCreateInfo->pNext;

		VkImageView ImageView;
		const VkResult Result = vkCreateImageView(Device.LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateImageView result is %d", Result);
		}

		return ImageView;
	}

	void DestroyUniformLayout(VkDescriptorSetLayout Layout)
	{
		vkDestroyDescriptorSetLayout(Device.LogicalDevice, Layout, nullptr);
	}

	void DestroyImageInterface(VkImageView Interface)
	{
		vkDestroyImageView(Device.LogicalDevice, Interface, nullptr);
	}

	void DestroySampler(VkSampler Sampler)
	{
		vkDestroySampler(Device.LogicalDevice, Sampler, nullptr);
	}

	void DestroyShader(VkShaderModule Shader)
	{
		vkDestroyShaderModule(Device.LogicalDevice, Shader, nullptr);
	}

	void AttachUniformsToSet(VkDescriptorSet Set, const UniformSetAttachmentInfo* Infos, u32 BufferCount)
	{
		auto SetWrites = Memory::BmMemoryManagementSystem::FrameAlloc<VkWriteDescriptorSet>(BufferCount);
		for (u32 BufferIndex = 0; BufferIndex < BufferCount; ++BufferIndex)
		{
			const UniformSetAttachmentInfo* Info = Infos + BufferIndex;

			VkWriteDescriptorSet* SetWrite = SetWrites + BufferIndex;
			*SetWrite = { };
			SetWrite->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			SetWrite->dstSet = Set;
			SetWrite->dstBinding = BufferIndex;
			SetWrite->dstArrayElement = 0;
			SetWrite->descriptorType = Info->Type;
			SetWrite->descriptorCount = 1;
			SetWrite->pBufferInfo = &Info->BufferInfo;
			SetWrite->pImageInfo = &Info->ImageInfo;
		}

		vkUpdateDescriptorSets(Device.LogicalDevice, BufferCount, SetWrites, 0, nullptr);
	}

	void CopyDataToBuffer(VkBuffer Buffer, VkDeviceSize Offset, VkDeviceSize Size, const void* Data)
	{
		assert(Size <= MB64);

		void* MappedMemory;
		vkMapMemory(Device.LogicalDevice, StagingBuffer.Memory, 0, Size, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, Size);
		vkUnmapMemory(Device.LogicalDevice, StagingBuffer.Memory);

		VkCommandBufferBeginInfo BeginInfo = { };
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// We're only using the command buffer once, so set up for one time submit
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(TransferCommandBuffer, &BeginInfo);

		VkBufferCopy BufferCopyRegion = { };
		BufferCopyRegion.srcOffset = 0;
		BufferCopyRegion.dstOffset = Offset;
		BufferCopyRegion.size = Size;

		vkCmdCopyBuffer(TransferCommandBuffer, StagingBuffer.Buffer, Buffer, 1, &BufferCopyRegion);
		vkEndCommandBuffer(TransferCommandBuffer);

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);
	}

	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, VkDeviceSize AlignedLayerSize,
		u32 LayersCount, void* Data, u32 Baselayer)
	{
		const VkDeviceSize TotalAlignedSize = AlignedLayerSize * LayersCount;
		assert(TotalAlignedSize <= MB256);

		const VkDeviceSize ActualLayerSize = Width * Height * Format; // Should be TotalAlignedSize in ideal (assert?)
		if (ActualLayerSize != AlignedLayerSize)
		{
			HandleLog(BMRVkLogType_Warning, "Image memory requirement size for layer is %d, actual size is %d",
				AlignedLayerSize, ActualLayerSize);
		}

		VkDeviceSize CopyOffset = 0;

		void* MappedMemory;
		vkMapMemory(Device.LogicalDevice, StagingBuffer.Memory, 0, TotalAlignedSize, 0, &MappedMemory);

		for (u32 i = 0; i < LayersCount; ++i)
		{
			std::memcpy(static_cast<u8*>(MappedMemory) + CopyOffset, reinterpret_cast<u8**>(Data)[i], ActualLayerSize);
			CopyOffset += AlignedLayerSize;
		}

		vkUnmapMemory(Device.LogicalDevice, StagingBuffer.Memory);

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
		ImageRegion.imageSubresource.baseArrayLayer = Baselayer;
		ImageRegion.imageSubresource.layerCount = LayersCount;
		ImageRegion.imageOffset = { 0, 0, 0 };
		ImageRegion.imageExtent = { Width, Height, 1 };

		// Todo copy multiple regions at once?
		vkCmdCopyBufferToImage(TransferCommandBuffer, StagingBuffer.Buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);

		vkEndCommandBuffer(TransferCommandBuffer);

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &TransferCommandBuffer;

		vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(GraphicsQueue);
	}

	// INTERNAL FUNCTIONS
	void HandleLog(LogType LogType, const char* Format, ...)
	{
		if (LogHandler != nullptr)
		{
			va_list Args;
			va_start(Args, Format);
			LogHandler(LogType, Format, Args);
			va_end(Args);
		}
	}

	Memory::FrameArray<VkExtensionProperties> GetAvailableExtensionProperties()
	{
		u32 Count;
		const VkResult Result = vkEnumerateInstanceExtensionProperties(nullptr, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkEnumerateInstanceExtensionProperties result is %d", static_cast<int>(Result));
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
			HandleLog(BMRVkLogType_Error, "vkEnumerateInstanceLayerProperties result is %d", Result);
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
			HandleLog(BMRVkLogType_Info, "Requested %s extension", Data[i]);
		}

		for (u32 i = 0; i < ValidationExtensionsCount; ++i)
		{
			Data[i + RequiredExtensionsCount] = ValidationExtensions[i];
			HandleLog(BMRVkLogType_Info, "Requested %s extension", Data[i + RequiredExtensionsCount]);
		}

		return Data;
	}

	bool CreateMainInstance()
	{
		//const char* ValidationLayers[] =
		//{
		//	"VK_LAYER_KHRONOS_validation",
		//	"VK_LAYER_LUNARG_monitor",
		//};
		//const u32 ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

		const char* ValidationExtensions[] =
		{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		const u32 ValidationExtensionsSize =  sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]);

		Memory::FrameArray<VkExtensionProperties> AvailableExtensions = GetAvailableExtensionProperties();
		Memory::FrameArray<const char*> RequiredExtensions = GetRequiredInstanceExtensions(ValidationExtensions, ValidationExtensionsSize);

		if (!CheckRequiredInstanceExtensionsSupport(AvailableExtensions.Pointer.Data, AvailableExtensions.Count,
			RequiredExtensions.Pointer.Data, RequiredExtensions.Count))
		{
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
		CreateInfo.enabledExtensionCount = RequiredExtensions.Count;
		CreateInfo.ppEnabledExtensionNames = RequiredExtensions.Pointer.Data;

		MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		MessengerCreateInfo.pfnUserCallback = MessengerDebugCallback;


		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateInstance result is %d", Result);
			return false;
		}

		const bool CreateDebugUtilsMessengerResult =
			CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);

		if (!CreateDebugUtilsMessengerResult)
		{
			HandleLog(BMRVkLogType_Error, "Cannot create debug messenger");
			return false;
		}

		return true;
	}

	void DestroyMainInstance(BMRMainInstance& Instance)
	{
		vkDestroyDevice(Device.LogicalDevice, nullptr);

		if (Instance.DebugMessenger != nullptr)
		{
			DestroyDebugMessenger(Instance.VulkanInstance, Instance.DebugMessenger, nullptr);
		}

		vkDestroyInstance(Instance.VulkanInstance, nullptr);
	}

	bool CreateDebugUtilsMessengerEXT(VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* CreateInfo,
		const VkAllocationCallbacks* Allocator, VkDebugUtilsMessengerEXT* InDebugMessenger)
	{
		auto CreateMessengerFunc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
		if (CreateMessengerFunc != nullptr)
		{
			const VkResult Result = CreateMessengerFunc(Instance, CreateInfo, Allocator, InDebugMessenger);
			if (Result != VK_SUCCESS)
			{
				HandleLog(BMRVkLogType_Error, "CreateMessengerFunc result is %d", Result);
				return false;
			}
		}
		else
		{
			HandleLog(BMRVkLogType_Error, "CreateMessengerFunc is nullptr");
			return false;
		}

		return true;
	}

	bool DestroyDebugMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT InDebugMessenger,
		const VkAllocationCallbacks* Allocator)
	{
		auto DestroyMessengerFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
		if (DestroyMessengerFunc != nullptr)
		{
			DestroyMessengerFunc(Instance, InDebugMessenger, Allocator);
			return true;
		}
		else
		{
			HandleLog(BMRVkLogType_Error, "DestroyMessengerFunc is nullptr");
			return false;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		[[maybe_unused]] void* UserData)
	{
		if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			HandleLog(BMRVkLogType_Error, CallbackData->pMessage);
		}
		else
		{
			HandleLog(BMRVkLogType_Info, CallbackData->pMessage);
		}

		return VK_FALSE;
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
				HandleLog(BMRVkLogType_Error, "Extension %s unsupported", RequiredExtensions[i]);
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
				HandleLog(BMRVkLogType_Error, "Validation layer %s unsupported", ValidationLeyersToCheck[i]);
				return false;
			}
		}

		return true;
	}

	bool CheckFormats()
	{
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

			HandleLog(BMRVkLogType_Warning, "Format %d is not supported", FormatToCheck);
		}

		if (!IsSupportedFormatFound)
		{
			HandleLog(BMRVkLogType_Error, "No supported format found");
			return false;
		}

		return true;
	}

	Memory::FrameArray<VkPhysicalDevice> GetPhysicalDeviceList(VkInstance VulkanInstance)
	{
		u32 Count;
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, nullptr);

		auto Data = Memory::FrameArray<VkPhysicalDevice>::Create(Count);
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkExtensionProperties> GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice)
	{
		u32 Count;
		const VkResult Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkEnumerateDeviceExtensionProperties result is %d", Result);
		}

		auto Data = Memory::FrameArray<VkExtensionProperties>::Create(Count);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, Data.Pointer.Data);

		return Data;
	}

	Memory::FrameArray<VkQueueFamilyProperties> GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice)
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
	BMRPhysicalDeviceIndices GetPhysicalDeviceIndices(VkQueueFamilyProperties* Properties, u32 PropertiesCount,
		VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		BMRPhysicalDeviceIndices Indices;

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

	bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, BMRPhysicalDeviceIndices Indices,
		VkPhysicalDevice Device)
	{
		VkPhysicalDeviceFeatures AvailableFeatures;
		vkGetPhysicalDeviceFeatures(Device, &AvailableFeatures);

		// TODO: Request vkGetPhysicalDeviceProperties in the caller
		vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

		HandleLog(BMRVkLogType_Info,
			"VkPhysicalDeviceProperties:\n"
			"  apiVersion: %u\n  driverVersion: %u\n  vendorID: %u\n  deviceID: %u\n"
			"  deviceType: %u\n  deviceName: %s\n"
			"  pipelineCacheUUID: %02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X\n"
			"  sparseProperties:\n    residencyStandard2DBlockShape: %u\n"
			"    residencyStandard2DMultisampleBlockShape: %u\n    residencyStandard3DBlockShape: %u\n"
			"    residencyAlignedMipSize: %u\n    residencyNonResidentStrict: %u",
			DeviceProperties.apiVersion,DeviceProperties.driverVersion,
			DeviceProperties.vendorID,DeviceProperties.deviceID,
			DeviceProperties.deviceType,DeviceProperties.deviceName,
			DeviceProperties.pipelineCacheUUID[0], DeviceProperties.pipelineCacheUUID[1], DeviceProperties.pipelineCacheUUID[2], DeviceProperties.pipelineCacheUUID[3],
			DeviceProperties.pipelineCacheUUID[4], DeviceProperties.pipelineCacheUUID[5], DeviceProperties.pipelineCacheUUID[6], DeviceProperties.pipelineCacheUUID[7],
			DeviceProperties.pipelineCacheUUID[8], DeviceProperties.pipelineCacheUUID[9], DeviceProperties.pipelineCacheUUID[10], DeviceProperties.pipelineCacheUUID[11],
			DeviceProperties.pipelineCacheUUID[12], DeviceProperties.pipelineCacheUUID[13], DeviceProperties.pipelineCacheUUID[14], DeviceProperties.pipelineCacheUUID[15],
			DeviceProperties.sparseProperties.residencyStandard2DBlockShape,
			DeviceProperties.sparseProperties.residencyStandard2DMultisampleBlockShape,
			DeviceProperties.sparseProperties.residencyStandard3DBlockShape,
			DeviceProperties.sparseProperties.residencyAlignedMipSize,
			DeviceProperties.sparseProperties.residencyNonResidentStrict
		);

		HandleLog(BMRVkLogType_Info,
			"VkPhysicalDeviceFeatures:\n  robustBufferAccess: %d\n  fullDrawIndexUint32: %d\n"
			"  imageCubeArray: %d\n  independentBlend: %d\n  geometryShader: %d\n"
			"  tessellationShader: %d\n  sampleRateShading: %d\n  dualSrcBlend: %d\n"
			"  logicOp: %d\n  multiDrawIndirect: %d\n  drawIndirectFirstInstance: %d\n"
			"  depthClamp: %d\n  depthBiasClamp: %d\n  fillModeNonSolid: %d\n"
			"  depthBounds: %d\n  wideLines: %d\n  largePoints: %d\n  alphaToOne: %d\n"
			"  multiViewport: %d\n  samplerAnisotropy: %d\n  textureCompressionETC2: %d\n"
			"  textureCompressionASTC_LDR: %d\n  textureCompressionBC: %d\n"
			"  occlusionQueryPrecise: %d\n  pipelineStatisticsQuery: %d\n"
			"  vertexPipelineStoresAndAtomics: %d\n  fragmentStoresAndAtomics: %d\n"
			"  shaderTessellationAndGeometryPointSize: %d\n  shaderImageGatherExtended: %d\n"
			"  shaderStorageImageExtendedFormats: %d\n  shaderStorageImageMultisample: %d\n"
			"  shaderStorageImageReadWithoutFormat: %d\n  shaderStorageImageWriteWithoutFormat: %d\n"
			"  shaderUniformBufferArrayDynamicIndexing: %d\n  shaderSampledImageArrayDynamicIndexing: %d\n"
			"  shaderStorageBufferArrayDynamicIndexing: %d\n  shaderStorageImageArrayDynamicIndexing: %d\n"
			"  shaderClipDistance: %d\n  shaderCullDistance: %d\n  shaderFloat64: %d\n"
			"  shaderInt64: %d\n  shaderInt16: %d\n  shaderResourceResidency: %d\n"
			"  shaderResourceMinLod: %d\n  sparseBinding: %d\n  sparseResidencyBuffer: %d\n"
			"  sparseResidencyImage2D: %d\n  sparseResidencyImage3D: %d\n  sparseResidency2Samples: %d\n"
			"  sparseResidency4Samples: %d\n  sparseResidency8Samples: %d\n  sparseResidency16Samples: %d\n"
			"  sparseResidencyAliased: %d\n  variableMultisampleRate: %d\n  inheritedQueries: %d",
			AvailableFeatures.robustBufferAccess,AvailableFeatures.fullDrawIndexUint32,AvailableFeatures.imageCubeArray,
			AvailableFeatures.independentBlend,AvailableFeatures.geometryShader,AvailableFeatures.tessellationShader,
			AvailableFeatures.sampleRateShading,AvailableFeatures.dualSrcBlend,AvailableFeatures.logicOp,
			AvailableFeatures.multiDrawIndirect,AvailableFeatures.drawIndirectFirstInstance,AvailableFeatures.depthClamp,
			AvailableFeatures.depthBiasClamp,AvailableFeatures.fillModeNonSolid,AvailableFeatures.depthBounds,
			AvailableFeatures.wideLines,AvailableFeatures.largePoints,AvailableFeatures.alphaToOne,
			AvailableFeatures.multiViewport,AvailableFeatures.samplerAnisotropy,AvailableFeatures.textureCompressionETC2,
			AvailableFeatures.textureCompressionASTC_LDR,AvailableFeatures.textureCompressionBC,
			AvailableFeatures.occlusionQueryPrecise,AvailableFeatures.pipelineStatisticsQuery,
			AvailableFeatures.vertexPipelineStoresAndAtomics,AvailableFeatures.fragmentStoresAndAtomics,
			AvailableFeatures.shaderTessellationAndGeometryPointSize,AvailableFeatures.shaderImageGatherExtended,
			AvailableFeatures.shaderStorageImageExtendedFormats,AvailableFeatures.shaderStorageImageMultisample,
			AvailableFeatures.shaderStorageImageReadWithoutFormat,AvailableFeatures.shaderStorageImageWriteWithoutFormat,
			AvailableFeatures.shaderUniformBufferArrayDynamicIndexing,AvailableFeatures.shaderSampledImageArrayDynamicIndexing,
			AvailableFeatures.shaderStorageBufferArrayDynamicIndexing,AvailableFeatures.shaderStorageImageArrayDynamicIndexing,
			AvailableFeatures.shaderClipDistance,AvailableFeatures.shaderCullDistance,
			AvailableFeatures.shaderFloat64,AvailableFeatures.shaderInt64,AvailableFeatures.shaderInt16,
			AvailableFeatures.shaderResourceResidency,AvailableFeatures.shaderResourceMinLod,
			AvailableFeatures.sparseBinding,AvailableFeatures.sparseResidencyBuffer,
			AvailableFeatures.sparseResidencyImage2D,AvailableFeatures.sparseResidencyImage3D,
			AvailableFeatures.sparseResidency2Samples,AvailableFeatures.sparseResidency4Samples,
			AvailableFeatures.sparseResidency8Samples,AvailableFeatures.sparseResidency16Samples,
			AvailableFeatures.sparseResidencyAliased,AvailableFeatures.variableMultisampleRate,
			AvailableFeatures.inheritedQueries
		);

		HandleLog(BMRVkLogType_Info,
			"VkPhysicalDeviceLimits:\n  maxImageDimension1D: %u\n  maxImageDimension2D: %u\n"
			"  maxImageDimension3D: %u\n  maxImageDimensionCube: %u\n"
			"  maxImageArrayLayers: %u\n  maxTexelBufferElements: %u\n"
			"  maxUniformBufferRange: %u\n  maxStorageBufferRange: %u\n"
			"  maxPushConstantsSize: %u\n  maxMemoryAllocationCount: %u\n"
			"  maxSamplerAllocationCount: %u\n  bufferImageGranularity: %llu\n"
			"  sparseAddressSpaceSize: %llu\n  maxBoundDescriptorSets: %u\n"
			"  maxPerStageDescriptorSamplers: %u\n  maxPerStageDescriptorUniformBuffers: %u\n"
			"  maxPerStageDescriptorStorageBuffers: %u\n  maxPerStageDescriptorSampledImages: %u\n"
			"  maxPerStageDescriptorStorageImages: %u\n  maxPerStageDescriptorInputAttachments: %u\n"
			"  maxPerStageResources: %u\n  maxDescriptorSetSamplers: %u\n"
			"  maxDescriptorSetUniformBuffers: %u\n  maxDescriptorSetUniformBuffersDynamic: %u\n"
			"  maxDescriptorSetStorageBuffers: %u\n  maxDescriptorSetStorageBuffersDynamic: %u\n"
			"  maxDescriptorSetSampledImages: %u\n  maxDescriptorSetStorageImages: %u\n"
			"  maxDescriptorSetInputAttachments: %u\n  maxVertexInputAttributes: %u\n"
			"  maxVertexInputBindings: %u\n  maxVertexInputAttributeOffset: %u\n"
			"  maxVertexInputBindingStride: %u\n  maxVertexOutputComponents: %u\n"
			"  maxTessellationGenerationLevel: %u\n  maxTessellationPatchSize: %u\n"
			"  maxTessellationControlPerVertexInputComponents: %u\n  maxTessellationControlPerVertexOutputComponents: %u\n"
			"  maxTessellationControlPerPatchOutputComponents: %u\n  maxTessellationControlTotalOutputComponents: %u\n"
			"  maxTessellationEvaluationInputComponents: %u\n  maxTessellationEvaluationOutputComponents: %u\n"
			"  maxGeometryShaderInvocations: %u\n  maxGeometryInputComponents: %u\n"
			"  maxGeometryOutputComponents: %u\n  maxGeometryOutputVertices: %u\n"
			"  maxGeometryTotalOutputComponents: %u\n  maxFragmentInputComponents: %u\n"
			"  maxFragmentOutputAttachments: %u\n  maxFragmentDualSrcAttachments: %u\n"
			"  maxFragmentCombinedOutputResources: %u\n  maxComputeSharedMemorySize: %u\n"
			"  maxComputeWorkGroupCount[0]: %u\n  maxComputeWorkGroupCount[1]: %u\n"
			"  maxComputeWorkGroupCount[2]: %u\n  maxComputeWorkGroupInvocations: %u\n"
			"  maxComputeWorkGroupSize[0]: %u\n  maxComputeWorkGroupSize[1]: %u\n"
			"  maxComputeWorkGroupSize[2]: %u\n  subPixelPrecisionBits: %u\n"
			"  subTexelPrecisionBits: %u\n  mipmapPrecisionBits: %u\n"
			"  maxDrawIndexedIndexValue: %u\n  maxDrawIndirectCount: %u\n"
			"  maxSamplerLodBias: %f\n  maxSamplerAnisotropy: %f\n"
			"  maxViewports: %u\n  maxViewportDimensions[0]: %u\n"
			"  maxViewportDimensions[1]: %u\n  viewportBoundsRange[0]: %f\n"
			"  viewportBoundsRange[1]: %f\n  viewportSubPixelBits: %u\n"
			"  minMemoryMapAlignment: %zu\n  minTexelBufferOffsetAlignment: %llu\n"
			"  minUniformBufferOffsetAlignment: %llu\n  minStorageBufferOffsetAlignment: %llu\n"
			"  minTexelOffset: %d\n  maxTexelOffset: %u\n"
			"  minTexelGatherOffset: %d\n  maxTexelGatherOffset: %u\n"
			"  minInterpolationOffset: %f\n  maxInterpolationOffset: %f\n"
			"  subPixelInterpolationOffsetBits: %u\n  maxFramebufferWidth: %u\n"
			"  maxFramebufferHeight: %u\n  maxFramebufferLayers: %u\n"
			"  framebufferColorSampleCounts: %u\n  framebufferDepthSampleCounts: %u\n"
			"  framebufferStencilSampleCounts: %u\n  framebufferNoAttachmentsSampleCounts: %u\n"
			"  maxColorAttachments: %u\n  sampledImageColorSampleCounts: %u\n"
			"  sampledImageIntegerSampleCounts: %u\n  sampledImageDepthSampleCounts: %u\n"
			"  sampledImageStencilSampleCounts: %u\n  storageImageSampleCounts: %u\n"
			"  maxSampleMaskWords: %u\n  timestampComputeAndGraphics: %d\n"
			"  timestampPeriod: %f\n  maxClipDistances: %u\n"
			"  maxCullDistances: %u\n  maxCombinedClipAndCullDistances: %u\n"
			"  discreteQueuePriorities: %u\n  pointSizeRange[0]: %f\n"
			"  pointSizeRange[1]: %f\n  lineWidthRange[0]: %f\n"
			"  lineWidthRange[1]: %f\n  pointSizeGranularity: %f\n"
			"  lineWidthGranularity: %f\n  strictLines: %d\n"
			"  standardSampleLocations: %d\n  optimalBufferCopyOffsetAlignment: %llu\n"
			"  optimalBufferCopyRowPitchAlignment: %llu\n  nonCoherentAtomSize: %llu",
			DeviceProperties.limits.maxImageDimension1D,DeviceProperties.limits.maxImageDimension2D,
			DeviceProperties.limits.maxImageDimension3D,DeviceProperties.limits.maxImageDimensionCube,
			DeviceProperties.limits.maxImageArrayLayers,DeviceProperties.limits.maxTexelBufferElements,
			DeviceProperties.limits.maxUniformBufferRange,DeviceProperties.limits.maxStorageBufferRange,
			DeviceProperties.limits.maxPushConstantsSize,DeviceProperties.limits.maxMemoryAllocationCount,
			DeviceProperties.limits.maxSamplerAllocationCount,DeviceProperties.limits.bufferImageGranularity,
			DeviceProperties.limits.sparseAddressSpaceSize,DeviceProperties.limits.maxBoundDescriptorSets,
			DeviceProperties.limits.maxPerStageDescriptorSamplers,DeviceProperties.limits.maxPerStageDescriptorUniformBuffers,
			DeviceProperties.limits.maxPerStageDescriptorStorageBuffers,DeviceProperties.limits.maxPerStageDescriptorSampledImages,
			DeviceProperties.limits.maxPerStageDescriptorStorageImages,DeviceProperties.limits.maxPerStageDescriptorInputAttachments,
			DeviceProperties.limits.maxPerStageResources,DeviceProperties.limits.maxDescriptorSetSamplers,
			DeviceProperties.limits.maxDescriptorSetUniformBuffers,DeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic,
			DeviceProperties.limits.maxDescriptorSetStorageBuffers,DeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic,
			DeviceProperties.limits.maxDescriptorSetSampledImages,DeviceProperties.limits.maxDescriptorSetStorageImages,
			DeviceProperties.limits.maxDescriptorSetInputAttachments,DeviceProperties.limits.maxVertexInputAttributes,
			DeviceProperties.limits.maxVertexInputBindings,DeviceProperties.limits.maxVertexInputAttributeOffset,
			DeviceProperties.limits.maxVertexInputBindingStride,DeviceProperties.limits.maxVertexOutputComponents,
			DeviceProperties.limits.maxTessellationGenerationLevel,DeviceProperties.limits.maxTessellationPatchSize,
			DeviceProperties.limits.maxTessellationControlPerVertexInputComponents,
			DeviceProperties.limits.maxTessellationControlPerVertexOutputComponents,
			DeviceProperties.limits.maxTessellationControlPerPatchOutputComponents,
			DeviceProperties.limits.maxTessellationControlTotalOutputComponents,
			DeviceProperties.limits.maxTessellationEvaluationInputComponents,
			DeviceProperties.limits.maxTessellationEvaluationOutputComponents,
			DeviceProperties.limits.maxGeometryShaderInvocations,DeviceProperties.limits.maxGeometryInputComponents,
			DeviceProperties.limits.maxGeometryOutputComponents,DeviceProperties.limits.maxGeometryOutputVertices,
			DeviceProperties.limits.maxGeometryTotalOutputComponents,DeviceProperties.limits.maxFragmentInputComponents,
			DeviceProperties.limits.maxFragmentOutputAttachments,DeviceProperties.limits.maxFragmentDualSrcAttachments,
			DeviceProperties.limits.maxFragmentCombinedOutputResources,DeviceProperties.limits.maxComputeSharedMemorySize,
			DeviceProperties.limits.maxComputeWorkGroupCount[0],DeviceProperties.limits.maxComputeWorkGroupCount[1],
			DeviceProperties.limits.maxComputeWorkGroupCount[2],DeviceProperties.limits.maxComputeWorkGroupInvocations,
			DeviceProperties.limits.maxComputeWorkGroupSize[0],DeviceProperties.limits.maxComputeWorkGroupSize[1],
			DeviceProperties.limits.maxComputeWorkGroupSize[2],DeviceProperties.limits.subPixelPrecisionBits,
			DeviceProperties.limits.subTexelPrecisionBits,DeviceProperties.limits.mipmapPrecisionBits,
			DeviceProperties.limits.maxDrawIndexedIndexValue,DeviceProperties.limits.maxDrawIndirectCount,
			DeviceProperties.limits.maxSamplerLodBias,DeviceProperties.limits.maxSamplerAnisotropy,
			DeviceProperties.limits.maxViewports,DeviceProperties.limits.maxViewportDimensions[0],
			DeviceProperties.limits.maxViewportDimensions[1],DeviceProperties.limits.viewportBoundsRange[0],
			DeviceProperties.limits.viewportBoundsRange[1],DeviceProperties.limits.viewportSubPixelBits,
			DeviceProperties.limits.minMemoryMapAlignment,DeviceProperties.limits.minTexelBufferOffsetAlignment,
			DeviceProperties.limits.minUniformBufferOffsetAlignment,DeviceProperties.limits.minStorageBufferOffsetAlignment,
			DeviceProperties.limits.minTexelOffset,DeviceProperties.limits.maxTexelOffset,
			DeviceProperties.limits.minTexelGatherOffset,DeviceProperties.limits.maxTexelGatherOffset,
			DeviceProperties.limits.minInterpolationOffset,DeviceProperties.limits.maxInterpolationOffset,
			DeviceProperties.limits.subPixelInterpolationOffsetBits,DeviceProperties.limits.maxFramebufferWidth,
			DeviceProperties.limits.maxFramebufferHeight,DeviceProperties.limits.maxFramebufferLayers,
			DeviceProperties.limits.framebufferColorSampleCounts,DeviceProperties.limits.framebufferDepthSampleCounts,
			DeviceProperties.limits.framebufferStencilSampleCounts,DeviceProperties.limits.framebufferNoAttachmentsSampleCounts,
			DeviceProperties.limits.maxColorAttachments,DeviceProperties.limits.sampledImageColorSampleCounts,
			DeviceProperties.limits.sampledImageIntegerSampleCounts,DeviceProperties.limits.sampledImageDepthSampleCounts,
			DeviceProperties.limits.sampledImageStencilSampleCounts,DeviceProperties.limits.storageImageSampleCounts,
			DeviceProperties.limits.maxSampleMaskWords,DeviceProperties.limits.timestampComputeAndGraphics,
			DeviceProperties.limits.timestampPeriod,DeviceProperties.limits.maxClipDistances,
			DeviceProperties.limits.maxCullDistances,DeviceProperties.limits.maxCombinedClipAndCullDistances,
			DeviceProperties.limits.discreteQueuePriorities,DeviceProperties.limits.pointSizeRange[0],
			DeviceProperties.limits.pointSizeRange[1],DeviceProperties.limits.lineWidthRange[0],
			DeviceProperties.limits.lineWidthRange[1],DeviceProperties.limits.pointSizeGranularity,
			DeviceProperties.limits.lineWidthGranularity,DeviceProperties.limits.strictLines,
			DeviceProperties.limits.standardSampleLocations,DeviceProperties.limits.optimalBufferCopyOffsetAlignment,
			DeviceProperties.limits.optimalBufferCopyRowPitchAlignment,DeviceProperties.limits.nonCoherentAtomSize
		);

		if (!CheckDeviceExtensionsSupport(ExtensionProperties, ExtensionPropertiesCount, DeviceExtensions, DeviceExtensionsSize))
		{
			HandleLog(BMRVkLogType_Warning, "PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (Indices.GraphicsFamily < 0 || Indices.PresentationFamily < 0)
		{
			HandleLog(BMRVkLogType_Warning, "PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (!AvailableFeatures.samplerAnisotropy)
		{
			HandleLog(BMRVkLogType_Warning, "Feature samplerAnisotropy is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			HandleLog(BMRVkLogType_Warning, "Feature multiViewport is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			HandleLog(BMRVkLogType_Warning, "Feature multiViewport is not supported");
			return false;
		}

		return true;
	}

	bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
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
				HandleLog(BMRVkLogType_Error, "extension %s unsupported", ExtensionsToCheck[i]);
				return false;
			}
		}

		return true;
	}

	bool CreateDeviceInstance()
	{
		const char* DeviceExtensions[] =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		};

		const u32 DeviceExtensionsSize = sizeof(DeviceExtensions) / sizeof(DeviceExtensions[0]);

		Memory::FrameArray<VkPhysicalDevice> DeviceList = GetPhysicalDeviceList(Instance.VulkanInstance);

		bool IsDeviceFound = false;
		for (u32 i = 0; i < DeviceList.Count; ++i)
		{
			Device.PhysicalDevice = DeviceList[i];

			Memory::FrameArray<VkExtensionProperties> DeviceExtensionsData = GetDeviceExtensionProperties(Device.PhysicalDevice);
			Memory::FrameArray<VkQueueFamilyProperties> FamilyPropertiesData = GetQueueFamilyProperties(Device.PhysicalDevice);

			Device.Indices = GetPhysicalDeviceIndices(FamilyPropertiesData.Pointer.Data, FamilyPropertiesData.Count, Device.PhysicalDevice, Surface);

			IsDeviceFound = CheckDeviceSuitability(DeviceExtensions, DeviceExtensionsSize,
				DeviceExtensionsData.Pointer.Data, DeviceExtensionsData.Count, Device.Indices, Device.PhysicalDevice);

			if (IsDeviceFound)
			{
				IsDeviceFound = true;
				break;
			}
		}

		if (!IsDeviceFound)
		{
			HandleLog(BMRVkLogType_Error, "Cannot find suitable device");
			return false;
		}

		Device.LogicalDevice = CreateLogicalDevice(Device.Indices, DeviceExtensions, DeviceExtensionsSize);
		if (Device.LogicalDevice == nullptr)
		{
			return false;
		}

		return true;
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

		// TODO: Check if supported
		VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeatures = { };
		DynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		DynamicRenderingFeatures.dynamicRendering = VK_TRUE;

		// TODO: Check if supported
		VkPhysicalDeviceSynchronization2Features Sync2Features = { };
		Sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		Sync2Features.synchronization2 = VK_TRUE;

		// TODO: Check if supported
		VkPhysicalDeviceDescriptorIndexingFeatures IndexingFeatures = { };
		IndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		IndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		IndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		IndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
		IndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

		Sync2Features.pNext = &IndexingFeatures;
		DynamicRenderingFeatures.pNext = &Sync2Features;

		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = FamilyIndicesSize;
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
		DeviceCreateInfo.enabledExtensionCount = DeviceExtensionsSize;
		DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions;
		DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;
		DeviceCreateInfo.pNext = &DynamicRenderingFeatures;

		VkDevice LogicalDevice;
		// Queues are created at the same time as the Device
		VkResult Result = vkCreateDevice(Device.PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateDevice result is %d", Result);
		}

		return LogicalDevice;
	}

	void SetupBestSurfaceFormat(VkSurfaceKHR Surface)
	{
		Memory::FrameArray<VkSurfaceFormatKHR> FormatsData = GetSurfaceFormats(Surface);

		SurfaceFormat = { VK_FORMAT_UNDEFINED, static_cast<VkColorSpaceKHR>(0) };

		// All formats available
		if (FormatsData.Count == 1 && FormatsData[0].format == VK_FORMAT_UNDEFINED)
		{
			SurfaceFormat = { VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}
		else
		{
			for (u32 i = 0; i < FormatsData.Count; ++i)
			{
				VkSurfaceFormatKHR AvailableFormat = FormatsData[i];
				if ((AvailableFormat.format == VK_FORMAT_R8G8B8_UNORM || AvailableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
					&& AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				{
					SurfaceFormat = AvailableFormat;
					break;
				}
			}
		}

		if (SurfaceFormat.format == VK_FORMAT_UNDEFINED)
		{
			HandleLog(BMRVkLogType_Error, "SurfaceFormat is undefined");
		}
	}

	Memory::FrameArray<VkSurfaceFormatKHR> GetSurfaceFormats(VkSurfaceKHR Surface)
	{
		u32 Count;
		const VkResult Result = vkGetPhysicalDeviceSurfaceFormatsKHR(Device.PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkGetPhysicalDeviceSurfaceFormatsKHR result is %d", Result);
		}

		auto Data = Memory::FrameArray<VkSurfaceFormatKHR>::Create(Count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device.PhysicalDevice, Surface, &Count, Data.Pointer.Data);

		return Data;
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

	void GetBestSwapExtent(Platform::BMRWindowHandler WindowHandler)
	{
		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };

		VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device.PhysicalDevice, Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Warning, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is %d", Result);
		}

		if (SurfaceCapabilities.currentExtent.width != UINT32_MAX)
		{
			SwapExtent = SurfaceCapabilities.currentExtent;
		}
		else
		{
			u32 Width;
			u32 Height;
			Platform::GetWindowSizes(WindowHandler, &Width, &Height);

			Width = glm::clamp(static_cast<u32>(Width), SurfaceCapabilities.minImageExtent.width, SurfaceCapabilities.maxImageExtent.width);
			Height = glm::clamp(static_cast<u32>(Height), SurfaceCapabilities.minImageExtent.height, SurfaceCapabilities.maxImageExtent.height);

			SwapExtent ={ static_cast<u32>(Width), static_cast<u32>(Height) };
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
			if (vkCreateSemaphore(Device.LogicalDevice, &SemaphoreCreateInfo, nullptr, &ImagesAvailable[i]) != VK_SUCCESS ||
				vkCreateSemaphore(Device.LogicalDevice, &SemaphoreCreateInfo, nullptr, &RenderFinished[i]) != VK_SUCCESS ||
				vkCreateFence(Device.LogicalDevice, &FenceCreateInfo, nullptr, &DrawFences[i]) != VK_SUCCESS)
			{
				HandleLog(BMRVkLogType_Error, "CreateSynchronisation error");
				assert(false);
			}
		}
	}

	void DestroySynchronisation()
	{
		for (u64 i = 0; i < MAX_DRAW_FRAMES; i++)
		{
			vkDestroySemaphore(Device.LogicalDevice, RenderFinished[i], nullptr);
			vkDestroySemaphore(Device.LogicalDevice, ImagesAvailable[i], nullptr);
			vkDestroyFence(Device.LogicalDevice, DrawFences[i], nullptr);
		}
	}

	bool SetupQueues()
	{
		vkGetDeviceQueue(Device.LogicalDevice, static_cast<u32>(Device.Indices.GraphicsFamily), 0, &GraphicsQueue);
		vkGetDeviceQueue(Device.LogicalDevice, static_cast<u32>(Device.Indices.PresentationFamily), 0, &PresentationQueue);

		if (GraphicsQueue == nullptr && PresentationQueue == nullptr)
		{
			return false;
		}
	}

	bool CreateCommandPool(u32 FamilyIndex)
	{
		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = FamilyIndex;

		VkResult Result = vkCreateCommandPool(Device.LogicalDevice, &PoolInfo, nullptr, &GraphicsCommandPool);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateCommandPool result is %d", Result);
			return false;
		}

		return true;
	}

	bool CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
		BMRPhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
		VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent)
	{
		SwapInstance.SwapExtent = Extent;

		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };
		VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Warning, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is %d", Result);
		}

		VkPresentModeKHR PresentationMode = GetBestPresentationMode(PhysicalDevice, Surface);

		SwapInstance.VulkanSwapchain = CreateSwapchain(LogicalDevice, SurfaceCapabilities, Surface, SurfaceFormat, SwapInstance.SwapExtent,
			PresentationMode, Indices);

		Memory::FrameArray<VkImage> Images = GetSwapchainImages(LogicalDevice, SwapInstance.VulkanSwapchain);

		SwapInstance.ImagesCount = Images.Count;

		for (u32 i = 0; i < SwapInstance.ImagesCount; ++i)
		{
			SwapInstance.Images[i] = Images[i];

			VkImageView ImageView = CreateImageView(Images[i],
				SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);
			if (ImageView == nullptr)
			{
				return false;
			}

			SwapInstance.ImageViews[i] = ImageView;
		}

		return true;
	}

	void DestroySwapchainInstance(VkDevice LogicalDevice, BMRSwapchain& Instance)
	{
		for (u32 i = 0; i < Instance.ImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, Instance.ImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(LogicalDevice, Instance.VulkanSwapchain, nullptr);
	}

	VkSwapchainKHR CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
		VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
		BMRPhysicalDeviceIndices DeviceIndices)
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
			HandleLog(BMRVkLogType_Error, "vkCreateSwapchainKHR result is %d", Result);
			assert(false);
		}

		return Swapchain;
	}

	bool CreateDrawCommandBuffers()
	{
		VkCommandBufferAllocateInfo GraphicsCommandBufferAllocateInfo = { };
		GraphicsCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		GraphicsCommandBufferAllocateInfo.commandPool = GraphicsCommandPool;
		GraphicsCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		GraphicsCommandBufferAllocateInfo.commandBufferCount = static_cast<u32>(SwapInstance.ImagesCount);

		VkResult Result = vkAllocateCommandBuffers(Device.LogicalDevice, &GraphicsCommandBufferAllocateInfo, DrawCommandBuffers);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkAllocateCommandBuffers result is %d", Result);
			return false;
		}

		VkCommandBufferAllocateInfo TransferCommandBufferAllocateInfo = { };
		TransferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		TransferCommandBufferAllocateInfo.commandPool = GraphicsCommandPool;
		TransferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		TransferCommandBufferAllocateInfo.commandBufferCount = 1;

		Result = vkAllocateCommandBuffers(Device.LogicalDevice, &TransferCommandBufferAllocateInfo, &TransferCommandBuffer);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkAllocateCommandBuffers result is %d", Result);
			return false;
		}

		return true;
	}

	VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
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
			HandleLog(BMRVkLogType_Warning, "Using default VK_PRESENT_MODE_FIFO_KHR");
		}

		return Mode;
	}

	Memory::FrameArray<VkPresentModeKHR> GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface)
	{
		u32 Count;
		const VkResult Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkGetPhysicalDeviceSurfacePresentModesKHR result is %d", Result);
			assert(false);
		}

		auto Data = Memory::FrameArray<VkPresentModeKHR>::Create(Count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, Data.Pointer.Data);

		for (u32 i = 0; i < Count; ++i)
		{
			HandleLog(BMRVkLogType_Info, "Present mode %d is available", Data[i]);
		}

		return Data;
	}

	Memory::FrameArray<VkImage> GetSwapchainImages(VkDevice LogicalDevice,
		VkSwapchainKHR VulkanSwapchain)
	{
		u32 Count;
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, nullptr);

		auto Data = Memory::FrameArray<VkImage>::Create(Count);
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, Data.Pointer.Data);

		return Data;
	}

	VkImageView CreateImageView(VkImage Image, VkFormat Format,
		VkImageAspectFlags AspectFlags, VkImageViewType Type, u32 LayerCount, u32 BaseArrayLayer)
	{
		VkImageViewCreateInfo ViewCreateInfo = { };
		ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCreateInfo.image = Image;
		ViewCreateInfo.viewType = Type;
		ViewCreateInfo.format = Format;
		ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;
		ViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ViewCreateInfo.subresourceRange.levelCount = 1;
		ViewCreateInfo.subresourceRange.baseArrayLayer = BaseArrayLayer;
		ViewCreateInfo.subresourceRange.layerCount = LayerCount;

		// Create image view and return it
		VkImageView ImageView;
		VkResult Result = vkCreateImageView(Device.LogicalDevice, &ViewCreateInfo, nullptr, &ImageView);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Info, "Can't create image view");
			return nullptr;
		}

		return ImageView;
	}

	VkBuffer CreateBuffer(const VkBufferCreateInfo* BufferInfo)
	{
		HandleLog(BMRVkLogType_Info, "Creating VkBuffer. Requested size: %d", BufferInfo->size);

		VkBuffer Buffer;
		VkResult Result = vkCreateBuffer(Device.LogicalDevice, BufferInfo, nullptr, &Buffer);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateBuffer result is %d", Result);
		}

		return Buffer;
	}

	VkDeviceMemory CreateDeviceMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex)
	{
		HandleLog(BMRVkLogType_Info, "Allocating Device memory. Buffer type: Image, Size count: %d, Index: %d",
			AllocationSize, MemoryTypeIndex);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = AllocationSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkDeviceMemory Memory;
		VkResult Result = vkAllocateMemory(Device.LogicalDevice, &MemoryAllocInfo, nullptr, &Memory);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkAllocateMemory result is %d", Result);
		}

		return Memory;
	}

	VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex)
	{
		HandleLog(BMRVkLogType_Info, "Allocating Device memory. Buffer type: Image, Size count: %d, Index: %d",
			AllocationSize, MemoryTypeIndex);

		VkMemoryAllocateInfo MemoryAllocInfo = { };
		MemoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		MemoryAllocInfo.allocationSize = AllocationSize;
		MemoryAllocInfo.memoryTypeIndex = MemoryTypeIndex;

		VkDeviceMemory Memory;
		VkResult Result = vkAllocateMemory(Device.LogicalDevice, &MemoryAllocInfo, nullptr, &Memory);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkAllocateMemory result is %d", Result);
		}

		return Memory;
	}



	bool CreateShader(const u32* Code, u32 CodeSize, VkShaderModule& VertexShaderModule)
	{
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = { };
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = CodeSize;
		ShaderModuleCreateInfo.pCode = Code;

		VkResult Result = vkCreateShaderModule(Device.LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateShaderModule result is %d", Result);
			return false;
		}

		return true;
	}

	VkDescriptorPool AllocateDescriptorPool(VkDescriptorPoolSize* PoolSizes, u32 PoolSizeCount, u32 MaxDescriptorCount)
	{
		HandleLog(BMRVkLogType_Info, "Creating descriptor pool. Size count: %d", PoolSizeCount);

		for (u32 i = 0; i < PoolSizeCount; ++i)
		{
			HandleLog(BMRVkLogType_Info, "Type: %d, Count: %d", PoolSizes[i].type, PoolSizes[i].descriptorCount);
		}

		HandleLog(BMRVkLogType_Info, "Maximum descriptor count: %d", MaxDescriptorCount);

		VkDescriptorPoolCreateInfo PoolCreateInfo = { };
		PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		PoolCreateInfo.maxSets = MaxDescriptorCount; // Maximum number of Descriptor Sets that can be created from pool
		PoolCreateInfo.poolSizeCount = PoolSizeCount; // Amount of Pool Sizes being passed
		PoolCreateInfo.pPoolSizes = PoolSizes; // Pool Sizes to create pool with
		PoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

		VkDescriptorPool Pool;
		VkResult Result = vkCreateDescriptorPool(Device.LogicalDevice, &PoolCreateInfo, nullptr, &Pool);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateDescriptorPool result is %d", Result);
		}

		return Pool;
	}

	UniformBuffer CreateUniformBuffer(u64 Size)
	{
		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		BufferInfo.size = Size;

		return VulkanInterface::CreateUniformBufferInternal(&BufferInfo,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	IndexBuffer CreateVertexBuffer(u64 Size)
	{
		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = Size;
		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		return VulkanInterface::CreateUniformBufferInternal(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	IndexBuffer CreateIndexBuffer(u64 Size)
	{
		VkBufferCreateInfo BufferInfo = { };
		BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferInfo.size = Size;
		BufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		return VulkanInterface::CreateUniformBufferInternal(&BufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	VkPhysicalDeviceProperties* GetDeviceProperties()
	{
		return &DeviceProperties;
	}

	VkDevice GetDevice()
	{
		return Device.LogicalDevice;
	}

	VkPhysicalDevice GetPhysicalDevice()
	{
		return Device.PhysicalDevice;
	}

	VkCommandBuffer GetCommandBuffer()
	{
		return DrawCommandBuffers[CurrentImageIndex];
	}

	VkCommandBuffer GetTransferCommandBuffer()
	{
		return TransferCommandBuffer;
	}

	VkQueue GetTransferQueue()
	{
		return GraphicsQueue;
	}

	VkFormat GetSurfaceFormat()
	{
		return SurfaceFormat.format;
	}

	u32 GetQueueGraphicsFamilyIndex()
	{
		return Device.Indices.GraphicsFamily;
	}

	VkDescriptorPool GetDescriptorPool()
	{
		return MainPool;
	}

	VkInstance GetInstance()
	{
		return Instance.VulkanInstance;
	}
}
