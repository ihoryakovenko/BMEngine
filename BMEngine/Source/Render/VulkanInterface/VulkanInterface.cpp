#include "VulkanInterface.h"

#include <cassert>
#include <cstring>
#include <stdio.h>
#include <stdarg.h>

#include <glm/glm.hpp>

#include "Engine/Systems/Memory/MemoryManagmentSystem.h"
#include "Engine/Systems/Render/VulkanHelper.h"

#include "Util/Util.h"


#include "Engine/Systems/Render/Render.h"

namespace VulkanInterface
{
	// TYPES DECLARATION
	struct BMRMainInstance
	{
		VkInstance VulkanInstance = nullptr;
		VkDebugUtilsMessengerEXT DebugMessenger = nullptr;
	};

	struct BMRDevice
	{
		VkPhysicalDevice PhysicalDevice = nullptr;
		VkDevice LogicalDevice = nullptr;
		VulkanHelper::PhysicalDeviceIndices Indices;
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
	static bool CheckFormats();
	static bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, VulkanHelper::PhysicalDeviceIndices Indices,
		VkPhysicalDevice Device);


	static bool CreateMainInstance();
	static bool CreateDeviceInstance();
	static VkDevice CreateLogicalDevice(VulkanHelper::PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
		u32 DeviceExtensionsSize);
	static void CreateSynchronisation();
	static bool SetupQueues();
	static bool CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
		VulkanHelper::PhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
		VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent);
	static VkSwapchainKHR CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
		VkSurfaceKHR Surface, VkSurfaceFormatKHR SurfaceFormat, VkExtent2D SwapExtent, VkPresentModeKHR PresentationMode,
		VulkanHelper::PhysicalDeviceIndices DeviceIndices);
	bool CreateCommandBuffers();

	static void DestroyMainInstance(BMRMainInstance& Instance);
	static void DestroySynchronisation();
	static void DestroySwapchainInstance(VkDevice LogicalDevice, BMRSwapchain& Instance);

	// INTERNAL VARIABLES
	static VulkanInterfaceConfig Config;

	static const u32 RequiredExtensionsCount = 2;
	const char* RequiredInstanceExtensions[RequiredExtensionsCount] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	};

	static const u32 MAX_DRAW_FRAMES = 3;

	// Vulkan variables
	static BMRMainInstance Instance;
	static BMRDevice Device;
	static VkPhysicalDeviceProperties DeviceProperties;
	static VkSurfaceKHR Surface;
	static VkSurfaceFormatKHR SurfaceFormat;
	static VkExtent2D SwapExtent;
	static VkSemaphore ImagesAvailable[MAX_DRAW_FRAMES];
	static VkSemaphore RenderFinished[MAX_DRAW_FRAMES];
	static VkFence DrawFences[MAX_DRAW_FRAMES];
	static VkQueue GraphicsQueue;
	//static VkQueue PresentationQueue;
	//static VkQueue TransferQueue;
	static VkCommandPool GraphicsCommandPool;
	static BMRSwapchain SwapInstance;
	static VkDescriptorPool MainPool;
	static VkCommandBuffer DrawCommandBuffers[MAX_SWAPCHAIN_IMAGES_COUNT];

	static u32 CurrentFrame;
	static u32 CurrentImageIndex;


	// FUNCTIONS IMPLEMENTATIONS
	void Init(GLFWwindow* WindowHandler, const VulkanInterfaceConfig& InConfig)
	{
		Config = InConfig;

		CreateMainInstance();

		VULKAN_CHECK_RESULT(glfwCreateWindowSurface(Instance.VulkanInstance, WindowHandler, nullptr, &Surface));

		CreateDeviceInstance();

		Memory::FrameArray<VkSurfaceFormatKHR> AvailableFormats = VulkanHelper::GetSurfaceFormats(Device.PhysicalDevice, Surface);
		SurfaceFormat = VulkanHelper::GetBestSurfaceFormat(Surface, AvailableFormats.Pointer.Data, AvailableFormats.Count);
		CheckFormats();
		SwapExtent = VulkanHelper::GetBestSwapExtent(Device.PhysicalDevice, WindowHandler, Surface);
		CreateSynchronisation();
		SetupQueues();

		VkCommandPoolCreateInfo PoolInfo = { };
		PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		PoolInfo.queueFamilyIndex = Device.Indices.GraphicsFamily;

		VULKAN_CHECK_RESULT(vkCreateCommandPool(Device.LogicalDevice, &PoolInfo, nullptr, &GraphicsCommandPool));

		CreateSwapchainInstance(Device.PhysicalDevice, Device.Indices, Device.LogicalDevice, Surface, SurfaceFormat, SwapExtent);
		CreateCommandBuffers();

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

		MainPool = VulkanHelper::CreateDescriptorPool(Device.LogicalDevice, TotalPassPoolSizes.Data, PoolSizeCount, TotalDescriptorCount);
		////////////////////
	}

	void DeInit()
	{
		vkDestroyDescriptorPool(Device.LogicalDevice, MainPool, nullptr);
		DestroySwapchainInstance(Device.LogicalDevice, SwapInstance);
		vkDestroyCommandPool(Device.LogicalDevice, GraphicsCommandPool, nullptr);
		DestroySynchronisation();
		vkDestroySurfaceKHR(Instance.VulkanInstance, Surface, nullptr);
		DestroyMainInstance(Instance);
	}

	VkPipeline BatchPipelineCreation(const Shader* Shaders, u32 ShadersCount,
		const BMRVertexInputBinding* VertexInputBinding, u32 VertexInputBindingCount,
		const PipelineSettings* Settings, const PipelineResourceInfo* ResourceInfo)
	{
		Util::RenderLog(Util::BMRVkLogType_Info,
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

		Util::RenderLog(Util::BMRVkLogType_Info, "Creating VkPipelineShaderStageCreateInfo, ShadersCount: %u", ShadersCount);

		auto ShaderStageCreateInfos = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineShaderStageCreateInfo>(ShadersCount);
		for (u32 i = 0; i < ShadersCount; ++i)
		{
			Util::RenderLog(Util::BMRVkLogType_Info, "Shader #%d, Stage: %d", i, Shaders[i].Stage);

			VkPipelineShaderStageCreateInfo* ShaderStageCreateInfo = ShaderStageCreateInfos + i;
			*ShaderStageCreateInfo = { };
			ShaderStageCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			ShaderStageCreateInfo->stage = Shaders[i].Stage;
			ShaderStageCreateInfo->pName = "main";
			ShaderStageCreateInfo->module = VulkanHelper::CreateShader(Device.LogicalDevice, (const u32*)Shaders[i].Code, Shaders[i].CodeSize);
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
		auto VertexInputBindings = Memory::MemoryManagementSystem::FrameAlloc<VkVertexInputBindingDescription>(VertexInputBindingCount);
		auto VertexInputAttributes = Memory::MemoryManagementSystem::FrameAlloc<VkVertexInputAttributeDescription>(VertexInputBindingCount * MAX_VERTEX_INPUTS_ATTRIBUTES);
		u32 VertexInputAttributesIndex = 0;

		Util::RenderLog(Util::BMRVkLogType_Info, "Creating vertex input, VertexInputBindingCount: %d", VertexInputBindingCount);

		for (u32 BindingIndex = 0; BindingIndex < VertexInputBindingCount; ++BindingIndex)
		{
			const BMRVertexInputBinding* BMRBinding = VertexInputBinding + BindingIndex;
			VkVertexInputBindingDescription* VertexInputBinding = VertexInputBindings + BindingIndex;

			VertexInputBinding->binding = BindingIndex;
			VertexInputBinding->inputRate = BMRBinding->InputRate;
			VertexInputBinding->stride = BMRBinding->Stride;

			Util::RenderLog(Util::BMRVkLogType_Info, "Initialized VkVertexInputBindingDescription, BindingName: %s, "
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

				Util::RenderLog(Util::BMRVkLogType_Info, "Initialized VkVertexInputAttributeDescription, "
					"AttributeName: %s, BindingIndex: %d, Location: %d, VkFormat: %d, Offset: %d, Index in creation array: %d",
					BMRAttribute->VertexInputAttributeName, BindingIndex, CurrentAttributeIndex,
					VertexInputAttribute->format, VertexInputAttribute->offset, VertexInputAttributesIndex);

				++VertexInputAttributesIndex;
			}
		}
		auto VertexInputInfo = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineVertexInputStateCreateInfo>();
		*VertexInputInfo = { };
		VertexInputInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputInfo->vertexBindingDescriptionCount = VertexInputBindingCount;
		VertexInputInfo->pVertexBindingDescriptions = VertexInputBindings;
		VertexInputInfo->vertexAttributeDescriptionCount = VertexInputAttributesIndex;
		VertexInputInfo->pVertexAttributeDescriptions = VertexInputAttributes;

		// VIEWPORT
		auto Viewport = Memory::MemoryManagementSystem::FrameAlloc<VkViewport>();
		Viewport->width = Settings->Extent.width;
		Viewport->height = Settings->Extent.height;
		Viewport->minDepth = 0.0f;
		Viewport->maxDepth = 1.0f;
		Viewport->x = 0.0f;
		Viewport->y = 0.0f;

		auto Scissor = Memory::MemoryManagementSystem::FrameAlloc<VkRect2D>();
		Scissor->extent.width = Settings->Extent.width;
		Scissor->extent.height = Settings->Extent.height;
		Scissor->offset = { };

		auto ViewportStateCreateInfo = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineViewportStateCreateInfo>();
		*ViewportStateCreateInfo = { };
		ViewportStateCreateInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		ViewportStateCreateInfo->viewportCount = 1;
		ViewportStateCreateInfo->pViewports = Viewport;
		ViewportStateCreateInfo->scissorCount = 1;
		ViewportStateCreateInfo->pScissors = Scissor;

		// RASTERIZATION
		auto RasterizationStateCreateInfo = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineRasterizationStateCreateInfo>();
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
		auto ColorBlendAttachmentState = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineColorBlendAttachmentState>();
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

		auto ColorBlendInfo = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineColorBlendStateCreateInfo>();
		*ColorBlendInfo = { };
		ColorBlendInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		ColorBlendInfo->logicOpEnable = Settings->LogicOpEnable; // Alternative to calculations is to use logical operations
		ColorBlendInfo->attachmentCount = Settings->AttachmentCount;
		ColorBlendInfo->pAttachments = Settings->AttachmentCount > 0 ? ColorBlendAttachmentState : nullptr;

		// DEPTH STENCIL
		auto DepthStencilInfo = Memory::MemoryManagementSystem::FrameAlloc<VkPipelineDepthStencilStateCreateInfo>();
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
		auto PipelineCreateInfo = Memory::MemoryManagementSystem::FrameAlloc<VkGraphicsPipelineCreateInfo>();
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
		VULKAN_CHECK_RESULT(vkCreateGraphicsPipelines(Device.LogicalDevice, VK_NULL_HANDLE, 1, PipelineCreateInfo, nullptr, &Pipeline));

		for (u32 j = 0; j < ShadersCount; ++j)
		{
			vkDestroyShaderModule(Device.LogicalDevice, ShaderStageCreateInfos[j].module, nullptr);
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
		vkWaitForFences(Device.LogicalDevice, 1, &DrawFences[CurrentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(Device.LogicalDevice, 1, &DrawFences[CurrentFrame]);

		vkAcquireNextImageKHR(Device.LogicalDevice, SwapInstance.VulkanSwapchain, UINT64_MAX,
			ImagesAvailable[CurrentFrame], nullptr, &CurrentImageIndex);

		return CurrentImageIndex;
	}

	u32 TestGetImageIndex()
	{
		return CurrentImageIndex;
	}

	VkCommandBuffer BeginFrame()
	{
		VkCommandBufferBeginInfo CommandBufferBeginInfo = { };
		CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkCommandBuffer CommandBuffer = DrawCommandBuffers[CurrentImageIndex];
		VULKAN_CHECK_RESULT(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));
		return CommandBuffer;
	}

	void EndFrame(std::mutex& mutex)
	{
		VULKAN_CHECK_RESULT(vkEndCommandBuffer(DrawCommandBuffers[CurrentImageIndex]));

		uint64_t WaitValues[] = {
			0,
		};

		VkSemaphore WaitSemaphores[] = {
			ImagesAvailable[CurrentFrame],
		};

		VkPipelineStageFlags WaitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		};

		// Timeline info
		VkTimelineSemaphoreSubmitInfo TimelineInfo = { };
		TimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		TimelineInfo.pWaitSemaphoreValues = WaitValues;
		TimelineInfo.waitSemaphoreValueCount = 1;
		TimelineInfo.signalSemaphoreValueCount = 0;
		TimelineInfo.pSignalSemaphoreValues = nullptr;

		VkSubmitInfo SubmitInfo = { };
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.pNext = &TimelineInfo;
		//SubmitInfo.waitSemaphoreCount = 2;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = WaitSemaphores;
		SubmitInfo.pWaitDstStageMask = WaitStages;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &DrawCommandBuffers[CurrentImageIndex];
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &RenderFinished[CurrentFrame];

		VkPresentInfoKHR PresentInfo = { };
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pWaitSemaphores = &RenderFinished[CurrentFrame];
		PresentInfo.pSwapchains = &SwapInstance.VulkanSwapchain; // Swapchains to present images to
		PresentInfo.pImageIndices = &CurrentImageIndex; // Index of images in swapchains to present
		

		{
			std::scoped_lock lock(mutex);
			VULKAN_CHECK_RESULT(vkQueueSubmit(GraphicsQueue, 1, &SubmitInfo, DrawFences[CurrentFrame]));
			VULKAN_CHECK_RESULT(vkQueuePresentKHR(GraphicsQueue, &PresentInfo));
		}

		CurrentFrame = (CurrentFrame + 1) % MAX_DRAW_FRAMES;
	}

	void WaitDevice()
	{
		vkDeviceWaitIdle(Device.LogicalDevice);
	}

	VkSampler CreateSampler(const VkSamplerCreateInfo* CreateInfo)
	{
		VkSampler Sampler;
		VULKAN_CHECK_RESULT(vkCreateSampler(Device.LogicalDevice, CreateInfo, nullptr, &Sampler));
		return Sampler;
	}

	VkDescriptorSetLayout CreateUniformLayout(const VkDescriptorType* Types, const VkShaderStageFlags* Stages,
		const VkDescriptorBindingFlags* BindingFlags, u32 BindingCount, u32 DescriptorCount)
	{
		auto LayoutBindings = Memory::MemoryManagementSystem::FrameAlloc<VkDescriptorSetLayoutBinding>(BindingCount);
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
		VULKAN_CHECK_RESULT(vkCreateDescriptorSetLayout(Device.LogicalDevice, &LayoutCreateInfo, nullptr, &Layout));
		return Layout;
	}

	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32 Count, VkDescriptorSet* OutSets)
	{
		Util::RenderLog(Util::BMRVkLogType_Info, "Allocating descriptor sets. Size count: %d", Count);

		VkDescriptorSetAllocateInfo SetAllocInfo = { };
		SetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocInfo.descriptorPool = MainPool; // Pool to allocate Descriptor Set from
		SetAllocInfo.descriptorSetCount = Count; // Number of sets to allocate
		SetAllocInfo.pSetLayouts = Layouts; // Layouts to use to allocate sets (1:1 relationship)

		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device.LogicalDevice, &SetAllocInfo, (VkDescriptorSet*)OutSets));
	}

	void CreateUniformSets(const VkDescriptorSetLayout* Layouts, u32* DescriptorCounts, u32 Count, VkDescriptorSet* OutSets)
	{
		Util::RenderLog(Util::BMRVkLogType_Info, "Allocating descriptor sets. Size count: %d", Count);

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

		VULKAN_CHECK_RESULT(vkAllocateDescriptorSets(Device.LogicalDevice, &SetAllocInfo, (VkDescriptorSet*)OutSets));
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
		VULKAN_CHECK_RESULT(vkCreateImageView(Device.LogicalDevice, &ViewCreateInfo, nullptr, &ImageView));
		return ImageView;
	}

	void AttachUniformsToSet(VkDescriptorSet Set, const UniformSetAttachmentInfo* Infos, u32 BufferCount)
	{
		auto SetWrites = Memory::MemoryManagementSystem::FrameAlloc<VkWriteDescriptorSet>(BufferCount);
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

	bool CreateMainInstance()
	{
		const char* ValidationExtensions[] =
		{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		const u32 ValidationExtensionsSize =  sizeof(ValidationExtensions) / sizeof(ValidationExtensions[0]);

		Memory::FrameArray<VkExtensionProperties> AvailableExtensions = VulkanHelper::GetAvailableExtensionProperties();
		Memory::FrameArray<const char*> RequiredExtensions = VulkanHelper::GetRequiredInstanceExtensions(RequiredInstanceExtensions, RequiredExtensionsCount,
			ValidationExtensions, ValidationExtensionsSize);

		if (!VulkanHelper::CheckRequiredInstanceExtensionsSupport(AvailableExtensions.Pointer.Data, AvailableExtensions.Count,
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

		MessengerCreateInfo.pfnUserCallback = VulkanHelper::MessengerDebugCallback;


		VULKAN_CHECK_RESULT(vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance));

		const bool CreateDebugUtilsMessengerResult =
			VulkanHelper::CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);

		if (!CreateDebugUtilsMessengerResult)
		{
			Util::RenderLog(Util::BMRVkLogType_Error, "Cannot create debug messenger");
			return false;
		}

		return true;
	}

	void DestroyMainInstance(BMRMainInstance& Instance)
	{
		vkDestroyDevice(Device.LogicalDevice, nullptr);

		if (Instance.DebugMessenger != nullptr)
		{
			VulkanHelper::DestroyDebugMessenger(Instance.VulkanInstance, Instance.DebugMessenger, nullptr);
		}

		vkDestroyInstance(Instance.VulkanInstance, nullptr);
	}

	bool CheckFormats()
	{
		const u32 FormatPrioritySize = 3;
		VkFormat FormatPriority[FormatPrioritySize] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };

		bool IsSupportedFormatFound = false;
		for (u32 i = 0; i < FormatPrioritySize; ++i)
		{
			VkFormat FormatToCheck = FormatPriority[i];
			if (VulkanHelper::CheckFormatSupport(Device.PhysicalDevice, FormatToCheck, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
			{
				IsSupportedFormatFound = true;
				break;
			}

			Util::RenderLog(Util::BMRVkLogType_Warning, "Format %d is not supported", FormatToCheck);
		}

		if (!IsSupportedFormatFound)
		{
			Util::RenderLog(Util::BMRVkLogType_Error, "No supported format found");
			return false;
		}

		return true;
	}

	bool CheckDeviceSuitability(const char* DeviceExtensions[], u32 DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, u32 ExtensionPropertiesCount, VulkanHelper::PhysicalDeviceIndices Indices,
		VkPhysicalDevice Device)
	{
		VkPhysicalDeviceFeatures AvailableFeatures;
		vkGetPhysicalDeviceFeatures(Device, &AvailableFeatures);

		// TODO: Request vkGetPhysicalDeviceProperties in the caller
		vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

		VulkanHelper::PrintDeviceData(&DeviceProperties, &AvailableFeatures);

		if (!VulkanHelper::CheckDeviceExtensionsSupport(ExtensionProperties, ExtensionPropertiesCount, DeviceExtensions, DeviceExtensionsSize))
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (Indices.GraphicsFamily < 0 || Indices.PresentationFamily < 0)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (!AvailableFeatures.samplerAnisotropy)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "Feature samplerAnisotropy is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "Feature multiViewport is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			Util::RenderLog(Util::BMRVkLogType_Warning, "Feature multiViewport is not supported");
			return false;
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

		Memory::FrameArray<VkPhysicalDevice> DeviceList = VulkanHelper::GetPhysicalDeviceList(Instance.VulkanInstance);

		bool IsDeviceFound = false;
		for (u32 i = 0; i < DeviceList.Count; ++i)
		{
			Device.PhysicalDevice = DeviceList[i];

			Memory::FrameArray<VkExtensionProperties> DeviceExtensionsData = VulkanHelper::GetDeviceExtensionProperties(Device.PhysicalDevice);
			Memory::FrameArray<VkQueueFamilyProperties> FamilyPropertiesData = VulkanHelper::GetQueueFamilyProperties(Device.PhysicalDevice);

			Device.Indices = VulkanHelper::GetPhysicalDeviceIndices(FamilyPropertiesData.Pointer.Data, FamilyPropertiesData.Count, Device.PhysicalDevice, Surface);

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
			Util::RenderLog(Util::BMRVkLogType_Error, "Cannot find suitable device");
			return false;
		}

		Device.LogicalDevice = CreateLogicalDevice(Device.Indices, DeviceExtensions, DeviceExtensionsSize);
		if (Device.LogicalDevice == nullptr)
		{
			return false;
		}

		return true;
	}

	VkDevice CreateLogicalDevice(VulkanHelper::PhysicalDeviceIndices Indices, const char* DeviceExtensions[],
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

		// TODO: Check if supported
		VkPhysicalDeviceFeatures2 DeviceFeatures2 = { };
		DeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		DeviceFeatures2.features.fillModeNonSolid = VK_TRUE; // Todo: get from configs
		DeviceFeatures2.features.samplerAnisotropy = VK_TRUE; // Todo: get from configs
		DeviceFeatures2.features.multiViewport = VK_TRUE; // Todo: get from configs

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

		VkPhysicalDeviceTimelineSemaphoreFeatures TimelineSemaphoreFeatures = { };
		TimelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		TimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

		VkPhysicalDeviceFeatures DeviceFeatures = { };
		DeviceFeatures.samplerAnisotropy = VK_TRUE;

		DeviceFeatures2.pNext = &TimelineSemaphoreFeatures;
		TimelineSemaphoreFeatures.pNext = &IndexingFeatures;
		IndexingFeatures.pNext = &Sync2Features;
		Sync2Features.pNext = &DynamicRenderingFeatures;


		VkDeviceCreateInfo DeviceCreateInfo = { };
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = FamilyIndicesSize;
		DeviceCreateInfo.pQueueCreateInfos = QueueCreateInfos;
		DeviceCreateInfo.enabledExtensionCount = DeviceExtensionsSize;
		DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions;
		DeviceCreateInfo.pEnabledFeatures = nullptr;
		DeviceCreateInfo.pNext = &DeviceFeatures2;

		VkDevice LogicalDevice;
		// Queues are created at the same time as the Device
		VULKAN_CHECK_RESULT(vkCreateDevice(Device.PhysicalDevice, &DeviceCreateInfo, nullptr, &LogicalDevice));
		return LogicalDevice;
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
				Util::RenderLog(Util::BMRVkLogType_Error, "CreateSynchronisation error");
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
		//vkGetDeviceQueue(Device.LogicalDevice, static_cast<u32>(Device.Indices.PresentationFamily), 0, &PresentationQueue);
		//vkGetDeviceQueue(Device.LogicalDevice, static_cast<u32>(Device.Indices.TransferFamily), 0, &TransferQueue);

		//if (GraphicsQueue == nullptr || PresentationQueue == nullptr || TransferQueue == nullptr)
		{
			//return false;
		}

		return true;
	}

	bool CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
		VulkanHelper::PhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
		VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent)
	{
		SwapInstance.SwapExtent = Extent;

		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };
		VULKAN_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities));

		VkPresentModeKHR PresentationMode = VulkanHelper::GetBestPresentationMode(PhysicalDevice, Surface);

		SwapInstance.VulkanSwapchain = CreateSwapchain(LogicalDevice, SurfaceCapabilities, Surface, SurfaceFormat, SwapInstance.SwapExtent,
			PresentationMode, Indices);

		Memory::FrameArray<VkImage> Images = VulkanHelper::GetSwapchainImages(LogicalDevice, SwapInstance.VulkanSwapchain);

		SwapInstance.ImagesCount = Images.Count;

		for (u32 i = 0; i < SwapInstance.ImagesCount; ++i)
		{
			SwapInstance.Images[i] = Images[i];

			VkImageViewCreateInfo ViewCreateInfo = { };
			ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ViewCreateInfo.image = Images[i];
			ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ViewCreateInfo.format = SurfaceFormat.format;
			ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ViewCreateInfo.subresourceRange.levelCount = 1;
			ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			ViewCreateInfo.subresourceRange.layerCount = 1;

			// Create image view and return it
			VkImageView ImageView;
			VULKAN_CHECK_RESULT(vkCreateImageView(Device.LogicalDevice, &ViewCreateInfo, nullptr, &ImageView));

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
		VulkanHelper::PhysicalDeviceIndices DeviceIndices)
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

		// Used if old swap chain been destroyed and this one replaces it
		SwapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		VkSwapchainKHR Swapchain;
		VULKAN_CHECK_RESULT(vkCreateSwapchainKHR(LogicalDevice, &SwapchainCreateInfo, nullptr, &Swapchain));
		return Swapchain;
	}

	bool CreateCommandBuffers()
	{
		VkCommandBufferAllocateInfo GraphicsCommandBufferAllocateInfo = { };
		GraphicsCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		GraphicsCommandBufferAllocateInfo.commandPool = GraphicsCommandPool;
		GraphicsCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
		// VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly. Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
		GraphicsCommandBufferAllocateInfo.commandBufferCount = static_cast<u32>(SwapInstance.ImagesCount);

		VULKAN_CHECK_RESULT(vkAllocateCommandBuffers(Device.LogicalDevice, &GraphicsCommandBufferAllocateInfo, DrawCommandBuffers));
		return true;
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

	VkQueue GetTransferQueue()
	{
		//return TransferQueue;
		return GraphicsQueue;
	}

	VkQueue GetGraphicsQueue()
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

	VkCommandPool GetTransferCommandPool()
	{
		return GraphicsCommandPool;
	}
}
