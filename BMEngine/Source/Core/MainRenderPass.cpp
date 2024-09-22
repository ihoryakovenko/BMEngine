#include "MainRenderPass.h"

#include "Util/Util.h"

namespace Core
{
	void MainRenderPass::Init(VkDevice InLogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat,
		VkSurfaceFormatKHR SurfaceFormat, int GraphicsFamily, uint32_t MaxDrawFrames,
		VkExtent2D* SwapExtents, uint32_t SwapExtentsCount)
	{
		LogicalDevice = InLogicalDevice;

		CreateVulkanPass(ColorFormat, DepthFormat, SurfaceFormat);
		
		PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		PushConstantRange.offset = 0;
		// Todo: check constant and model size?
		PushConstantRange.size = sizeof(Model);

		CreateSamplerSetLayout();
		CreateCommandPool(GraphicsFamily);
		CreateSynchronisation(MaxDrawFrames);
		CreateDescriptorSetLayout();
		CreateInputSetLayout();
		CreatePipelineLayout();
		CreatePipelines(SwapExtents, SwapExtentsCount);
	}

	void MainRenderPass::DeInit()
	{
		//TODO: FIX
		//vkDestroyPipeline(LogicalDevice, Instance.GraphicsPipeline, nullptr);
		//vkDestroyPipeline(LogicalDevice, Instance.GraphicsPipeline, nullptr);
		DestroyRenderPassSynchronisationData();
		vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
		vkDestroyPipelineLayout(LogicalDevice, SecondPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, InputSetLayout, nullptr);
		vkDestroyCommandPool(LogicalDevice, GraphicsCommandPool, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, SamplerSetLayout, nullptr);
		vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);
	}

	void MainRenderPass::CreateVulkanPass(VkFormat ColorFormat, VkFormat DepthFormat, VkSurfaceFormatKHR SurfaceFormat)
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

	void MainRenderPass::CreateSamplerSetLayout()
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

	void MainRenderPass::CreateCommandPool(uint32_t FamilyIndex)
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

	void MainRenderPass::CreateSynchronisation(uint32_t MaxFrameDraws)
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

	void MainRenderPass::DestroyRenderPassSynchronisationData()
	{
		for (size_t i = 0; i < MaxFrameDrawsCounter; i++)
		{
			vkDestroySemaphore(LogicalDevice, RenderFinished[i], nullptr);
			vkDestroySemaphore(LogicalDevice, ImageAvailable[i], nullptr);
			vkDestroyFence(LogicalDevice, DrawFences[i], nullptr);
		}
	}
	void MainRenderPass::CreateDescriptorSetLayout()
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

	void MainRenderPass::CreateInputSetLayout()
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

	void MainRenderPass::CreatePipelineLayout()
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

	void MainRenderPass::CreatePipelines(VkExtent2D* SwapExtents, uint32_t SwapExtentsCount)
	{
		// TODO FIX!!!
		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull(Util::UtilHelper::GetVertexShaderPath().data(), VertexShaderCode, "rb");

		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull(Util::UtilHelper::GetFragmentShaderPath().data(), FragmentShaderCode, "rb");


		//Build shader modules
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = { };
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = VertexShaderCode.size();
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(VertexShaderCode.data());

		VkShaderModule VertexShaderModule = nullptr;
		VkResult Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			assert(false);
		}

		ShaderModuleCreateInfo.codeSize = FragmentShaderCode.size();
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(FragmentShaderCode.data());

		VkShaderModule FragmentShaderModule = nullptr;
		Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &FragmentShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			assert(false);
		}

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

		const uint32_t ShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo ShaderStages[ShaderStagesCount] = { VertexShaderCreateInfo, FragmentShaderCreateInfo };

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

		// Inputassembly
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo = { };
		InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

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

		// Graphics pipeline creation
		const uint32_t GraphicsPipelineCreateInfosCount = 1;
		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = { };
		GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.stageCount = ShaderStagesCount;					// Number of shader stages
		GraphicsPipelineCreateInfo.pStages = ShaderStages;							// List of shader stages
		GraphicsPipelineCreateInfo.pVertexInputState = &VertexInputCreateInfo;		// All the fixed function pipeline states
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;
		GraphicsPipelineCreateInfo.pViewportState = &ViewportStateCreateInfo;
		GraphicsPipelineCreateInfo.pDynamicState = nullptr;
		GraphicsPipelineCreateInfo.pRasterizationState = &RasterizationStateCreateInfo;
		GraphicsPipelineCreateInfo.pMultisampleState = &MultisampleStateCreateInfo;
		GraphicsPipelineCreateInfo.pColorBlendState = &ColorBlendingCreateInfo;
		GraphicsPipelineCreateInfo.pDepthStencilState = &DepthStencilCreateInfo;
		GraphicsPipelineCreateInfo.layout = PipelineLayout;							// Pipeline Layout pipeline should use
		GraphicsPipelineCreateInfo.renderPass = RenderPass;							// Render pass description the pipeline is compatible with
		GraphicsPipelineCreateInfo.subpass = 0;										// Subpass of render pass to use with pipeline

		// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)


		Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, GraphicsPipelineCreateInfosCount, &GraphicsPipelineCreateInfo, nullptr, &GraphicsPipeline);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateGraphicsPipelines result is {}", static_cast<int>(Result));
			assert(false);
		}

		vkDestroyShaderModule(LogicalDevice, FragmentShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, VertexShaderModule, nullptr);

		// Second pipeline
		// Todo: move to function to avoid code duplication?
		// Todo: use separate pipeline initialization structures or remove all values that are not used in next pipeline
		// TODO FIX!!!
		std::vector<char> SecondVertexShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/second_vert.spv", VertexShaderCode, "rb");

		std::vector<char> SecondFragmentShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/second_frag.spv", FragmentShaderCode, "rb");

		// Build shaders
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.codeSize = VertexShaderCode.size();
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(VertexShaderCode.data());

		Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			assert(false);
		}

		ShaderModuleCreateInfo.codeSize = FragmentShaderCode.size();
		ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(FragmentShaderCode.data());

		Result = vkCreateShaderModule(LogicalDevice, &ShaderModuleCreateInfo, nullptr, &FragmentShaderModule);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateShaderModule result is {}", static_cast<int>(Result));
			assert(false);
		}
		// Set new shaders

		VertexShaderCreateInfo.module = VertexShaderModule;
		FragmentShaderCreateInfo.module = FragmentShaderModule;

		const uint32_t SecondShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo SecondShaderStages[SecondShaderStagesCount] = { VertexShaderCreateInfo, FragmentShaderCreateInfo };

		// No vertex data for second pass
		VertexInputCreateInfo.vertexBindingDescriptionCount = 0;
		VertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
		VertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
		VertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

		// Don't want to write to depth buffer
		DepthStencilCreateInfo.depthWriteEnable = VK_FALSE;

		GraphicsPipelineCreateInfo.pStages = SecondShaderStages;	// Update second shader stage list
		GraphicsPipelineCreateInfo.layout = SecondPipelineLayout;	// Change pipeline layout for input attachment descriptor sets
		GraphicsPipelineCreateInfo.subpass = 1;						// Use second subpass

		// Create second pipeline
		// Todo: !!! Use one vkCreateGraphicsPipelines call to create pipelines
		Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &SecondPipeline);
		if (Result != VK_SUCCESS)
		{
			//TODO LOG
			assert(false);
		}

		// Destroy second shader modules
		vkDestroyShaderModule(LogicalDevice, FragmentShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, VertexShaderModule, nullptr);
	}
}
