#include "BMRVulkan.h"

#include <cassert>
#include <cstring>

#include <glm/glm.hpp>

#include <vulkan/vulkan_win32.h>

#include "Memory/MemoryManagmentSystem.h"
#include "BMR/VulkanMemoryManagementSystem.h"

namespace BMR
{
	// TYPES DECLARATION
	struct BMRMainInstance
	{
		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
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
	static void GetBestSwapExtent(HWND WindowHandler);
	static VkPresentModeKHR GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface);
	static Memory::FrameArray<VkPresentModeKHR> GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice,
		VkSurfaceKHR Surface);
	static Memory::FrameArray<VkImage> GetSwapchainImages(VkDevice LogicalDevice, VkSwapchainKHR VulkanSwapchain);
	u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties);

	static bool CheckRequiredInstanceExtensionsSupport(VkExtensionProperties* AvailableExtensions, u32 AvailableExtensionsCount,
		const char** RequiredExtensions, u32 RequiredExtensionsCount);
	static bool CheckValidationLayersSupport(VkLayerProperties* Properties, u32 PropertiesSize,
		const char** ValidationLeyersToCheck, u32 ValidationLeyersToCheckSize);
	static bool CheckFormats();
	static bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, BMRPhysicalDeviceIndices Indices,
		VkPhysicalDeviceFeatures AvailableFeatures);
	static bool CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount,
		const char** ExtensionsToCheck, u32 ExtensionsToCheckSize);
	static bool IsFormatSupported(VkFormat Format, VkImageTiling Tiling, VkFormatFeatureFlags FeatureFlags);

	static bool CreateMainInstance();
	static bool CreateSurface(HWND WindowHandler);
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

	static VkDeviceMemory AllocateMemory(VkDeviceSize AllocationSize, u32 MemoryTypeIndex);

	static VKAPI_ATTR VkBool32 VKAPI_CALL MessengerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		void* UserData);

	static void HandleLog(BMRVkLogType LogType, const char* Format, ...);

	// INTERNAL VARIABLES
	static BMRVkLogHandler LogHandler = nullptr;
	static BMRVkConfig Config;

	static const u32 RequiredExtensionsCount = 2;
	const char* RequiredInstanceExtensions[RequiredExtensionsCount] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};

	// Vulkan variables
	static BMRMainInstance Instance;
	BMRDevice Device;
	static VkSurfaceKHR Surface = nullptr;
	VkSurfaceFormatKHR SurfaceFormat;
	VkExtent2D SwapExtent;
	VkSemaphore ImagesAvailable[MAX_DRAW_FRAMES];
	VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
	VkFence DrawFences[MAX_DRAW_FRAMES];
	VkQueue GraphicsQueue = nullptr;
	VkQueue PresentationQueue = nullptr;
	VkCommandPool GraphicsCommandPool = nullptr;
	BMRSwapchain SwapInstance;
	VkDescriptorPool MainPool = nullptr;
	VkCommandBuffer DrawCommandBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];
	

	// FUNCTIONS IMPLEMENTATIONS
	void BMRVkInit(HWND WindowHandler, const BMRVkConfig& InConfig)
	{
		Config = InConfig;
		LogHandler = Config.LogHandler;

		CreateMainInstance();
		CreateSurface(WindowHandler);
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
		TotalPassPoolSizes[0] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance.ImagesCount };
		// Layout 2
		TotalPassPoolSizes[1] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance.ImagesCount };

		TotalPassPoolSizes[2] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance.ImagesCount };

		TotalPassPoolSizes[3] = { .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = SwapInstance.ImagesCount };
		// Layout 3
		TotalPassPoolSizes[4] = { .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = SwapInstance.ImagesCount };
		TotalPassPoolSizes[5] = { .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = SwapInstance.ImagesCount };
		// Textures
		TotalPassPoolSizes[6] = { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = Config.MaxTextures };


		TotalPassPoolSizes[7] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance.ImagesCount };
		TotalPassPoolSizes[8] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance.ImagesCount };

		TotalPassPoolSizes[9] = { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = Config.MaxTextures };
		TotalPassPoolSizes[10] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance.ImagesCount };
		TotalPassPoolSizes[11] = { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = Config.MaxTextures };
		TotalPassPoolSizes[12] = { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SwapInstance.ImagesCount };

		u32 TotalDescriptorCount = TotalDescriptorLayouts * SwapInstance.ImagesCount;
		TotalDescriptorCount += Config.MaxTextures;


		VulkanMemoryManagementSystem::BMRMemorySourceDevice MemoryDevice;
		MemoryDevice.PhysicalDevice = Device.PhysicalDevice;
		MemoryDevice.LogicalDevice = Device.LogicalDevice;
		MemoryDevice.TransferCommandPool = GraphicsCommandPool;
		MemoryDevice.TransferQueue = GraphicsQueue;

		VulkanMemoryManagementSystem::Init(MemoryDevice);
		MainPool = VulkanMemoryManagementSystem::AllocateDescriptorPool(TotalPassPoolSizes.Data, PoolSizeCount, TotalDescriptorCount);
		////////////////////
	}

	void BMRVkDeInit()
	{
		vkDestroyDescriptorPool(Device.LogicalDevice, MainPool, nullptr);
		VulkanMemoryManagementSystem::Deinit();
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

	void CreatePipelines(const BMRSPipelineShaderInfo* ShaderInputs, const BMRVertexInput* VertexInputs,
		const BMRPipelineSettings* PipelinesSettings, const BMRPipelineResourceInfo* ResourceInfos,
		u32 PipelinesCount, VkPipeline* OutputPipelines)
	{
		auto PipelineCreateInfos = Memory::BmMemoryManagementSystem::FrameAlloc<VkGraphicsPipelineCreateInfo>(PipelinesCount);
		auto VertexInputInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineVertexInputStateCreateInfo>(PipelinesCount);
		auto ViewportStateCreateInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineViewportStateCreateInfo>(PipelinesCount);
		auto RasterizationStateCreateInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineRasterizationStateCreateInfo>(PipelinesCount);
		auto ColorBlendAttachmentState = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineColorBlendAttachmentState>(PipelinesCount);
		auto ColorBlendInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineColorBlendStateCreateInfo>(PipelinesCount);
		auto DepthStencilInfo = Memory::BmMemoryManagementSystem::FrameAlloc<VkPipelineDepthStencilStateCreateInfo>(PipelinesCount);
		auto Viewports = Memory::BmMemoryManagementSystem::FrameAlloc<VkViewport>(PipelinesCount);
		auto Scissors = Memory::BmMemoryManagementSystem::FrameAlloc<VkRect2D>(PipelinesCount);

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

		for (u32 PipelineIndex = 0; PipelineIndex < PipelinesCount; ++PipelineIndex)
		{
			const BMRVertexInput* VertexInput = VertexInputs + PipelineIndex;
			const BMRPipelineSettings* Settings = PipelinesSettings + PipelineIndex;
			VkViewport* Viewport = Viewports + PipelineIndex;
			VkRect2D* Scissor = Scissors + PipelineIndex;

			// VERTEX INPUT
			auto VertexInputBindings = Memory::BmMemoryManagementSystem::FrameAlloc<VkVertexInputBindingDescription>(VertexInput->VertexInputBindingCount);
			auto VertexInputAttributes = Memory::BmMemoryManagementSystem::FrameAlloc<VkVertexInputAttributeDescription>(VertexInput->VertexInputBindingCount * MAX_VERTEX_INPUTS_ATTRIBUTES);
			u32 VertexInputAttributesIndex = 0;

			HandleLog(BMR::BMRVkLogType_Info,
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

			HandleLog(BMR::BMRVkLogType_Info, "Creating vertex input, VertexInputBindingCount: %d", VertexInput->VertexInputBindingCount);

			for (u32 BindingIndex = 0; BindingIndex < VertexInput->VertexInputBindingCount; ++BindingIndex)
			{
				const BMRVertexInputBinding* BMRBinding = VertexInput->VertexInputBinding + BindingIndex;
				VkVertexInputBindingDescription* VertexInputBinding = VertexInputBindings + BindingIndex;

				VertexInputBinding->binding = BindingIndex;
				VertexInputBinding->inputRate = BMRBinding->InputRate;
				VertexInputBinding->stride = BMRBinding->Stride;

				HandleLog(BMR::BMRVkLogType_Info, "Initialized VkVertexInputBindingDescription, BindingName: %s, "
					"BindingIndex: %d, VkInputRate: %d, Stride: %d, InputAttributesCount: %d",
					BMRBinding->VertexInputBindingName, VertexInputBinding->binding, VertexInputBinding->inputRate,
					VertexInputBinding->stride, BMRBinding->InputAttributesCount);

				for (u32 CurrentAttributeIndex = 0; CurrentAttributeIndex < BMRBinding->InputAttributesCount; ++CurrentAttributeIndex)
				{
					const BMRVertexInputAttribute* BMRAttribute = BMRBinding->InputAttributes + CurrentAttributeIndex;
					VkVertexInputAttributeDescription* VertexInputAttribute = VertexInputAttributes + VertexInputAttributesIndex;

					VertexInputAttribute->binding = BindingIndex;
					VertexInputAttribute->location = CurrentAttributeIndex;
					VertexInputAttribute->format = BMRAttribute->Format;
					VertexInputAttribute->offset = BMRAttribute->AttributeOffset;

					HandleLog(BMR::BMRVkLogType_Info, "Initialized VkVertexInputAttributeDescription, "
						"AttributeName: %s, BindingIndex: %d, Location: %d, VkFormat: %d, Offset: %d, Index in creation array: %d",
						BMRAttribute->VertexInputAttributeName, BindingIndex, CurrentAttributeIndex,
						VertexInputAttribute->format, VertexInputAttribute->offset, VertexInputAttributesIndex);

					++VertexInputAttributesIndex;
				}
			}

			VertexInputInfo[PipelineIndex] = { };
			VertexInputInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			VertexInputInfo[PipelineIndex].vertexBindingDescriptionCount = VertexInput->VertexInputBindingCount;
			VertexInputInfo[PipelineIndex].pVertexBindingDescriptions = VertexInputBindings;
			VertexInputInfo[PipelineIndex].vertexAttributeDescriptionCount = VertexInputAttributesIndex;
			VertexInputInfo[PipelineIndex].pVertexAttributeDescriptions = VertexInputAttributes;

			// VIEWPORT
			Viewport->width = Settings->Extent.width;
			Viewport->height = Settings->Extent.height;
			Viewport->minDepth = 0.0f;
			Viewport->maxDepth = 1.0f;
			Viewport->x = 0.0f;
			Viewport->y = 0.0f;

			Scissor->extent.width = Settings->Extent.width;
			Scissor->extent.height = Settings->Extent.height;
			Scissor->offset = { };

			ViewportStateCreateInfo[PipelineIndex] = { };
			ViewportStateCreateInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			ViewportStateCreateInfo[PipelineIndex].viewportCount = 1;
			ViewportStateCreateInfo[PipelineIndex].pViewports = Viewport;
			ViewportStateCreateInfo[PipelineIndex].scissorCount = 1;
			ViewportStateCreateInfo[PipelineIndex].pScissors = Scissor;

			// RASTERIZATION
			RasterizationStateCreateInfo[PipelineIndex] = { };
			RasterizationStateCreateInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			RasterizationStateCreateInfo[PipelineIndex].depthClampEnable = Settings->DepthClampEnable;
			// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
			RasterizationStateCreateInfo[PipelineIndex].rasterizerDiscardEnable = Settings->RasterizerDiscardEnable;
			RasterizationStateCreateInfo[PipelineIndex].polygonMode = Settings->PolygonMode;
			RasterizationStateCreateInfo[PipelineIndex].lineWidth = Settings->LineWidth;
			RasterizationStateCreateInfo[PipelineIndex].cullMode = Settings->CullMode;
			RasterizationStateCreateInfo[PipelineIndex].frontFace = Settings->FrontFace;
			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
			RasterizationStateCreateInfo[PipelineIndex].depthBiasEnable = Settings->DepthBiasEnable;

			// COLOR BLENDING
			// Colors to apply blending to
			ColorBlendAttachmentState[PipelineIndex].colorWriteMask = Settings->ColorWriteMask;
			ColorBlendAttachmentState[PipelineIndex].blendEnable = Settings->BlendEnable;
			// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)// Enable blending
			ColorBlendAttachmentState[PipelineIndex].srcColorBlendFactor = Settings->SrcColorBlendFactor;
			ColorBlendAttachmentState[PipelineIndex].dstColorBlendFactor = Settings->DstColorBlendFactor;
			ColorBlendAttachmentState[PipelineIndex].colorBlendOp = Settings->ColorBlendOp;
			ColorBlendAttachmentState[PipelineIndex].srcAlphaBlendFactor = Settings->SrcAlphaBlendFactor;
			// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
//			   (new color alpha * new color) + ((1 - new color alpha) * old color)
			ColorBlendAttachmentState[PipelineIndex].dstAlphaBlendFactor = Settings->DstAlphaBlendFactor;
			ColorBlendAttachmentState[PipelineIndex].alphaBlendOp = Settings->AlphaBlendOp;
			// Summarised: (1 * new alpha) + (0 * old alpha) = new alpharesult != VK_SUCCESS

			ColorBlendInfo[PipelineIndex] = { };
			ColorBlendInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			ColorBlendInfo[PipelineIndex].logicOpEnable = Settings->LogicOpEnable; // Alternative to calculations is to use logical operations
			ColorBlendInfo[PipelineIndex].attachmentCount = Settings->AttachmentCount;
			ColorBlendInfo[PipelineIndex].pAttachments = Settings->AttachmentCount > 0 ? ColorBlendAttachmentState + PipelineIndex : nullptr;

			// DEPTH STENCIL
			DepthStencilInfo[PipelineIndex] = { };
			DepthStencilInfo[PipelineIndex].sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			DepthStencilInfo[PipelineIndex].depthTestEnable = Settings->DepthTestEnable;
			DepthStencilInfo[PipelineIndex].depthWriteEnable = Settings->DepthWriteEnable;
			DepthStencilInfo[PipelineIndex].depthCompareOp = Settings->DepthCompareOp;
			DepthStencilInfo[PipelineIndex].depthBoundsTestEnable = Settings->DepthBoundsTestEnable;
			DepthStencilInfo[PipelineIndex].stencilTestEnable = Settings->StencilTestEnable;

			// CREATE INFO
			const BMRPipelineResourceInfo* ResourceInfo = ResourceInfos + PipelineIndex;

			VkGraphicsPipelineCreateInfo* PipelineCreateInfo = PipelineCreateInfos + PipelineIndex;
			PipelineCreateInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			PipelineCreateInfo->stageCount = ShaderInputs[PipelineIndex].InfosCounter;
			PipelineCreateInfo->pStages = ShaderInputs[PipelineIndex].Infos;
			PipelineCreateInfo->pVertexInputState = VertexInputInfo + PipelineIndex;
			PipelineCreateInfo->pInputAssemblyState = &InputAssemblyStateCreateInfo;
			PipelineCreateInfo->pViewportState = ViewportStateCreateInfo + PipelineIndex;
			PipelineCreateInfo->pDynamicState = nullptr;
			PipelineCreateInfo->pRasterizationState = RasterizationStateCreateInfo + PipelineIndex;
			PipelineCreateInfo->pMultisampleState = &MultisampleStateCreateInfo;
			PipelineCreateInfo->pColorBlendState = ColorBlendInfo + PipelineIndex;
			PipelineCreateInfo->pDepthStencilState = DepthStencilInfo + PipelineIndex;
			PipelineCreateInfo->layout = ResourceInfo->PipelineLayout;
			PipelineCreateInfo->renderPass = ResourceInfo->RenderPass;
			PipelineCreateInfo->subpass = ResourceInfo->SubpassIndex;

			// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
			PipelineCreateInfo->basePipelineHandle = VK_NULL_HANDLE;
			PipelineCreateInfo->basePipelineIndex = -1;
		}

		const VkResult Result = vkCreateGraphicsPipelines(Device.LogicalDevice, VK_NULL_HANDLE, PipelinesCount,
			PipelineCreateInfos, nullptr, OutputPipelines);

		for (u32 i = 0; i < PipelinesCount; ++i)
		{
			for (u32 j = 0; j < PipelineCreateInfos[i].stageCount; ++j)
			{
				vkDestroyShaderModule(Device.LogicalDevice, PipelineCreateInfos[i].pStages[j].module, nullptr);
			}
		}

		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateGraphicsPipelines result is %d", Result);
		}
	}

	u32 GetImageCount()
	{
		return SwapInstance.ImagesCount;
	}

	VkImageView* GetSwapchainImageViews()
	{
		return SwapInstance.ImageViews;
	}

	void CreateRenderPass(const BMRRenderPassSettings* Settings, const BMRRenderTarget* Targets,
		VkExtent2D TargetExtent, u32 TargetCount, u32 SwapchainImagesCount, BMRRenderPass* OutPass)
	{
		HandleLog(BMRVkLogType_Info, "Initializing RenderPass, Name: %s, Subpass count: %d, Attachments count: %d",
			Settings->RenderPassName, Settings->SubpassSettingsCount, Settings->AttachmentDescriptionsCount);

		// RenderPass create info
		auto Subpasses = Memory::BmMemoryManagementSystem::FrameAlloc<VkSubpassDescription>(Settings->SubpassSettingsCount);
		for (u32 SubpassIndex = 0; SubpassIndex < Settings->SubpassSettingsCount; ++SubpassIndex)
		{
			const BMRSubpassSettings* SubpassSettings = Settings->SubpassesSettings + SubpassIndex;
			VkSubpassDescription* Subpass = Subpasses + SubpassIndex;

			HandleLog(BMRVkLogType_Info, "Initializing Subpass, Name: %s, Color attachments count: %d, "
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
				HandleLog(BMRVkLogType_Warning, "Attempting to set Input attachment to first subpass");
			}
		}

		for (u32 AttachmentDescriptionIndex = 0; AttachmentDescriptionIndex < Settings->AttachmentDescriptionsCount; ++AttachmentDescriptionIndex)
		{
			if (Settings->AttachmentDescriptions[AttachmentDescriptionIndex].format == VK_FORMAT_UNDEFINED)
			{
				HandleLog(BMRVkLogType_Info, "Setting new %d format for attachment description with index %d",
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

		const VkResult Result = vkCreateRenderPass(Device.LogicalDevice, &RenderPassCreateInfo, nullptr, &OutPass->Pass);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateRenderPass result is %d", Result);
		}

		// Set clear values
		OutPass->ClearValues = Memory::BmMemoryManagementSystem::Allocate<VkClearValue>(Settings->AttachmentDescriptionsCount);
		OutPass->ClearValuesCount = Settings->AttachmentDescriptionsCount;
		std::memcpy(OutPass->ClearValues, Settings->ClearValues, Settings->AttachmentDescriptionsCount * sizeof(VkClearValue));

		// Creating framebuffers
		OutPass->FramebufferSets = Memory::BmMemoryManagementSystem::Allocate<BMRFramebufferSet>(TargetCount);
		for (u32 TargetIndex = 0; TargetIndex < TargetCount; ++TargetIndex)
		{
			BMRFramebufferSet* FramebufferSet = OutPass->FramebufferSets + TargetIndex;
			const BMRRenderTarget* Target = Targets + TargetIndex;

			for (u32 SwapchainImageIndex = 0; SwapchainImageIndex < SwapchainImagesCount; ++SwapchainImageIndex)
			{
				VkFramebuffer* Framebuffer = FramebufferSet->FrameBuffers + SwapchainImageIndex;
				const BMRAttachmentView* AttachmentView = Target->AttachmentViews + SwapchainImageIndex;

				VkFramebufferCreateInfo Info = { };
				Info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				Info.renderPass = OutPass->Pass;
				Info.attachmentCount = Settings->AttachmentDescriptionsCount;
				Info.pAttachments = AttachmentView->ImageViews; // List of attachments (1:1 with Render Pass)
				Info.width = TargetExtent.width;
				Info.height = TargetExtent.height;
				Info.layers = 1;

				const VkResult Result = vkCreateFramebuffer(Device.LogicalDevice, &Info, nullptr, Framebuffer);
				if (Result != VK_SUCCESS)
				{
					HandleLog(BMRVkLogType_Error, "vkCreateFramebuffer result is %d", Result);
				}
			}
		}

		OutPass->FramebufferSetCount = TargetCount;
	}

	void DestroyRenderPass(BMRRenderPass* Pass)
	{
		for (u32 TargetIndex = 0; TargetIndex < Pass->FramebufferSetCount; ++TargetIndex)
		{
			for (u32 SwapchainImageIndex = 0; SwapchainImageIndex < SwapInstance.ImagesCount; ++SwapchainImageIndex)
			{
				vkDestroyFramebuffer(Device.LogicalDevice, Pass->FramebufferSets[TargetIndex].FrameBuffers[SwapchainImageIndex], nullptr);
			}
		}

		vkDestroyRenderPass(Device.LogicalDevice, Pass->Pass, nullptr);
		Memory::BmMemoryManagementSystem::Deallocate(Pass->ClearValues);
		Memory::BmMemoryManagementSystem::Deallocate(Pass->FramebufferSets);
	}

	BMRUniform CreateUniformBuffer(const VkBufferCreateInfo* BufferInfo, VkMemoryPropertyFlags Properties)
	{
		BMRUniform UniformBuffer;
		UniformBuffer.Buffer = CreateBuffer(BufferInfo);

		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(Device.LogicalDevice, UniformBuffer.Buffer, &MemoryRequirements);

		const u32 MemoryTypeIndex = GetMemoryTypeIndex(Device.PhysicalDevice, MemoryRequirements.memoryTypeBits, Properties);

		if (BufferInfo->size != MemoryRequirements.size)
		{
			HandleLog(BMRVkLogType_Warning, "Buffer memory requirement size is %d, allocating %d more then buffer size",
				MemoryRequirements.size, MemoryRequirements.size - BufferInfo->size);
		}

		UniformBuffer.Memory = CreateDeviceMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindBufferMemory(Device.LogicalDevice, UniformBuffer.Buffer, UniformBuffer.Memory, 0);

		return UniformBuffer;
	}

	void UpdateUniformBuffer(BMRUniform Buffer, VkDeviceSize DataSize, VkDeviceSize Offset, const void* Data)
	{
		void* MappedMemory;
		vkMapMemory(Device.LogicalDevice, Buffer.Memory, Offset, DataSize, 0, &MappedMemory);
		std::memcpy(MappedMemory, Data, DataSize);
		vkUnmapMemory(Device.LogicalDevice, Buffer.Memory);
	}


	void CopyDataToImage(VkImage Image, u32 Width, u32 Height, u32 Format, u32 LayersCount, void* Data)
	{
		// FIRST TRANSITION
		VkImageMemoryBarrier Barrier = { };
		Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;									// Layout to transition from
		Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;									// Layout to transition to
		Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
		Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
		Barrier.image = Image;											// Image being accessed and modified as part of barrier
		Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
		Barrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
		Barrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from baseMipLevel
		Barrier.subresourceRange.baseArrayLayer = 0;
		Barrier.subresourceRange.layerCount = LayersCount;
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

		vkAllocateCommandBuffers(Device.LogicalDevice, &AllocInfo, &CommandBuffer);
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

		// Copy data to image
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device.LogicalDevice, Image, &MemoryRequirements);

		VulkanMemoryManagementSystem::CopyDataToImage(Image, Width, Height,
			Format, MemoryRequirements.size / LayersCount, LayersCount, Data);

		// SECOND TRANSITION
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

		vkFreeCommandBuffers(Device.LogicalDevice, GraphicsCommandPool, 1, &CommandBuffer);
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

	BMRUniform CreateUniformImage(const VkImageCreateInfo* ImageCreateInfo)
	{
		BMRUniform Buffer;
		VkResult Result = vkCreateImage(Device.LogicalDevice, ImageCreateInfo, nullptr, &Buffer.Image);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "CreateImage result is %d", Result);
		}

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(Device.LogicalDevice, Buffer.Image, &MemoryRequirements);

		const u32 MemoryTypeIndex = GetMemoryTypeIndex(Device.PhysicalDevice, MemoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		Buffer.Memory = AllocateMemory(MemoryRequirements.size, MemoryTypeIndex);
		vkBindImageMemory(Device.LogicalDevice, Buffer.Image, Buffer.Memory, 0);

		return Buffer;
	}

	void DestroyUniformImage(BMRUniform Image)
	{
		vkDestroyImage(Device.LogicalDevice, Image.Image, nullptr);
		vkFreeMemory(Device.LogicalDevice, Image.Memory, nullptr);
	}

	VkDescriptorSetLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages, u32 Count)
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

		VkDescriptorSetLayout Layout;
		const VkResult Result = vkCreateDescriptorSetLayout(Device.LogicalDevice, &LayoutCreateInfo, nullptr, &Layout);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateDescriptorSetLayout result is %d", Result);
		}

		return Layout;
	}

	void DestroyUniformBuffer(BMRUniform Buffer)
	{
		vkDeviceWaitIdle(Device.LogicalDevice); // TODO!!!!!!!!!!!



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

	VkImageView CreateImageInterface(const BMRUniformImageInterfaceCreateInfo* InterfaceCreateInfo, VkImage Image)
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

	void AttachUniformsToSet(VkDescriptorSet Set, const BMRUniformSetAttachmentInfo* Infos, u32 BufferCount)
	{
		auto SetWrites = Memory::BmMemoryManagementSystem::FrameAlloc<VkWriteDescriptorSet>(BufferCount);
		for (u32 BufferIndex = 0; BufferIndex < BufferCount; ++BufferIndex)
		{
			const BMRUniformSetAttachmentInfo* Info = Infos + BufferIndex;

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

	// INTERNAL FUNCTIONS
	void HandleLog(BMRVkLogType LogType, const char* Format, ...)
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
		const char* ValidationLayers[] =
		{
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_monitor",
		};
		const u32 ValidationLayersSize = sizeof(ValidationLayers) / sizeof(ValidationLayers[0]);

		const char* ValidationExtensions[] =
		{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		const u32 ValidationExtensionsSize = Config.EnableValidationLayers ? sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]) : 0;

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
				return false;
			}
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

		if (Config.EnableValidationLayers)
		{
			MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			MessengerCreateInfo.pfnUserCallback = MessengerDebugCallback;
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

		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateInstance result is %d", Result);
			return false;
		}

		if (Config.EnableValidationLayers)
		{
			const bool CreateDebugUtilsMessengerResult =
				CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);

			if (!CreateDebugUtilsMessengerResult)
			{
				HandleLog(BMRVkLogType_Error, "Cannot create debug messenger");
				return false;
			}
		}

		return true;
	}

	bool CreateSurface(HWND WindowHandler)
	{
		VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = { };
		SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		SurfaceCreateInfo.hwnd = WindowHandler;
		SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

		VkResult Result = vkCreateWin32SurfaceKHR(Instance.VulkanInstance, &SurfaceCreateInfo, nullptr, &Surface);
		if (Result != VK_SUCCESS)
		{
			HandleLog(BMRVkLogType_Error, "vkCreateWin32SurfaceKHR result is %d", Result);
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
		VkPhysicalDeviceFeatures AvailableFeatures)
	{
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
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

			VkPhysicalDeviceFeatures AvailableFeatures;
			vkGetPhysicalDeviceFeatures(Device.PhysicalDevice, &AvailableFeatures);

			IsDeviceFound = CheckDeviceSuitability(DeviceExtensions, DeviceExtensionsSize,
				DeviceExtensionsData.Pointer.Data, DeviceExtensionsData.Count, Device.Indices, AvailableFeatures);

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

	void GetBestSwapExtent(HWND WindowHandler)
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
			RECT Rect;
			if (!GetClientRect(WindowHandler, &Rect))
			{
				assert(false);
			}

			u32 Width = Rect.right - Rect.left;
			u32 Height = Rect.bottom - Rect.top;


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
		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = { };
		CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CommandBufferAllocateInfo.commandPool = GraphicsCommandPool;
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		CommandBufferAllocateInfo.commandBufferCount = static_cast<u32>(SwapInstance.ImagesCount);

		VkResult Result = vkAllocateCommandBuffers(Device.LogicalDevice, &CommandBufferAllocateInfo, DrawCommandBuffers);
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

	u32 GetMemoryTypeIndex(VkPhysicalDevice PhysicalDevice, u32 AllowedTypes, VkMemoryPropertyFlags Properties)
	{
		VkPhysicalDeviceMemoryProperties MemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

		for (u32 MemoryTypeIndex = 0; MemoryTypeIndex < MemoryProperties.memoryTypeCount; MemoryTypeIndex++)
		{
			if ((AllowedTypes & (1 << MemoryTypeIndex))														// Index of memory type must match corresponding bit in allowedTypes
				&& (MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags & Properties) == Properties)	// Desired property bit flags are part of memory type's property flags
			{
				// This memory type is valid, so return its index
				return MemoryTypeIndex;
			}
		}

		assert(false);
		return 0;
	}
}
