#include "MainRenderPass.h"

#include "Util/Util.h"
#include "VulkanHelper.h"

namespace Core
{
	void MainRenderPass::GetPoolSizes(uint32_t TotalImagesCount, std::vector<VkDescriptorPoolSize>& TotalPassPoolSizes,
		uint32_t& TotalDescriptorCount)
	{
		TotalPassPoolSizes.reserve(4);
		TotalPassPoolSizes.push_back({ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = TotalImagesCount });
		TotalPassPoolSizes.push_back({ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = TotalImagesCount });
		TotalPassPoolSizes.push_back({ .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = TotalImagesCount });
		TotalPassPoolSizes.push_back({ .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = TotalImagesCount });

		uint32_t TotalDescriptorLayouts = 3;
		TotalDescriptorCount = TotalDescriptorLayouts * TotalImagesCount;
	}

	void MainRenderPass::ClearResources(VkDevice LogicalDevice, uint32_t ImagesCount)
	{
		for (uint32_t i = 0; i < ImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, DepthBuffers[i].ImageView, nullptr);
			vkDestroyImage(LogicalDevice, DepthBuffers[i].Image, nullptr);
			vkFreeMemory(LogicalDevice, DepthBuffers[i].Memory, nullptr);

			vkDestroyImageView(LogicalDevice, ColorBuffers[i].ImageView, nullptr);
			vkDestroyImage(LogicalDevice, ColorBuffers[i].Image, nullptr);
			vkFreeMemory(LogicalDevice, ColorBuffers[i].Memory, nullptr);

			GPUBuffer::GPUBuffer::DestroyGPUBuffer(LogicalDevice, VpUniformBuffers[i]);
		}

		TerrainPass.ClearResources(LogicalDevice);
		DeferredPass.ClearResources(LogicalDevice);
		EntityPass.ClearResources(LogicalDevice);

		vkDestroyCommandPool(LogicalDevice, GraphicsCommandPool, nullptr);
		vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);

		Util::Memory::Deallocate(DepthBuffers);
		Util::Memory::Deallocate(ColorBuffers);
		Util::Memory::Deallocate(VpUniformBuffers);
	}

	void MainRenderPass::CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat, VkSurfaceFormatKHR SurfaceFormat)
	{
		// TODO: Check AccessMasks

		const uint32_t TerrainSubpassIndex = 0;  // Terrain will be first
		const uint32_t EntitySubpassIndex = 1;   // Entity subpass comes next
		const uint32_t DeferredSubpassIndex = 2; // Fullscreen effects

		const uint32_t SubpassesCount = 3;
		VkSubpassDescription Subpasses[SubpassesCount];

		const uint32_t SwapchainColorAttachmentIndex = 0;
		const uint32_t SubpassColorAttachmentIndex = 1;
		const uint32_t SubpassDepthAttachmentIndex = 2;

		const uint32_t AttachmentDescriptionsCount = 3;
		VkAttachmentDescription AttachmentDescriptions[AttachmentDescriptionsCount];

		AttachmentDescriptions[SwapchainColorAttachmentIndex] = { };
		AttachmentDescriptions[SwapchainColorAttachmentIndex].format = SurfaceFormat.format;  // Use the appropriate surface format
		AttachmentDescriptions[SwapchainColorAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;  // Typically single sample for swapchain
		AttachmentDescriptions[SwapchainColorAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear the color buffer at the beginning
		AttachmentDescriptions[SwapchainColorAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // Store the result so it can be presented
		AttachmentDescriptions[SwapchainColorAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[SwapchainColorAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[SwapchainColorAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // No need for initial content
		AttachmentDescriptions[SwapchainColorAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Make it ready for presentation

		AttachmentDescriptions[SubpassColorAttachmentIndex] = { };
		AttachmentDescriptions[SubpassColorAttachmentIndex].format = ColorFormat;
		AttachmentDescriptions[SubpassColorAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescriptions[SubpassColorAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		AttachmentDescriptions[SubpassColorAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[SubpassColorAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[SubpassColorAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[SubpassColorAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		AttachmentDescriptions[SubpassColorAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		AttachmentDescriptions[SubpassDepthAttachmentIndex] = { };
		AttachmentDescriptions[SubpassDepthAttachmentIndex].format = DepthFormat;
		AttachmentDescriptions[SubpassDepthAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescriptions[SubpassDepthAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		AttachmentDescriptions[SubpassDepthAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[SubpassDepthAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[SubpassDepthAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[SubpassDepthAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		AttachmentDescriptions[SubpassDepthAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Color and depth attachment references for the Terrain subpass
		VkAttachmentReference TerrainColorAttachmentRef = { };
		TerrainColorAttachmentRef.attachment = SubpassColorAttachmentIndex; // Color attachment index in Attachments array
		TerrainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference TerrainDepthAttachmentRef = { };
		TerrainDepthAttachmentRef.attachment = SubpassDepthAttachmentIndex; // Depth attachment index in Attachments array
		TerrainDepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Define the Terrain subpass
		Subpasses[TerrainSubpassIndex] = { };
		Subpasses[TerrainSubpassIndex].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[TerrainSubpassIndex].colorAttachmentCount = 1;
		Subpasses[TerrainSubpassIndex].pColorAttachments = &TerrainColorAttachmentRef;
		Subpasses[TerrainSubpassIndex].pDepthStencilAttachment = &TerrainDepthAttachmentRef;

		// Define the Entity subpass, which will read the color and depth attachments
		VkAttachmentReference EntitySubpassColourAttachmentReference = { };
		EntitySubpassColourAttachmentReference.attachment = SubpassColorAttachmentIndex; // Index of the color attachment (shared with Terrain subpass)
		EntitySubpassColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference EntitySubpassDepthAttachmentReference = { };
		EntitySubpassDepthAttachmentReference.attachment = SubpassDepthAttachmentIndex; // Depth attachment (shared with Terrain subpass)
		EntitySubpassDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		Subpasses[EntitySubpassIndex] = { };
		Subpasses[EntitySubpassIndex].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[EntitySubpassIndex].colorAttachmentCount = 1;
		Subpasses[EntitySubpassIndex].pColorAttachments = &EntitySubpassColourAttachmentReference;
		Subpasses[EntitySubpassIndex].pDepthStencilAttachment = &EntitySubpassDepthAttachmentReference;

		// Deferred subpass (Fullscreen effects)
		VkAttachmentReference SwapchainColorAttachmentReference = { };
		SwapchainColorAttachmentReference.attachment = SwapchainColorAttachmentIndex; // Swapchain color attachment
		SwapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		const uint32_t InputReferencesCount = 2;
		VkAttachmentReference InputReferences[InputReferencesCount];
		InputReferences[0].attachment = 1;
		InputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		InputReferences[1].attachment = 2;
		InputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		Subpasses[DeferredSubpassIndex] = { };
		Subpasses[DeferredSubpassIndex].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[DeferredSubpassIndex].colorAttachmentCount = 1;
		Subpasses[DeferredSubpassIndex].pColorAttachments = &SwapchainColorAttachmentReference;
		Subpasses[DeferredSubpassIndex].inputAttachmentCount = InputReferencesCount;
		Subpasses[DeferredSubpassIndex].pInputAttachments = InputReferences;

		// Subpass dependencies
		const uint32_t ExitDependenciesIndex = DeferredSubpassIndex + 1;
		const uint32_t SubpassDependenciesCount = SubpassesCount + 1;
		VkSubpassDependency SubpassDependencies[SubpassDependenciesCount];

		// Transition from external to Terrain subpass
		// Transition must happen after...
		SubpassDependencies[TerrainSubpassIndex].srcSubpass = VK_SUBPASS_EXTERNAL;
		SubpassDependencies[TerrainSubpassIndex].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		SubpassDependencies[TerrainSubpassIndex].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// But must happen before...
		SubpassDependencies[TerrainSubpassIndex].dstSubpass = TerrainSubpassIndex;
		SubpassDependencies[TerrainSubpassIndex].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[TerrainSubpassIndex].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		SubpassDependencies[TerrainSubpassIndex].dependencyFlags = 0;

		// Transition from Terrain to Entity subpass
		// Transition must happen after...
		SubpassDependencies[EntitySubpassIndex].srcSubpass = TerrainSubpassIndex;
		SubpassDependencies[EntitySubpassIndex].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[EntitySubpassIndex].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// But must happen before...
		SubpassDependencies[EntitySubpassIndex].dstSubpass = EntitySubpassIndex;
		SubpassDependencies[EntitySubpassIndex].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		SubpassDependencies[EntitySubpassIndex].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		SubpassDependencies[EntitySubpassIndex].dependencyFlags = 0;

		// Transition from Entity subpass to Deferred subpass
		// Transition must happen after...
		SubpassDependencies[DeferredSubpassIndex].srcSubpass = EntitySubpassIndex;
		SubpassDependencies[DeferredSubpassIndex].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[DeferredSubpassIndex].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// But must happen before...
		SubpassDependencies[DeferredSubpassIndex].dstSubpass = DeferredSubpassIndex;
		SubpassDependencies[DeferredSubpassIndex].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		SubpassDependencies[DeferredSubpassIndex].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		SubpassDependencies[DeferredSubpassIndex].dependencyFlags = 0;

		// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		// Transition must happen after...
		SubpassDependencies[ExitDependenciesIndex].srcSubpass = EntitySubpassIndex;
		SubpassDependencies[ExitDependenciesIndex].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[ExitDependenciesIndex].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// But must happen before...
		SubpassDependencies[ExitDependenciesIndex].dstSubpass = VK_SUBPASS_EXTERNAL;
		SubpassDependencies[ExitDependenciesIndex].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		SubpassDependencies[ExitDependenciesIndex].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		SubpassDependencies[ExitDependenciesIndex].dependencyFlags = 0;

		// Create the render pass
		VkRenderPassCreateInfo RenderPassCreateInfo = { };
		RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.attachmentCount = AttachmentDescriptionsCount;
		RenderPassCreateInfo.pAttachments = AttachmentDescriptions;
		RenderPassCreateInfo.subpassCount = SubpassesCount;
		RenderPassCreateInfo.pSubpasses = Subpasses;
		RenderPassCreateInfo.dependencyCount = SubpassDependenciesCount;
		RenderPassCreateInfo.pDependencies = SubpassDependencies;

		VkResult Result = vkCreateRenderPass(LogicalDevice, &RenderPassCreateInfo, nullptr, &RenderPass);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateRenderPass result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void MainRenderPass::SetupPushConstants()
	{
		EntityPass.PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		EntityPass.PushConstantRange.offset = 0;
		// Todo: check constant and model size?
		EntityPass.PushConstantRange.size = sizeof(Model);
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

		const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &TextureLayoutCreateInfo,
			nullptr, &EntityPass.SamplerSetLayout);
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

	void MainRenderPass::CreateTerrainSetLayout(VkDevice LogicalDevice)
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

		VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo,
			nullptr, &TerrainPass.TerrainSetLayout);
		if (Result != VK_SUCCESS)
		{
			//TODO LOG
			assert(false);
		}
	}

	void MainRenderPass::CreateEntitySetLayout(VkDevice LogicalDevice)
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

		VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo,
			nullptr, &EntityPass.EntitySetLayout);
		if (Result != VK_SUCCESS)
		{
			//TODO LOG
			assert(false);
		}
	}

	void MainRenderPass::CreateDeferredSetLayout(VkDevice LogicalDevice)
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
		const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &InputLayoutCreateInfo,
			nullptr, &DeferredPass.DeferredLayout);
		if (Result != VK_SUCCESS)
		{
			// TODO LOG
			assert(false);
		}
	}

	void MainRenderPass::CreatePipelineLayouts(VkDevice LogicalDevice)
	{
		VkPipelineLayoutCreateInfo TerrainLayoutCreateInfo = { };
		TerrainLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		TerrainLayoutCreateInfo.setLayoutCount = 1;
		TerrainLayoutCreateInfo.pSetLayouts = &TerrainPass.TerrainSetLayout;
		TerrainLayoutCreateInfo.pushConstantRangeCount = 0;
		TerrainLayoutCreateInfo.pPushConstantRanges = nullptr;

		VkResult Result = vkCreatePipelineLayout(LogicalDevice, &TerrainLayoutCreateInfo, nullptr, &TerrainPass.PipelineLayout);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
			assert(false);
		}

		const uint32_t EntityPassDescriptorSetLayoutsCount = 2;
		VkDescriptorSetLayout EntityPassDescriptorSetLayouts[EntityPassDescriptorSetLayoutsCount] = {
			EntityPass.EntitySetLayout, EntityPass.SamplerSetLayout
		};

		VkPipelineLayoutCreateInfo EntityLayoutCreateInfo = { };
		EntityLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		EntityLayoutCreateInfo.setLayoutCount = EntityPassDescriptorSetLayoutsCount;
		EntityLayoutCreateInfo.pSetLayouts = EntityPassDescriptorSetLayouts;
		EntityLayoutCreateInfo.pushConstantRangeCount = 1;
		EntityLayoutCreateInfo.pPushConstantRanges = &EntityPass.PushConstantRange;

		Result = vkCreatePipelineLayout(LogicalDevice, &EntityLayoutCreateInfo, nullptr, &EntityPass.PipelineLayout);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkPipelineLayoutCreateInfo DeferredLayoutCreateInfo = { };
		DeferredLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		DeferredLayoutCreateInfo.setLayoutCount = 1;
		DeferredLayoutCreateInfo.pSetLayouts = &DeferredPass.DeferredLayout;
		DeferredLayoutCreateInfo.pushConstantRangeCount = 0;
		DeferredLayoutCreateInfo.pPushConstantRanges = nullptr;

		Result = vkCreatePipelineLayout(LogicalDevice, &DeferredLayoutCreateInfo, nullptr, &DeferredPass.PipelineLayout);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void MainRenderPass::CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent)
	{
		// TODO FIX!!!
		// Todo: move to function to avoid code duplication?
// Todo: use separate pipeline initialization structures or remove all values that are not used in next pipeline
// TODO FIX!!!

				// Viewport and scissor
		VkViewport Viewport; //TODO: CHECK 4
		VkRect2D Scissor;

		Viewport.x = 0.0f;
		Viewport.y = 0.0f;
		Viewport.width = static_cast<float>(SwapExtent.width);
		Viewport.height = static_cast<float>(SwapExtent.height);
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		Scissor.offset = { 0, 0 };
		Scissor.extent = SwapExtent;

		VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = { };
		ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		ViewportStateCreateInfo.viewportCount = 1;
		ViewportStateCreateInfo.pViewports = &Viewport;
		ViewportStateCreateInfo.scissorCount = 1;
		ViewportStateCreateInfo.pScissors = &Scissor;

		std::vector<char> TerrainVertexShaderCode;
		std::vector<char> TerrainFragmentShaderCode;
		std::vector<char> VertexShaderCode;
		std::vector<char> FragmentShaderCode;
		std::vector<char> SecondVertexShaderCode;
		std::vector<char> SecondFragmentShaderCode;

		Util::OpenAndReadFileFull("./Resources/Shaders/TerrainGenerator_vert.spv", TerrainVertexShaderCode, "rb");
		Util::OpenAndReadFileFull("./Resources/Shaders/TerrainGenerator_frag.spv", TerrainFragmentShaderCode, "rb");
		Util::OpenAndReadFileFull(Util::UtilHelper::GetVertexShaderPath().data(), VertexShaderCode, "rb");
		Util::OpenAndReadFileFull(Util::UtilHelper::GetFragmentShaderPath().data(), FragmentShaderCode, "rb");
		Util::OpenAndReadFileFull("./Resources/Shaders/second_vert.spv", SecondVertexShaderCode, "rb");
		Util::OpenAndReadFileFull("./Resources/Shaders/second_frag.spv", SecondFragmentShaderCode, "rb");

		VkShaderModule TerrainVertexShaderModule;
		VkShaderModule TerrainFragmentShaderModule;
		VkShaderModule VertexShaderModule;
		VkShaderModule FragmentShaderModule;
		VkShaderModule SecondVertexShaderModule;
		VkShaderModule SecondFragmentShaderModule;

		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(TerrainVertexShaderCode.data()),
			TerrainVertexShaderCode.size(), TerrainVertexShaderModule);
		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(TerrainFragmentShaderCode.data()),
			TerrainFragmentShaderCode.size(), TerrainFragmentShaderModule);
		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(VertexShaderCode.data()),
			VertexShaderCode.size(), VertexShaderModule);
		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(FragmentShaderCode.data()),
			FragmentShaderCode.size(), FragmentShaderModule);
		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(SecondVertexShaderCode.data()),
			SecondVertexShaderCode.size(), SecondVertexShaderModule);
		CreateShader(LogicalDevice, reinterpret_cast<const uint32_t*>(SecondFragmentShaderCode.data()),
			SecondFragmentShaderCode.size(), SecondFragmentShaderModule);

		VkPipelineShaderStageCreateInfo TerrainVertexShaderCreateInfo = { };
		TerrainVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		TerrainVertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		TerrainVertexShaderCreateInfo.module = TerrainVertexShaderModule;
		TerrainVertexShaderCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo TerrainFragmentShaderCreateInfo = { };
		TerrainFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		TerrainFragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		TerrainFragmentShaderCreateInfo.module = TerrainFragmentShaderModule;
		TerrainFragmentShaderCreateInfo.pName = "main";

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

		const uint32_t TerrainShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo TerrainShaderStages[TerrainShaderStagesCount] = {
			TerrainVertexShaderCreateInfo, TerrainFragmentShaderCreateInfo
		};

		const uint32_t ShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo ShaderStages[ShaderStagesCount] = {
			VertexShaderCreateInfo, FragmentShaderCreateInfo
		};

		const uint32_t SecondShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo SecondShaderStages[SecondShaderStagesCount] = {
			SecondVertexShaderCreateInfo, SecondFragmentShaderCreateInfo
		};

		VkVertexInputBindingDescription TerrainVertexInputBindingDescription = { };
		TerrainVertexInputBindingDescription.binding = 0;
		TerrainVertexInputBindingDescription.stride = sizeof(float);
		TerrainVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		const uint32_t TerrainVertexInputBindingDescriptionCount = 1;
		VkVertexInputAttributeDescription TerrainAttributeDescriptions[TerrainVertexInputBindingDescriptionCount];

		TerrainAttributeDescriptions[0].binding = 0;
		TerrainAttributeDescriptions[0].location = 0;
		TerrainAttributeDescriptions[0].format = VK_FORMAT_R32_SFLOAT;
		TerrainAttributeDescriptions[0].offset = 0;

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

		VkPipelineVertexInputStateCreateInfo TerrainVertexInputCreateInfo = { };
		TerrainVertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		TerrainVertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		TerrainVertexInputCreateInfo.pVertexBindingDescriptions = &TerrainVertexInputBindingDescription;
		TerrainVertexInputCreateInfo.vertexAttributeDescriptionCount = TerrainVertexInputBindingDescriptionCount;
		TerrainVertexInputCreateInfo.pVertexAttributeDescriptions = TerrainAttributeDescriptions;

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
		const uint32_t GraphicsPipelineCreateInfosCount = 3;
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
		GraphicsPipelineCreateInfos[0].layout = EntityPass.PipelineLayout;							// Pipeline Layout pipeline should use
		GraphicsPipelineCreateInfos[0].renderPass = RenderPass;							// Render pass description the pipeline is compatible with
		GraphicsPipelineCreateInfos[0].subpass = 1;										// Subpass of render pass to use with pipeline

		// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
		GraphicsPipelineCreateInfos[0].basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
		GraphicsPipelineCreateInfos[0].basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)

		GraphicsPipelineCreateInfos[1] = GraphicsPipelineCreateInfos[0];
		GraphicsPipelineCreateInfos[1].pDepthStencilState = &SecondDepthStencilCreateInfo;
		GraphicsPipelineCreateInfos[1].pVertexInputState = &SecondVertexInputCreateInfo;
		GraphicsPipelineCreateInfos[1].pStages = SecondShaderStages;	// Update second shader stage list
		GraphicsPipelineCreateInfos[1].layout = DeferredPass.PipelineLayout;	// Change pipeline layout for input attachment descriptor sets
		GraphicsPipelineCreateInfos[1].subpass = 2;						// Use second subpass


		GraphicsPipelineCreateInfos[2] = GraphicsPipelineCreateInfos[0];
		GraphicsPipelineCreateInfos[2].pVertexInputState = &TerrainVertexInputCreateInfo;
		GraphicsPipelineCreateInfos[2].pStages = TerrainShaderStages;
		GraphicsPipelineCreateInfos[2].layout = TerrainPass.PipelineLayout;
		GraphicsPipelineCreateInfos[2].subpass = 0;

		VkPipeline Pipelines[3];

		const VkResult Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, GraphicsPipelineCreateInfosCount,
			GraphicsPipelineCreateInfos, nullptr, Pipelines);

		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateGraphicsPipelines result is {}", static_cast<int>(Result));
			assert(false);
		}

		TerrainPass.Pipeline = Pipelines[2];
		EntityPass.Pipeline = Pipelines[0];
		DeferredPass.Pipeline = Pipelines[1];

		vkDestroyShaderModule(LogicalDevice, TerrainFragmentShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, TerrainVertexShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, FragmentShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, VertexShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, SecondFragmentShaderModule, nullptr);
		vkDestroyShaderModule(LogicalDevice, SecondVertexShaderModule, nullptr);
	}

	void MainRenderPass::CreateAttachments(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, uint32_t ImagesCount, VkExtent2D SwapExtent,
		VkFormat DepthFormat, VkFormat ColorFormat)
	{
		DepthBuffers = Util::Memory::Allocate<ImageBuffer>(ImagesCount);
		ColorBuffers = Util::Memory::Allocate<ImageBuffer>(ImagesCount);

		for (uint32_t i = 0; i < ImagesCount; ++i)
		{
			DepthBuffers[i].Image = CreateImage(PhysicalDevice, LogicalDevice, SwapExtent.width, SwapExtent.height,
				DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&DepthBuffers[i].Memory);

			DepthBuffers[i].ImageView = CreateImageView(LogicalDevice, DepthBuffers[i].Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

			// VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT says that image can be used only as attachment. Used only for sub pass
			ColorBuffers[i].Image = CreateImage(PhysicalDevice, LogicalDevice, SwapExtent.width, SwapExtent.height,
				ColorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&ColorBuffers[i].Memory);

			ColorBuffers[i].ImageView = CreateImageView(LogicalDevice, ColorBuffers[i].Image, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void MainRenderPass::CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
		uint32_t ImagesCount)
	{
		const VkDeviceSize VpBufferSize = sizeof(UboViewProjection);

		VpUniformBuffers = Util::Memory::Allocate<GPUBuffer>(ImagesCount);
		for (uint32_t i = 0; i < ImagesCount; i++)
		{
			VpUniformBuffers[i] = GPUBuffer::CreateUniformBuffer(PhysicalDevice, LogicalDevice, VpBufferSize);
		}
	}

	void MainRenderPass::CreateSets(VkDevice LogicalDevice, VkDescriptorPool DescriptorPool, uint32_t ImagesCount)
	{
		TerrainPass.TerrainSets = Util::Memory::Allocate<VkDescriptorSet>(ImagesCount);
		EntityPass.EntitySets = Util::Memory::Allocate<VkDescriptorSet>(ImagesCount);
		DeferredPass.DeferredSets = Util::Memory::Allocate<VkDescriptorSet>(ImagesCount);

		VkDescriptorSetLayout* SetLayouts = Util::Memory::Allocate<VkDescriptorSetLayout>(ImagesCount);
		for (uint32_t i = 0; i < ImagesCount; i++)
		{
			SetLayouts[i] = EntityPass.EntitySetLayout;
		}

		CreateDescriptorSets(LogicalDevice, DescriptorPool, SetLayouts, ImagesCount, EntityPass.EntitySets);

		//TODO FIX
		CreateDescriptorSets(LogicalDevice, DescriptorPool, SetLayouts, ImagesCount, TerrainPass.TerrainSets);

		VkDescriptorBufferInfo VpBufferInfo = { };
		// Update all of descriptor set buffer bindings
		for (uint32_t i = 0; i < ImagesCount; i++)
		{
			// Todo: validate
			VpBufferInfo.buffer = VpUniformBuffers[i].Buffer;
			VpBufferInfo.offset = 0;
			VpBufferInfo.range = sizeof(UboViewProjection);

			VkWriteDescriptorSet VpSetWrite = { };
			VpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			VpSetWrite.dstSet = EntityPass.EntitySets[i];
			VpSetWrite.dstBinding = 0;
			VpSetWrite.dstArrayElement = 0;
			VpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			VpSetWrite.descriptorCount = 1;
			VpSetWrite.pBufferInfo = &VpBufferInfo;

			const uint32_t WriteSetsCount = 1;
			// Todo: do not copy
			VkWriteDescriptorSet WriteSets[WriteSetsCount] = { VpSetWrite };

			// Todo: use one vkUpdateDescriptorSets call to update all descriptors
			vkUpdateDescriptorSets(LogicalDevice, WriteSetsCount, WriteSets, 0, nullptr);

			//TODO FIX
			VpSetWrite.dstSet = TerrainPass.TerrainSets[i];
			VkWriteDescriptorSet WriteSets2[WriteSetsCount] = { VpSetWrite };
			vkUpdateDescriptorSets(LogicalDevice, WriteSetsCount, WriteSets2, 0, nullptr);
		}

		for (uint32_t i = 0; i < ImagesCount; i++)
		{
			SetLayouts[i] = DeferredPass.DeferredLayout;
		}

		CreateDescriptorSets(LogicalDevice, DescriptorPool, SetLayouts, ImagesCount, DeferredPass.DeferredSets);

		// Todo: move this and previus loop to function?
		for (size_t i = 0; i < ImagesCount; i++)
		{
			// Todo: move from loop?
			VkDescriptorImageInfo ColourAttachmentDescriptor = { };
			ColourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColourAttachmentDescriptor.imageView = ColorBuffers[i].ImageView;
			ColourAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkWriteDescriptorSet ColourWrite = { };
			ColourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ColourWrite.dstSet = DeferredPass.DeferredSets[i];
			ColourWrite.dstBinding = 0;
			ColourWrite.dstArrayElement = 0;
			ColourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			ColourWrite.descriptorCount = 1;
			ColourWrite.pImageInfo = &ColourAttachmentDescriptor;

			VkDescriptorImageInfo DepthAttachmentDescriptor = { };
			DepthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthAttachmentDescriptor.imageView = DepthBuffers[i].ImageView;
			DepthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkWriteDescriptorSet DepthWrite = { };
			DepthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DepthWrite.dstSet = DeferredPass.DeferredSets[i];
			DepthWrite.dstBinding = 1;
			DepthWrite.dstArrayElement = 0;
			DepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			DepthWrite.descriptorCount = 1;
			DepthWrite.pImageInfo = &DepthAttachmentDescriptor;


			// Todo: do not copy
			const uint32_t CetWritesCount = 2;
			VkWriteDescriptorSet SetWrites[CetWritesCount] = { ColourWrite, DepthWrite };

			// Update descriptor sets
			vkUpdateDescriptorSets(LogicalDevice, CetWritesCount, SetWrites, 0, nullptr);
		}

		Util::Memory::Deallocate(SetLayouts);
	}

	void EntitySubpass::ClearResources(VkDevice LogicalDevice)
	{

		vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
		vkDestroyPipeline(LogicalDevice, Pipeline, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, EntitySetLayout, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, SamplerSetLayout, nullptr);

		Util::Memory::Deallocate(EntitySets);
	}

	void DeferredSubpass::ClearResources(VkDevice LogicalDevice)
	{
		vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
		vkDestroyPipeline(LogicalDevice, Pipeline, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, DeferredLayout, nullptr);

		Util::Memory::Deallocate(DeferredSets);
	}

	void TerrainSubpass::ClearResources(VkDevice LogicalDevice)
	{
		vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
		vkDestroyPipeline(LogicalDevice, Pipeline, nullptr);
		vkDestroyDescriptorSetLayout(LogicalDevice, TerrainSetLayout, nullptr);

		Util::Memory::Deallocate(TerrainSets);
	}
}
