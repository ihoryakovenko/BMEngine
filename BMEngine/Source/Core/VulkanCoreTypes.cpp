#include "VulkanCoreTypes.h"

#include "VulkanHelper.h"
#include "Util/Util.h"

namespace Core
{
	MainInstance MainInstance::CreateMainInstance(const char** RequiredExtensions, uint32_t RequiredExtensionsCount,
		bool IsValidationLayersEnabled, const char* ValidationLayers[], uint32_t ValidationLayersSize)
	{
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
		CreateInfo.enabledExtensionCount = RequiredExtensionsCount;
		CreateInfo.ppEnabledExtensionNames = RequiredExtensions;

		if (IsValidationLayersEnabled)
		{
			MessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			MessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			MessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			MessengerCreateInfo.pfnUserCallback = Util::MessengerDebugCallback;
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

		MainInstance Instance;
		VkResult Result = vkCreateInstance(&CreateInfo, nullptr, &Instance.VulkanInstance);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateInstance result is {}", static_cast<int>(Result));
			assert(false);
		}

		if (IsValidationLayersEnabled)
		{
			const bool Result = Util::CreateDebugUtilsMessengerEXT(Instance.VulkanInstance, &MessengerCreateInfo, nullptr, &Instance.DebugMessenger);
			assert(Result);
		}

		return Instance;
	}

	void MainInstance::DestroyMainInstance(MainInstance& Instance)
	{
		if (Instance.DebugMessenger != nullptr)
		{
			Util::DestroyDebugMessenger(Instance.VulkanInstance, Instance.DebugMessenger, nullptr);
		}

		vkDestroyInstance(Instance.VulkanInstance, nullptr);
	}

	void MainRenderPass::Init(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
		VkSurfaceFormatKHR SurfaceFormat, int GraphicsFamily, uint32_t MaxDrawFrames,
		VkExtent2D* SwapExtents, uint32_t SwapExtentsCount)
	{
		CreateVulkanPass(LogicalDevice, ColorFormat, DepthFormat, SurfaceFormat);

		PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		PushConstantRange.offset = 0;
		// Todo: check constant and model size?
		PushConstantRange.size = sizeof(Model);

		CreateSamplerSetLayout(LogicalDevice);
		CreateCommandPool(LogicalDevice, GraphicsFamily);
		CreateSynchronisation(LogicalDevice, MaxDrawFrames);
		CreateDescriptorSetLayout(LogicalDevice);
		CreateInputSetLayout(LogicalDevice);
		CreatePipelineLayouts(LogicalDevice);
		CreatePipelines(LogicalDevice, SwapExtents, SwapExtentsCount);
	}

	void MainRenderPass::DeInit(VkDevice LogicalDevice)
	{
		//TODO: FIX
		//vkDestroyPipeline(LogicalDevice, Instance.GraphicsPipeline, nullptr);
		//vkDestroyPipeline(LogicalDevice, Instance.GraphicsPipeline, nullptr);
		DestroySynchronisation(LogicalDevice);
		vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
		vkDestroyPipelineLayout(LogicalDevice, SecondPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, InputSetLayout, nullptr);
		vkDestroyCommandPool(LogicalDevice, GraphicsCommandPool, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, SamplerSetLayout, nullptr);
		vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);
	}

	void MainRenderPass::CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat, VkSurfaceFormatKHR SurfaceFormat)
	{
		// Function CreateRenderPass
		const uint32_t SubpassesCount = 2;
		VkSubpassDescription Subpasses[SubpassesCount];

		// Subpass 1 attachments and references
		VkAttachmentDescription SubpassOneColorAttachment = { };
		SubpassOneColorAttachment.format = ColorFormat;
		SubpassOneColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		SubpassOneColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SubpassOneColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		SubpassOneColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		SubpassOneColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		SubpassOneColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SubpassOneColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription SubpassOneDepthAttachment = { };
		SubpassOneDepthAttachment.format = DepthFormat;
		SubpassOneDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		SubpassOneDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SubpassOneDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		SubpassOneDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		SubpassOneDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		SubpassOneDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SubpassOneDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference SubpassOneColourAttachmentReference = { };
		SubpassOneColourAttachmentReference.attachment = 1; // this index is related to Attachments array
		SubpassOneColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference SubpassOneDepthAttachmentReference = { };
		SubpassOneDepthAttachmentReference.attachment = 2; // this index is related to Attachments array
		SubpassOneDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		Subpasses[0] = { };
		Subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[0].colorAttachmentCount = 1;
		Subpasses[0].pColorAttachments = &SubpassOneColourAttachmentReference;
		Subpasses[0].pDepthStencilAttachment = &SubpassOneDepthAttachmentReference;

		// Subpass 2 attachments and references

		VkAttachmentDescription SwapchainColorAttachment = { };
		SwapchainColorAttachment.format = SurfaceFormat.format;
		SwapchainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		SwapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		SwapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		SwapchainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		SwapchainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		// Framebuffer data will be stored as an image, but images can be given different data layouts
		// to give optimal use for certain operations
		SwapchainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts
		SwapchainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass (to change to)

		VkAttachmentReference SwapchainColorAttachmentReference = { };
		// Todo: do not forget about position in array AttachmentDescriptions
		SwapchainColorAttachmentReference.attachment = 0;
		SwapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		const uint32_t InputReferencesCount = 2;
		VkAttachmentReference InputReferences[InputReferencesCount];
		InputReferences[0].attachment = 1; // SubpassOneColorAttachment
		InputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		InputReferences[1].attachment = 2; // SubpassOneDepthAttachment
		InputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		Subpasses[1] = { };
		Subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[1].colorAttachmentCount = 1;
		Subpasses[1].pColorAttachments = &SwapchainColorAttachmentReference;
		Subpasses[1].inputAttachmentCount = InputReferencesCount;
		Subpasses[1].pInputAttachments = InputReferences;

		// Subpass dependencies

		// Need to determine when layout transitions occur using subpass dependencies
		const uint32_t SubpassDependenciesSize = 3;
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

		// Subpass 1 layout (colour/depth) to Subpass 2 layout (shader read)
		SubpassDependencies[1].srcSubpass = 0;
		SubpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		SubpassDependencies[1].dstSubpass = 1;
		SubpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		SubpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		SubpassDependencies[1].dependencyFlags = 0;

		// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		// Transition must happen after...
		SubpassDependencies[2].srcSubpass = 0;
		SubpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
		// But must happen before...
		SubpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		SubpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		SubpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		SubpassDependencies[2].dependencyFlags = 0;

		// Todo: do not copy atachments to array
		const uint32_t AttachmentDescriptionsCount = 3;
		// must match .attachment
		VkAttachmentDescription AttachmentDescriptions[AttachmentDescriptionsCount] =
		{ SwapchainColorAttachment, SubpassOneColorAttachment, SubpassOneDepthAttachment };

		// Create info for Render Pass
		VkRenderPassCreateInfo RenderPassCreateInfo = { };
		RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.attachmentCount = AttachmentDescriptionsCount;
		RenderPassCreateInfo.pAttachments = AttachmentDescriptions;
		RenderPassCreateInfo.subpassCount = SubpassesCount;
		RenderPassCreateInfo.pSubpasses = Subpasses;
		RenderPassCreateInfo.dependencyCount = SubpassDependenciesSize;
		RenderPassCreateInfo.pDependencies = SubpassDependencies;

		VkResult Result = vkCreateRenderPass(LogicalDevice, &RenderPassCreateInfo, nullptr, &RenderPass);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateRenderPass result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void MainRenderPass::CreateSamplerSetLayout(VkDevice LogicalDevice)
	{
		VkDescriptorSetLayoutBinding SamplerLayoutBinding = { };
		SamplerLayoutBinding.binding = 0;
		SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		SamplerLayoutBinding.descriptorCount = 1;
		SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		SamplerLayoutBinding.pImmutableSamplers = nullptr;

		// Create a Descriptor Set Layout with given bindings for texture
		VkDescriptorSetLayoutCreateInfo TextureLayoutCreateInfo = { };
		TextureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		TextureLayoutCreateInfo.bindingCount = 1;
		TextureLayoutCreateInfo.pBindings = &SamplerLayoutBinding;

		const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &TextureLayoutCreateInfo, nullptr, &SamplerSetLayout);
		if (Result != VK_SUCCESS)
		{
			// TODO LOG
			assert(false);
		}
	}

	void MainRenderPass::CreateCommandPool(VkDevice LogicalDevice, uint32_t FamilyIndex)
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

	void MainRenderPass::CreateSynchronisation(VkDevice LogicalDevice, uint32_t MaxFrameDraws)
	{
		MaxFrameDrawsCounter = MaxFrameDraws;
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

	void MainRenderPass::DestroySynchronisation(VkDevice LogicalDevice)
	{
		for (size_t i = 0; i < MaxFrameDrawsCounter; i++)
		{
			vkDestroySemaphore(LogicalDevice, RenderFinished[i], nullptr);
			vkDestroySemaphore(LogicalDevice, ImageAvailable[i], nullptr);
			vkDestroyFence(LogicalDevice, DrawFences[i], nullptr);
		}
	}
	void MainRenderPass::CreateDescriptorSetLayout(VkDevice LogicalDevice)
	{
		VkDescriptorSetLayoutBinding VpLayoutBinding = { };
		VpLayoutBinding.binding = 0;											// Binding point in shader (designated by binding number in shader)
		VpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Type of descriptor (uniform, dynamic uniform, image sampler, etc)
		VpLayoutBinding.descriptorCount = 1;									// Number of descriptors for binding
		VpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage to bind to
		VpLayoutBinding.pImmutableSamplers = nullptr;							// For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

		const uint32_t BindingCount = 1;
		VkDescriptorSetLayoutBinding Bindings[BindingCount] = { VpLayoutBinding };

		// Create Descriptor Set Layout with given bindings
		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = BindingCount;					// Number of binding infos
		LayoutCreateInfo.pBindings = Bindings;		// Array of binding infos

		VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo, nullptr, &DescriptorSetLayout);
		if (Result != VK_SUCCESS)
		{
			//TODO LOG
			assert(false);
		}
	}

	void MainRenderPass::CreateInputSetLayout(VkDevice LogicalDevice)
	{
		//Create input attachment image descriptor set layout
		// Colour Input Binding
		VkDescriptorSetLayoutBinding ColourInputLayoutBinding = { };
		ColourInputLayoutBinding.binding = 0;
		ColourInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		ColourInputLayoutBinding.descriptorCount = 1;
		ColourInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// Depth Input Binding
		VkDescriptorSetLayoutBinding DepthInputLayoutBinding = { };
		DepthInputLayoutBinding.binding = 1;
		DepthInputLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		DepthInputLayoutBinding.descriptorCount = 1;
		DepthInputLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		// Array of input attachment bindings
		const uint32_t InputBindingsCount = 2;
		// Todo: do not copy InputBindings
		VkDescriptorSetLayoutBinding InputBindings[InputBindingsCount] = { ColourInputLayoutBinding, DepthInputLayoutBinding };

		// Create a descriptor set layout for input attachments
		VkDescriptorSetLayoutCreateInfo InputLayoutCreateInfo = { };
		InputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		InputLayoutCreateInfo.bindingCount = InputBindingsCount;
		InputLayoutCreateInfo.pBindings = InputBindings;

		// Create Descriptor Set Layout
		const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &InputLayoutCreateInfo, nullptr, &InputSetLayout);
		if (Result != VK_SUCCESS)
		{
			// TODO LOG
			assert(false);
		}
	}

	void MainRenderPass::CreatePipelineLayouts(VkDevice LogicalDevice)
	{
		// Pipeline layout
		const uint32_t DescriptorSetLayoutsCount = 2;
		VkDescriptorSetLayout DescriptorSetLayouts[DescriptorSetLayoutsCount] = { DescriptorSetLayout, SamplerSetLayout };

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { };
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.setLayoutCount = DescriptorSetLayoutsCount;
		PipelineLayoutCreateInfo.pSetLayouts = DescriptorSetLayouts;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstantRange;

		VkResult Result = vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
			assert(false);
		}

		// Create new pipeline layout
		VkPipelineLayoutCreateInfo SecondPipelineLayoutCreateInfo = { };
		SecondPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		SecondPipelineLayoutCreateInfo.setLayoutCount = 1;
		SecondPipelineLayoutCreateInfo.pSetLayouts = &InputSetLayout;
		SecondPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		SecondPipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

		Result = vkCreatePipelineLayout(LogicalDevice, &SecondPipelineLayoutCreateInfo, nullptr, &SecondPipelineLayout);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void MainRenderPass::CreatePipelines(VkDevice LogicalDevice, VkExtent2D* SwapExtents, uint32_t SwapExtentsCount)
	{
		// TODO FIX!!!
		// Todo: move to function to avoid code duplication?
// Todo: use separate pipeline initialization structures or remove all values that are not used in next pipeline
// TODO FIX!!!

				// Viewport and scissor
		VkViewport Viewports[4]; //TODO: CHECK 4
		VkRect2D Scissors[4];

		for (int i = 0; i < SwapExtentsCount; ++i)
		{
			Viewports[i].x = 0.0f;
			Viewports[i].y = 0.0f;
			Viewports[i].width = static_cast<float>(SwapExtents[i].width);
			Viewports[i].height = static_cast<float>(SwapExtents[i].height);
			Viewports[i].minDepth = 0.0f;
			Viewports[i].maxDepth = 1.0f;

			Scissors[i].offset = { 0, 0 };
			Scissors[i].extent = SwapExtents[i];
		}

		VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = { };
		ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		ViewportStateCreateInfo.viewportCount = SwapExtentsCount;
		ViewportStateCreateInfo.pViewports = Viewports;
		ViewportStateCreateInfo.scissorCount = SwapExtentsCount;
		ViewportStateCreateInfo.pScissors = Scissors;

		std::vector<char> VertexShaderCode;
		std::vector<char> FragmentShaderCode;
		std::vector<char> SecondVertexShaderCode;
		std::vector<char> SecondFragmentShaderCode;

		Util::OpenAndReadFileFull(Util::UtilHelper::GetVertexShaderPath().data(), VertexShaderCode, "rb");
		Util::OpenAndReadFileFull(Util::UtilHelper::GetFragmentShaderPath().data(), FragmentShaderCode, "rb");
		Util::OpenAndReadFileFull("./Resources/Shaders/second_vert.spv", SecondVertexShaderCode, "rb");
		Util::OpenAndReadFileFull("./Resources/Shaders/second_frag.spv", SecondFragmentShaderCode, "rb");

		VkShaderModule VertexShaderModule;
		VkShaderModule FragmentShaderModule;
		VkShaderModule SecondVertexShaderModule;
		VkShaderModule SecondFragmentShaderModule;

		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(VertexShaderCode.data()),
			VertexShaderCode.size(), VertexShaderModule);
		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(FragmentShaderCode.data()),
			FragmentShaderCode.size(), FragmentShaderModule);
		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(SecondVertexShaderCode.data()),
			SecondVertexShaderCode.size(), SecondVertexShaderModule);
		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(SecondFragmentShaderCode.data()),
			SecondFragmentShaderCode.size(), SecondFragmentShaderModule);

		VkPipelineShaderStageCreateInfo VertexShaderCreateInfo = { };
		VertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		VertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		VertexShaderCreateInfo.module = VertexShaderModule;
		VertexShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo FragmentShaderCreateInfo = { };
		FragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		FragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		FragmentShaderCreateInfo.module = FragmentShaderModule;
		FragmentShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo SecondVertexShaderCreateInfo = { };
		SecondVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		SecondVertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		SecondVertexShaderCreateInfo.module = SecondVertexShaderModule;
		SecondVertexShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo SecondFragmentShaderCreateInfo = { };
		SecondFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		SecondFragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		SecondFragmentShaderCreateInfo.module = SecondFragmentShaderModule;
		SecondFragmentShaderCreateInfo.pName = "main";

		const uint32_t ShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo ShaderStages[ShaderStagesCount] = {
			VertexShaderCreateInfo, FragmentShaderCreateInfo
		};

		const uint32_t SecondShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo SecondShaderStages[SecondShaderStagesCount] = {
			SecondVertexShaderCreateInfo, SecondFragmentShaderCreateInfo
		};

		// How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
		VkVertexInputBindingDescription VertexInputBindingDescription = { };
		VertexInputBindingDescription.binding = 0;									// Can bind multiple streams of data, this defines which one
		VertexInputBindingDescription.stride = sizeof(Core::Vertex);						// Size of a single vertex object
		VertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// How to move between data after each vertex.
		// VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex
		// VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance

		// How the data for an attribute is defined within a vertex
		const uint32_t VertexInputBindingDescriptionCount = 3;
		VkVertexInputAttributeDescription AttributeDescriptions[VertexInputBindingDescriptionCount];

		// Position Attribute
		AttributeDescriptions[0].binding = 0;							// Which binding the data is at (should be same as above)
		AttributeDescriptions[0].location = 0;							// Location in shader where data will be read from
		AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Format the data will take (also helps define size of data)
		AttributeDescriptions[0].offset = offsetof(Core::Vertex, Position);		// Where this attribute is defined in the data for a single vertex

		// Color Attribute
		AttributeDescriptions[1].binding = 0;
		AttributeDescriptions[1].location = 1;
		AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		AttributeDescriptions[1].offset = offsetof(Core::Vertex, Color);

		// Texture Attribute
		AttributeDescriptions[2].binding = 0;
		AttributeDescriptions[2].location = 2;
		AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		AttributeDescriptions[2].offset = offsetof(Core::Vertex, TextureCoords);

		// Vertex input
		VkPipelineVertexInputStateCreateInfo VertexInputCreateInfo = { };
		VertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		VertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		VertexInputCreateInfo.pVertexBindingDescriptions = &VertexInputBindingDescription;
		VertexInputCreateInfo.vertexAttributeDescriptionCount = VertexInputBindingDescriptionCount;
		VertexInputCreateInfo.pVertexAttributeDescriptions = AttributeDescriptions;

		VkPipelineVertexInputStateCreateInfo SecondVertexInputCreateInfo = { };
		// No vertex data for second pass
		SecondVertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		SecondVertexInputCreateInfo.vertexBindingDescriptionCount = 0;
		SecondVertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
		SecondVertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
		SecondVertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

		// Inputassembly
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = { };
		InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Dynamic states TODO
		// Use vkCmdSetViewport(Commandbuffer, 0, 1, &Viewport) and vkCmdSetScissor(Commandbuffer, 0, 1, &Scissor)
		//VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		//VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
		//DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		//DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(2);
		//DynamicStateCreateInfo.pDynamicStates = DynamicStates;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = { };
		RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
		RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		RasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;	// How to handle filling points between Vertices
		RasterizationStateCreateInfo.lineWidth = 1.0f;						// How thick lines should be when drawn
		RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		// Which face of a tri to cull
		RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Winding to determine which side is front
		RasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

		// Multisampling
		VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = { };
		MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
		MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

		// Blending

		VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = { };
		ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	// Colors to apply blending to
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorBlendAttachmentState.blendEnable = VK_TRUE;													// Enable blending

		// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
		ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;

		// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
		//			   (new color alpha * new color) + ((1 - new color alpha) * old color)

		ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		// Summarised: (1 * new alpha) + (0 * old alpha) = new alpharesult != VK_SUCCESS

		VkPipelineColorBlendStateCreateInfo ColorBlendingCreateInfo = { };
		ColorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		ColorBlendingCreateInfo.logicOpEnable = VK_FALSE;				// Alternative to calculations is to use logical operations
		ColorBlendingCreateInfo.attachmentCount = 1;
		ColorBlendingCreateInfo.pAttachments = &ColorBlendAttachmentState;

		// Depth stencil testing
		VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo = { };
		DepthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		DepthStencilCreateInfo.depthTestEnable = VK_TRUE;				// Enable checking depth to determine fragment write
		DepthStencilCreateInfo.depthWriteEnable = VK_TRUE;				// Enable writing to depth buffer (to replace old values)
		DepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;		// Comparison operation that allows an overwrite (is in front)
		DepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;		// Depth Bounds Test: Does the depth value exist between two bounds
		DepthStencilCreateInfo.stencilTestEnable = VK_FALSE;			// Enable Stencil Test

		VkPipelineDepthStencilStateCreateInfo SecondDepthStencilCreateInfo = DepthStencilCreateInfo;
		// Don't want to write to depth buffer
		SecondDepthStencilCreateInfo.depthWriteEnable = VK_FALSE;

		// Graphics pipeline creation
		const uint32_t GraphicsPipelineCreateInfosCount = 2;
		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfos[GraphicsPipelineCreateInfosCount];

		GraphicsPipelineCreateInfos[0] = { };
		GraphicsPipelineCreateInfos[0].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfos[0].stageCount = ShaderStagesCount;					// Number of shader stages
		GraphicsPipelineCreateInfos[0].pStages = ShaderStages;							// List of shader stages
		GraphicsPipelineCreateInfos[0].pVertexInputState = &VertexInputCreateInfo;		// All the fixed function pipeline states
		GraphicsPipelineCreateInfos[0].pInputAssemblyState = &InputAssemblyStateCreateInfo;
		GraphicsPipelineCreateInfos[0].pViewportState = &ViewportStateCreateInfo;
		GraphicsPipelineCreateInfos[0].pDynamicState = nullptr;
		GraphicsPipelineCreateInfos[0].pRasterizationState = &RasterizationStateCreateInfo;
		GraphicsPipelineCreateInfos[0].pMultisampleState = &MultisampleStateCreateInfo;
		GraphicsPipelineCreateInfos[0].pColorBlendState = &ColorBlendingCreateInfo;
		GraphicsPipelineCreateInfos[0].pDepthStencilState = &DepthStencilCreateInfo;
		GraphicsPipelineCreateInfos[0].layout = PipelineLayout;							// Pipeline Layout pipeline should use
		GraphicsPipelineCreateInfos[0].renderPass = RenderPass;							// Render pass description the pipeline is compatible with
		GraphicsPipelineCreateInfos[0].subpass = 0;										// Subpass of render pass to use with pipeline

		// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
		GraphicsPipelineCreateInfos[0].basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
		GraphicsPipelineCreateInfos[0].basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)

		GraphicsPipelineCreateInfos[1] = GraphicsPipelineCreateInfos[0];
		GraphicsPipelineCreateInfos[1].pDepthStencilState = &SecondDepthStencilCreateInfo;
		GraphicsPipelineCreateInfos[1].pVertexInputState = &SecondVertexInputCreateInfo;
		GraphicsPipelineCreateInfos[1].pStages = SecondShaderStages;	// Update second shader stage list
		GraphicsPipelineCreateInfos[1].layout = SecondPipelineLayout;	// Change pipeline layout for input attachment descriptor sets
		GraphicsPipelineCreateInfos[1].subpass = 1;						// Use second subpass

		VkPipeline Pipelines[2];

		const VkResult Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, GraphicsPipelineCreateInfosCount,
			GraphicsPipelineCreateInfos, nullptr, Pipelines);

		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateGraphicsPipelines result is {}", static_cast<int>(Result));
			assert(false);
		}

		GraphicsPipeline = Pipelines[0];
		SecondPipeline = Pipelines[1];

		vkDestroyShaderModule(LogicalDevice, FragmentShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, VertexShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, SecondFragmentShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, SecondVertexShaderModule, nullptr);
	}

	void DeviceInstance::Init(VkInstance VulkanInstance, VkSurfaceKHR Surface, const char** DeviceExtensions,
		uint32_t DeviceExtensionsSize)
	{
		std::vector<VkPhysicalDevice> DeviceList;
		GetPhysicalDeviceList(VulkanInstance, DeviceList);

		bool IsDeviceFound = false;
		for (uint32_t i = 0; i < DeviceList.size(); ++i)
		{
			PhysicalDevice = DeviceList[i];

			std::vector<VkExtensionProperties> DeviceExtensionsData;
			GetDeviceExtensionProperties(PhysicalDevice, DeviceExtensionsData);

			std::vector<VkQueueFamilyProperties> FamilyPropertiesData;
			GetQueueFamilyProperties(PhysicalDevice, FamilyPropertiesData);

			Indices = GetPhysicalDeviceIndices(FamilyPropertiesData, PhysicalDevice, Surface);
			vkGetPhysicalDeviceProperties(PhysicalDevice, &Properties);
			vkGetPhysicalDeviceFeatures(PhysicalDevice, &AvailableFeatures);

			IsDeviceFound = CheckDeviceSuitability(DeviceExtensions, DeviceExtensionsSize,
				DeviceExtensionsData.data(), DeviceExtensionsData.size(), Indices, AvailableFeatures);

			if (IsDeviceFound)
			{
				IsDeviceFound = true;
				break;
			}
		}

		assert(IsDeviceFound);
	}

	void DeviceInstance::GetPhysicalDeviceList(VkInstance VulkanInstance, std::vector<VkPhysicalDevice>& DeviceList)
	{
		uint32_t Count;
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, nullptr);

		DeviceList.resize(Count);
		vkEnumeratePhysicalDevices(VulkanInstance, &Count, DeviceList.data());
	}

	void DeviceInstance::GetDeviceExtensionProperties(VkPhysicalDevice PhysicalDevice, std::vector<VkExtensionProperties>& Data)
	{
		uint32_t Count;
		const VkResult Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkEnumerateDeviceExtensionProperties result is {}", static_cast<int>(Result));
			assert(false);
		}

		Data.resize(Count);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &Count, Data.data());
	}

	void DeviceInstance::GetQueueFamilyProperties(VkPhysicalDevice PhysicalDevice, std::vector<VkQueueFamilyProperties>& Data)
	{
		uint32_t Count;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Count, nullptr);

		Data.resize(Count);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &Count, Data.data());
	}

	// TODO check function
	// In current realization if GraphicsFamily is valid but if PresentationFamily is not valid
	// GraphicsFamily could be overridden on next iteration even when it is valid
	PhysicalDeviceIndices DeviceInstance::GetPhysicalDeviceIndices(const std::vector<VkQueueFamilyProperties>& Properties,
		VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		PhysicalDeviceIndices Indices;

		for (uint32_t i = 0; i < Properties.size(); ++i)
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

	bool DeviceInstance::CheckDeviceSuitability(const char* DeviceExtensions[], uint32_t DeviceExtensionsSize,
		VkExtensionProperties* ExtensionProperties, uint32_t ExtensionPropertiesCount, PhysicalDeviceIndices Indices,
		VkPhysicalDeviceFeatures AvailableFeatures)
	{
		if (!CheckDeviceExtensionsSupport(ExtensionProperties, ExtensionPropertiesCount, DeviceExtensions, DeviceExtensionsSize))
		{
			Util::Log().Warning("PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (Indices.GraphicsFamily < 0 || Indices.PresentationFamily < 0)
		{
			Util::Log().Warning("PhysicalDeviceIndices are not initialized");
			return false;
		}

		if (!AvailableFeatures.samplerAnisotropy)
		{
			Util::Log().Warning("Feature samplerAnisotropy is not supported");
			return false;
		}

		if (!AvailableFeatures.multiViewport)
		{
			Util::Log().Warning("Feature samplerAnisotropy is not supported");
			return false;
		}

		return true;
	}

	bool DeviceInstance::CheckDeviceExtensionsSupport(VkExtensionProperties* ExtensionProperties, uint32_t ExtensionPropertiesCount,
		const char** ExtensionsToCheck, uint32_t ExtensionsToCheckSize)
	{
		for (uint32_t i = 0; i < ExtensionsToCheckSize; ++i)
		{
			bool IsDeviceExtensionSupported = false;
			for (uint32_t j = 0; j < ExtensionPropertiesCount; ++j)
			{
				if (std::strcmp(ExtensionsToCheck[i], ExtensionProperties[j].extensionName) == 0)
				{
					IsDeviceExtensionSupported = true;
					break;
				}
			}

			if (!IsDeviceExtensionSupported)
			{
				Util::Log().Error("Device extension {} unsupported", ExtensionsToCheck[i]);
				return false;
			}
		}

		return true;
	}

	SwapchainInstance SwapchainInstance::CreateSwapchainInstance(VkPhysicalDevice PhysicalDevice,
		PhysicalDeviceIndices Indices, VkDevice LogicalDevice, VkSurfaceKHR Surface,
		VkSurfaceFormatKHR SurfaceFormat, VkExtent2D Extent)
	{
		SwapchainInstance Instance;

		Instance.SwapExtent = Extent;

		VkSurfaceCapabilitiesKHR SurfaceCapabilities = { };
		VkResult Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Warning("vkGetPhysicalDeviceSurfaceCapabilitiesKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkPresentModeKHR PresentationMode = GetBestPresentationMode(PhysicalDevice, Surface);

		Instance.VulkanSwapchain = CreateSwapchain(LogicalDevice, SurfaceCapabilities, Surface, SurfaceFormat, Instance.SwapExtent,
			PresentationMode, Indices);

		std::vector<VkImage> Images;
		GetSwapchainImages(LogicalDevice, Instance.VulkanSwapchain, Images);

		Instance.ImagesCount = Images.size();
		Instance.ImageViews = static_cast<VkImageView*>(Util::Memory::Allocate(Instance.ImagesCount * sizeof(VkImageView)));

		for (uint32_t i = 0; i < Instance.ImagesCount; ++i)
		{
			VkImageView ImageView = CreateImageView(LogicalDevice, Images[i], SurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
			if (ImageView == nullptr)
			{
				assert(false);
			}

			Instance.ImageViews[i] = ImageView;
		}

		return Instance;
	}

	void SwapchainInstance::DestroySwapchainInstance(VkDevice LogicalDevice, SwapchainInstance& Instance)
	{
		for (uint32_t i = 0; i < Instance.ImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, Instance.ImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(LogicalDevice, Instance.VulkanSwapchain, nullptr);
		Util::Memory::Deallocate(Instance.ImageViews);
	}

	VkSwapchainKHR SwapchainInstance::CreateSwapchain(VkDevice LogicalDevice, const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
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

	VkPresentModeKHR SwapchainInstance::GetBestPresentationMode(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface)
	{
		std::vector<VkPresentModeKHR> PresentModes;
		GetAvailablePresentModes(PhysicalDevice, Surface, PresentModes);

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

	void SwapchainInstance::GetAvailablePresentModes(VkPhysicalDevice PhysicalDevice, VkSurfaceKHR Surface, std::vector<VkPresentModeKHR>& PresentModes)
	{
		uint32_t Count;
		const VkResult Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, nullptr);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkGetPhysicalDeviceSurfacePresentModesKHR result is {}", static_cast<int>(Result));
			assert(false);
		}

		PresentModes.resize(Count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &Count, PresentModes.data());
	}

	void SwapchainInstance::GetSwapchainImages(VkDevice LogicalDevice, VkSwapchainKHR VulkanSwapchain, std::vector<VkImage>& Images)
	{
		uint32_t Count;
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, nullptr);

		Images.resize(Count);
		vkGetSwapchainImagesKHR(LogicalDevice, VulkanSwapchain, &Count, Images.data());
	}
}