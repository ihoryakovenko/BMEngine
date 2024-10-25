#include "MainRenderPass.h"

#include "Util/Util.h"
#include "VulkanHelper.h"
#include "VulkanMemoryManagementSystem.h"

#include "Memory/MemoryManagmentSystem.h"

namespace Core
{
	namespace SubpassIndex
	{
		enum
		{
			MainSubpass = 0,
			DeferredSubpas, // Fullscreen effects

			Count
		};
	}

	void BMRMainRenderPass::ClearResources(VkDevice LogicalDevice, u32 ImagesCount)
	{
		for (u32 i = 0; i < ImagesCount; ++i)
		{
			vkDestroyImageView(LogicalDevice, DepthBufferViews[i], nullptr);
			VulkanMemoryManagementSystem::DestroyImageBuffer(DepthBuffers[i]);

			vkDestroyImageView(LogicalDevice, ColorBufferViews[i], nullptr);
			VulkanMemoryManagementSystem::DestroyImageBuffer(ColorBuffers[i]);

			VulkanMemoryManagementSystem::DestroyBuffer(VpUniformBuffers[i]);
			VulkanMemoryManagementSystem::DestroyBuffer(LightBuffers[i]);
		}

		for (u32 i = 0; i < DescriptorLayoutHandles::Count; ++i)
		{
			vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorLayouts[i], nullptr);
		}

		for (u32 i = 0; i < PipelineHandles::Count; ++i)
		{
			vkDestroyPipelineLayout(LogicalDevice, PipelineLayouts[i], nullptr);
			vkDestroyPipeline(LogicalDevice, Pipelines[i], nullptr);
		}

		VulkanMemoryManagementSystem::DestroyBuffer(MaterialBuffer);

		vkDestroyRenderPass(LogicalDevice, RenderPass, nullptr);
	}

	void BMRMainRenderPass::CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat, VkSurfaceFormatKHR SurfaceFormat)
	{
		VkSubpassDescription Subpasses[SubpassIndex::Count];

		const u32 SwapchainColorAttachmentIndex = 0;
		const u32 SubpassColorAttachmentIndex = 1;
		const u32 SubpassDepthAttachmentIndex = 2;

		const u32 AttachmentDescriptionsCount = 3;
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

		// Define the Entity subpass, which will read the color and depth attachments
		VkAttachmentReference EntitySubpassColourAttachmentReference = { };
		EntitySubpassColourAttachmentReference.attachment = SubpassColorAttachmentIndex; // Index of the color attachment (shared with Terrain subpass)
		EntitySubpassColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference EntitySubpassDepthAttachmentReference = { };
		EntitySubpassDepthAttachmentReference.attachment = SubpassDepthAttachmentIndex; // Depth attachment (shared with Terrain subpass)
		EntitySubpassDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		Subpasses[SubpassIndex::MainSubpass] = { };
		Subpasses[SubpassIndex::MainSubpass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[SubpassIndex::MainSubpass].colorAttachmentCount = 1;
		Subpasses[SubpassIndex::MainSubpass].pColorAttachments = &EntitySubpassColourAttachmentReference;
		Subpasses[SubpassIndex::MainSubpass].pDepthStencilAttachment = &EntitySubpassDepthAttachmentReference;

		// Deferred subpass (Fullscreen effects)
		VkAttachmentReference SwapchainColorAttachmentReference = { };
		SwapchainColorAttachmentReference.attachment = SwapchainColorAttachmentIndex; // Swapchain color attachment
		SwapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		const u32 InputReferencesCount = 2;
		VkAttachmentReference InputReferences[InputReferencesCount];
		InputReferences[0].attachment = 1;
		InputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		InputReferences[1].attachment = 2;
		InputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		Subpasses[SubpassIndex::DeferredSubpas] = { };
		Subpasses[SubpassIndex::DeferredSubpas].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpasses[SubpassIndex::DeferredSubpas].colorAttachmentCount = 1;
		Subpasses[SubpassIndex::DeferredSubpas].pColorAttachments = &SwapchainColorAttachmentReference;
		Subpasses[SubpassIndex::DeferredSubpas].inputAttachmentCount = InputReferencesCount;
		Subpasses[SubpassIndex::DeferredSubpas].pInputAttachments = InputReferences;

		// Subpass dependencies
		const u32 ExitDependenciesIndex = SubpassIndex::DeferredSubpas + 1;
		const u32 SubpassDependenciesCount = SubpassIndex::Count + 1;
		VkSubpassDependency SubpassDependencies[SubpassDependenciesCount];

		// Transition from external to Terrain subpass
		// Transition must happen after...
		SubpassDependencies[SubpassIndex::MainSubpass].srcSubpass = VK_SUBPASS_EXTERNAL;
		SubpassDependencies[SubpassIndex::MainSubpass].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		SubpassDependencies[SubpassIndex::MainSubpass].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// But must happen before...
		SubpassDependencies[SubpassIndex::MainSubpass].dstSubpass = SubpassIndex::DeferredSubpas;
		SubpassDependencies[SubpassIndex::MainSubpass].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[SubpassIndex::MainSubpass].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		SubpassDependencies[SubpassIndex::MainSubpass].dependencyFlags = 0;

		// Transition from Entity subpass to Deferred subpass
		// Transition must happen after...
		SubpassDependencies[SubpassIndex::DeferredSubpas].srcSubpass = SubpassIndex::MainSubpass;
		SubpassDependencies[SubpassIndex::DeferredSubpas].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependencies[SubpassIndex::DeferredSubpas].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// But must happen before...
		SubpassDependencies[SubpassIndex::DeferredSubpas].dstSubpass = SubpassIndex::DeferredSubpas;
		SubpassDependencies[SubpassIndex::DeferredSubpas].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		SubpassDependencies[SubpassIndex::DeferredSubpas].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		SubpassDependencies[SubpassIndex::DeferredSubpas].dependencyFlags = 0;

		// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		// Transition must happen after...
		SubpassDependencies[ExitDependenciesIndex].srcSubpass = SubpassIndex::DeferredSubpas;
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
		RenderPassCreateInfo.subpassCount = SubpassIndex::Count;
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

	void BMRMainRenderPass::SetupPushConstants()
	{
		PushConstants[PushConstantHandles::Entity].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants[PushConstantHandles::Entity].offset = 0;
		// Todo: check constant and model size?
		PushConstants[PushConstantHandles::Entity].size = sizeof(BMRModel);
	}

	void BMRMainRenderPass::CreateSamplerSetLayout(VkDevice LogicalDevice)
	{
		const u32 EntitySamplerLayoutBindingCount = 2;
		VkDescriptorSetLayoutBinding EntitySamplerLayoutBinding[EntitySamplerLayoutBindingCount];
		EntitySamplerLayoutBinding[0].binding = 0;
		EntitySamplerLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		EntitySamplerLayoutBinding[0].descriptorCount = 1;
		EntitySamplerLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		EntitySamplerLayoutBinding[0].pImmutableSamplers = nullptr;

		EntitySamplerLayoutBinding[1] = EntitySamplerLayoutBinding[0];
		EntitySamplerLayoutBinding[1].binding = 1;


		VkDescriptorSetLayoutCreateInfo EntityTextureLayoutCreateInfo = { };
		EntityTextureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		EntityTextureLayoutCreateInfo.bindingCount = EntitySamplerLayoutBindingCount;
		EntityTextureLayoutCreateInfo.pBindings = EntitySamplerLayoutBinding;

		VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &EntityTextureLayoutCreateInfo,
			nullptr, &DescriptorLayouts[DescriptorLayoutHandles::EntitySampler]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorSetLayout result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkDescriptorSetLayoutBinding TerrainSamplerLayoutBinding = { };
		TerrainSamplerLayoutBinding.binding = 0;
		TerrainSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		TerrainSamplerLayoutBinding.descriptorCount = 1;
		TerrainSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		TerrainSamplerLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo TerrainTextureLayoutCreateInfo = { };
		TerrainTextureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		TerrainTextureLayoutCreateInfo.bindingCount = 1;
		TerrainTextureLayoutCreateInfo.pBindings = &TerrainSamplerLayoutBinding;

		Result = vkCreateDescriptorSetLayout(LogicalDevice, &TerrainTextureLayoutCreateInfo,
			nullptr, &DescriptorLayouts[DescriptorLayoutHandles::TerrainSampler]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorSetLayout result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void BMRMainRenderPass::CreateTerrainSetLayout(VkDevice LogicalDevice)
	{
		VkDescriptorSetLayoutBinding VpLayoutBinding = { };
		VpLayoutBinding.binding = 0;											// Binding point in shader (designated by binding number in shader)
		VpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Type of descriptor (uniform, dynamic uniform, image sampler, etc)
		VpLayoutBinding.descriptorCount = 1;									// Number of descriptors for binding
		VpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage to bind to
		VpLayoutBinding.pImmutableSamplers = nullptr;							// For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

		const u32 BindingCount = 1;
		VkDescriptorSetLayoutBinding Bindings[BindingCount] = { VpLayoutBinding };

		// Create Descriptor Set Layout with given bindings
		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = BindingCount;					// Number of binding infos
		LayoutCreateInfo.pBindings = Bindings;		// Array of binding infos

		VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo,
			nullptr, &DescriptorLayouts[DescriptorLayoutHandles::TerrainVp]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorSetLayout result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void BMRMainRenderPass::CreateEntitySetLayout(VkDevice LogicalDevice)
	{
		VkDescriptorSetLayoutBinding VpLayoutBinding = { };
		VpLayoutBinding.binding = 0;
		VpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VpLayoutBinding.descriptorCount = 1;
		VpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		VpLayoutBinding.pImmutableSamplers = nullptr; // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

		const u32 BindingCount = 1;

		// Create Descriptor Set Layout with given bindings
		VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = { };
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutCreateInfo.bindingCount = BindingCount;					// Number of binding infos
		LayoutCreateInfo.pBindings = &VpLayoutBinding;		// Array of binding infos

		VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo,
			nullptr, &DescriptorLayouts[DescriptorLayoutHandles::EntityVp]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorSetLayout result is {}", static_cast<int>(Result));
			assert(false);
		}

		const u32 LightBindingsCount = 2;
		VkDescriptorSetLayoutBinding LightBindings[LightBindingsCount];
		LightBindings[0] = { };
		LightBindings[0].binding = 0;
		LightBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		LightBindings[0].descriptorCount = 1;
		LightBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		LightBindings[0].pImmutableSamplers = nullptr;

		LightBindings[1] = LightBindings[0];
		LightBindings[1].binding = 1;

		LayoutCreateInfo.bindingCount = LightBindingsCount;
		LayoutCreateInfo.pBindings = LightBindings;

		Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo,
			nullptr, &DescriptorLayouts[DescriptorLayoutHandles::Light]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorSetLayout result is {}", static_cast<int>(Result));
			assert(false);
		}

		VkDescriptorSetLayoutBinding materialLayoutBinding = { };
		materialLayoutBinding.binding = 0;
		materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		materialLayoutBinding.descriptorCount = 1;
		materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		materialLayoutBinding.pImmutableSamplers = nullptr;

		LayoutCreateInfo.bindingCount = 1;
		LayoutCreateInfo.pBindings = &materialLayoutBinding;

		Result = vkCreateDescriptorSetLayout(LogicalDevice, &LayoutCreateInfo,
			nullptr, &DescriptorLayouts[DescriptorLayoutHandles::Material]);
		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateDescriptorSetLayout result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void BMRMainRenderPass::CreateDeferredSetLayout(VkDevice LogicalDevice)
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
		const u32 InputBindingsCount = 2;
		// Todo: do not copy InputBindings
		VkDescriptorSetLayoutBinding InputBindings[InputBindingsCount] = { ColourInputLayoutBinding, DepthInputLayoutBinding };

		// Create a descriptor set layout for input attachments
		VkDescriptorSetLayoutCreateInfo InputLayoutCreateInfo = { };
		InputLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		InputLayoutCreateInfo.bindingCount = InputBindingsCount;
		InputLayoutCreateInfo.pBindings = InputBindings;

		// Create Descriptor Set Layout
		const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &InputLayoutCreateInfo,
			nullptr, &DescriptorLayouts[DescriptorLayoutHandles::DeferredInput]);
		if (Result != VK_SUCCESS)
		{
			// TODO LOG
			assert(false);
		}
	}

	void BMRMainRenderPass::CreatePipelineLayouts(VkDevice LogicalDevice)
	{
		const u32 TerrainDescriptorLayoutsCount = 2;
		VkDescriptorSetLayout TerrainDescriptorLayouts[TerrainDescriptorLayoutsCount] = {
			DescriptorLayouts[DescriptorLayoutHandles::TerrainVp],
			DescriptorLayouts[DescriptorLayoutHandles::TerrainSampler]
		};

		PipelineLayouts[PipelineHandles::Terrain] = CreatePipelineLayout(LogicalDevice,
			TerrainDescriptorLayoutsCount, TerrainDescriptorLayouts, 0, nullptr);

		const u32 EntityDescriptorLayoutCount = 4;
		VkDescriptorSetLayout EntityDescriptorLayouts[EntityDescriptorLayoutCount] = {
			DescriptorLayouts[DescriptorLayoutHandles::EntityVp],
			DescriptorLayouts[DescriptorLayoutHandles::EntitySampler],
			DescriptorLayouts[DescriptorLayoutHandles::Light],
			DescriptorLayouts[DescriptorLayoutHandles::Material]
		};

		PipelineLayouts[PipelineHandles::Entity] = CreatePipelineLayout(LogicalDevice,
			EntityDescriptorLayoutCount, EntityDescriptorLayouts, 1, &PushConstants[PushConstantHandles::Entity]);

		PipelineLayouts[PipelineHandles::Deferred] = CreatePipelineLayout(LogicalDevice,
			1, &DescriptorLayouts[DescriptorLayoutHandles::DeferredInput], 0, nullptr);

		//PipelineLayouts[PipelineHandles::SkyBoxPipeline] = CreatePipelineLayout(LogicalDevice, 1,
		//	&DescriptorLayouts[DescriptorLayoutHandles::SkyBoxVb], 0, nullptr);
	}

	void BMRMainRenderPass::CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent, BMRShaderInput* ShaderInputs, u32 ShaderInputsCount)
	{
		VkViewport Viewport;
		VkRect2D Scissor;

		Viewport.x = 0.0f;
		Viewport.y = 0.0f;
		Viewport.width = static_cast<f32>(SwapExtent.width);
		Viewport.height = static_cast<f32>(SwapExtent.height);
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

		VkPipelineShaderStageCreateInfo TerrainVertexShaderCreateInfo = { };
		TerrainVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		TerrainVertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		TerrainVertexShaderCreateInfo.module = BMRShaderInput::FindShaderModuleByName(TERRAIN_VERTEX, ShaderInputs, ShaderInputsCount);
		TerrainVertexShaderCreateInfo.pName = "main";
		
		VkPipelineShaderStageCreateInfo TerrainFragmentShaderCreateInfo = { };
		TerrainFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		TerrainFragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		TerrainFragmentShaderCreateInfo.module = BMRShaderInput::FindShaderModuleByName(TERRAIN_FRAGMENT, ShaderInputs, ShaderInputsCount);
		TerrainFragmentShaderCreateInfo.pName = "main";
		
		VkPipelineShaderStageCreateInfo EntityVertexShaderCreateInfo = { };
		EntityVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		EntityVertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		EntityVertexShaderCreateInfo.module = BMRShaderInput::FindShaderModuleByName(ENTITY_VERTEX, ShaderInputs, ShaderInputsCount);
		EntityVertexShaderCreateInfo.pName = "main";
		
		VkPipelineShaderStageCreateInfo EntityFragmentShaderCreateInfo = { };
		EntityFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		EntityFragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		EntityFragmentShaderCreateInfo.module = BMRShaderInput::FindShaderModuleByName(ENTITY_FRAGMENT, ShaderInputs, ShaderInputsCount);
		EntityFragmentShaderCreateInfo.pName = "main";
		
		VkPipelineShaderStageCreateInfo DeferredVertexShaderCreateInfo = { };
		DeferredVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		DeferredVertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		DeferredVertexShaderCreateInfo.module = BMRShaderInput::FindShaderModuleByName(DEFERRED_VERTEX, ShaderInputs, ShaderInputsCount);
		DeferredVertexShaderCreateInfo.pName = "main";
		
		VkPipelineShaderStageCreateInfo DeferredFragmentShaderCreateInfo = { };
		DeferredFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		DeferredFragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		DeferredFragmentShaderCreateInfo.module = BMRShaderInput::FindShaderModuleByName(DEFERRED_FRAGMENT, ShaderInputs, ShaderInputsCount);
		DeferredFragmentShaderCreateInfo.pName = "main";

		const u32 TerrainShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo TerrainShaderStages[TerrainShaderStagesCount] = {
			TerrainVertexShaderCreateInfo, TerrainFragmentShaderCreateInfo
		};

		const u32 EntityShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo EntityShaderStages[EntityShaderStagesCount] = {
			EntityVertexShaderCreateInfo, EntityFragmentShaderCreateInfo
		};

		const u32 DeferredShaderStagesCount = 2;
		VkPipelineShaderStageCreateInfo DeferredShaderStages[DeferredShaderStagesCount] = {
			DeferredVertexShaderCreateInfo, DeferredFragmentShaderCreateInfo
		};

		VkVertexInputBindingDescription TerrainVertexInputBindingDescription = { };
		TerrainVertexInputBindingDescription.binding = 0;
		TerrainVertexInputBindingDescription.stride = sizeof(BMRTerrainVertex);
		TerrainVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		const u32 TerrainVertexInputBindingDescriptionCount = 1;
		VkVertexInputAttributeDescription TerrainAttributeDescriptions[TerrainVertexInputBindingDescriptionCount];

		TerrainAttributeDescriptions[0].binding = 0;
		TerrainAttributeDescriptions[0].location = 0;
		TerrainAttributeDescriptions[0].format = VK_FORMAT_R32_SFLOAT;
		TerrainAttributeDescriptions[0].offset = offsetof(BMRTerrainVertex, Altitude);

		// How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
		VkVertexInputBindingDescription EntityInputBindingDescription = { };
		EntityInputBindingDescription.binding = 0;									// Can bind multiple streams of data, this defines which one
		EntityInputBindingDescription.stride = sizeof(BMREntityVertex);						// Size of a single vertex object
		EntityInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// How to move between data after each vertex.
		// VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex
		// VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance

		// How the data for an attribute is defined within a vertex
		const u32 EntityVertexInputBindingDescriptionCount = 4;
		VkVertexInputAttributeDescription EntityAttributeDescriptions[EntityVertexInputBindingDescriptionCount];

		// Position Attribute
		EntityAttributeDescriptions[0].binding = 0;							// Which binding the data is at (should be same as above)
		EntityAttributeDescriptions[0].location = 0;							// Location in shader where data will be read from
		EntityAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Format the data will take (also helps define size of data)
		EntityAttributeDescriptions[0].offset = offsetof(BMREntityVertex, Position);		// Where this attribute is defined in the data for a single vertex

		// Color Attribute
		EntityAttributeDescriptions[1].binding = 0;
		EntityAttributeDescriptions[1].location = 1;
		EntityAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		EntityAttributeDescriptions[1].offset = offsetof(BMREntityVertex, Color);

		// Texture Attribute
		EntityAttributeDescriptions[2].binding = 0;
		EntityAttributeDescriptions[2].location = 2;
		EntityAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		EntityAttributeDescriptions[2].offset = offsetof(BMREntityVertex, TextureCoords);

		// Normal Attribute
		EntityAttributeDescriptions[3].binding = 0;
		EntityAttributeDescriptions[3].location = 3;
		EntityAttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		EntityAttributeDescriptions[3].offset = offsetof(BMREntityVertex, Normal);

		VkPipelineVertexInputStateCreateInfo TerrainVertexInputCreateInfo = { };
		TerrainVertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		TerrainVertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		TerrainVertexInputCreateInfo.pVertexBindingDescriptions = &TerrainVertexInputBindingDescription;
		TerrainVertexInputCreateInfo.vertexAttributeDescriptionCount = TerrainVertexInputBindingDescriptionCount;
		TerrainVertexInputCreateInfo.pVertexAttributeDescriptions = TerrainAttributeDescriptions;

		// Vertex input
		VkPipelineVertexInputStateCreateInfo EntityVertexInputCreateInfo = { };
		EntityVertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		EntityVertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		EntityVertexInputCreateInfo.pVertexBindingDescriptions = &EntityInputBindingDescription;
		EntityVertexInputCreateInfo.vertexAttributeDescriptionCount = EntityVertexInputBindingDescriptionCount;
		EntityVertexInputCreateInfo.pVertexAttributeDescriptions = EntityAttributeDescriptions;

		VkPipelineVertexInputStateCreateInfo DeferredVertexInputCreateInfo = { };
		// No vertex data for second pass
		DeferredVertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		DeferredVertexInputCreateInfo.vertexBindingDescriptionCount = 0;
		DeferredVertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
		DeferredVertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
		DeferredVertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

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
		//DynamicStateCreateInfo.dynamicStateCount = static_cast<u32>(2);
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

		VkPipelineColorBlendAttachmentState DeferredColorBlendAttachmentState = { };
		DeferredColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;  // Write to all color components
		DeferredColorBlendAttachmentState.blendEnable = VK_FALSE;

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

		VkPipelineColorBlendStateCreateInfo DeferredColorBlendingCreateInfo = { };
		DeferredColorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		DeferredColorBlendingCreateInfo.logicOpEnable = VK_FALSE;  // No logical operations
		DeferredColorBlendingCreateInfo.attachmentCount = 1;
		DeferredColorBlendingCreateInfo.pAttachments = &DeferredColorBlendAttachmentState;

		// Depth stencil testing
		VkPipelineDepthStencilStateCreateInfo EntityDepthStencilCreateInfo = { };
		EntityDepthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		EntityDepthStencilCreateInfo.depthTestEnable = VK_TRUE;				// Enable checking depth to determine fragment write
		EntityDepthStencilCreateInfo.depthWriteEnable = VK_TRUE;				// Enable writing to depth buffer (to replace old values)
		EntityDepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;		// Comparison operation that allows an overwrite (is in front)
		EntityDepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;		// Depth Bounds Test: Does the depth value exist between two bounds
		EntityDepthStencilCreateInfo.stencilTestEnable = VK_FALSE;			// Enable Stencil Test

		VkPipelineDepthStencilStateCreateInfo DeferredDepthStencilCreateInfo = EntityDepthStencilCreateInfo;
		DeferredDepthStencilCreateInfo.depthWriteEnable = VK_FALSE;

		VkGraphicsPipelineCreateInfo PipelineCreateInfos[PipelineHandles::Count];

		PipelineCreateInfos[PipelineHandles::Entity] = { };
		PipelineCreateInfos[PipelineHandles::Entity].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		PipelineCreateInfos[PipelineHandles::Entity].stageCount = EntityShaderStagesCount;					// Number of shader stages
		PipelineCreateInfos[PipelineHandles::Entity].pStages = EntityShaderStages;							// List of shader stages
		PipelineCreateInfos[PipelineHandles::Entity].pVertexInputState = &EntityVertexInputCreateInfo;		// All the fixed function pipeline states
		PipelineCreateInfos[PipelineHandles::Entity].pInputAssemblyState = &InputAssemblyStateCreateInfo;
		PipelineCreateInfos[PipelineHandles::Entity].pViewportState = &ViewportStateCreateInfo;
		PipelineCreateInfos[PipelineHandles::Entity].pDynamicState = nullptr;
		PipelineCreateInfos[PipelineHandles::Entity].pRasterizationState = &RasterizationStateCreateInfo;
		PipelineCreateInfos[PipelineHandles::Entity].pMultisampleState = &MultisampleStateCreateInfo;
		PipelineCreateInfos[PipelineHandles::Entity].pColorBlendState = &ColorBlendingCreateInfo;
		PipelineCreateInfos[PipelineHandles::Entity].pDepthStencilState = &EntityDepthStencilCreateInfo;
		PipelineCreateInfos[PipelineHandles::Entity].layout = PipelineLayouts[PipelineHandles::Entity];							// Pipeline Layout pipeline should use
		PipelineCreateInfos[PipelineHandles::Entity].renderPass = RenderPass;							// Render pass description the pipeline is compatible with
		PipelineCreateInfos[PipelineHandles::Entity].subpass = SubpassIndex::MainSubpass;										// Subpass of render pass to use with pipeline

		// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
		PipelineCreateInfos[PipelineHandles::Entity].basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
		PipelineCreateInfos[PipelineHandles::Entity].basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)

		PipelineCreateInfos[PipelineHandles::Deferred] = PipelineCreateInfos[PipelineHandles::Entity];
		PipelineCreateInfos[PipelineHandles::Deferred].pDepthStencilState = &DeferredDepthStencilCreateInfo;
		PipelineCreateInfos[PipelineHandles::Deferred].pVertexInputState = &DeferredVertexInputCreateInfo;
		PipelineCreateInfos[PipelineHandles::Deferred].pStages = DeferredShaderStages;	// Update second shader stage list
		PipelineCreateInfos[PipelineHandles::Deferred].layout = PipelineLayouts[PipelineHandles::Deferred];	// Change pipeline layout for input attachment descriptor sets
		PipelineCreateInfos[PipelineHandles::Deferred].subpass = SubpassIndex::DeferredSubpas;						// Use second subpass
		PipelineCreateInfos[PipelineHandles::Deferred].pColorBlendState = &DeferredColorBlendingCreateInfo;

		PipelineCreateInfos[PipelineHandles::Terrain] = PipelineCreateInfos[PipelineHandles::Entity];
		PipelineCreateInfos[PipelineHandles::Terrain].pVertexInputState = &TerrainVertexInputCreateInfo;
		PipelineCreateInfos[PipelineHandles::Terrain].pStages = TerrainShaderStages;
		PipelineCreateInfos[PipelineHandles::Terrain].layout = PipelineLayouts[PipelineHandles::Terrain];
		PipelineCreateInfos[PipelineHandles::Terrain].subpass = SubpassIndex::MainSubpass;

		//PipelineCreateInfos[PipelineHandles::SkyBoxPipeline]

		const VkResult Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, PipelineHandles::Count,
			PipelineCreateInfos, nullptr, Pipelines);

		if (Result != VK_SUCCESS)
		{
			Util::Log().Error("vkCreateGraphicsPipelines result is {}", static_cast<int>(Result));
			assert(false);
		}
	}

	void BMRMainRenderPass::CreateAttachments(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice, u32 ImagesCount, VkExtent2D SwapExtent,
		VkFormat DepthFormat, VkFormat ColorFormat)
	{
		for (u32 i = 0; i < ImagesCount; ++i)
		{
			DepthBuffers[i].Image = CreateImage(PhysicalDevice, LogicalDevice, SwapExtent.width, SwapExtent.height,
				DepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&DepthBuffers[i].Memory);

			DepthBufferViews[i] = CreateImageView(LogicalDevice, DepthBuffers[i].Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

			// VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT says that image can be used only as attachment. Used only for sub pass
			ColorBuffers[i].Image = CreateImage(PhysicalDevice, LogicalDevice, SwapExtent.width, SwapExtent.height,
				ColorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&ColorBuffers[i].Memory);

			ColorBufferViews[i] = CreateImageView(LogicalDevice, ColorBuffers[i].Image, ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void BMRMainRenderPass::CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
		u32 ImagesCount)
	{
		const VkDeviceSize VpBufferSize = sizeof(BMRUboViewProjection);
		const VkDeviceSize LightBufferSize = sizeof(BMRLightBuffer);
		const VkDeviceSize MaterialBufferSize = sizeof(BMRMaterial);

		//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);
		//const VkDeviceSize AlignedAmbientLightSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(AmbientLightBufferSize);
		//const VkDeviceSize AlignedPointLightSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(PointLightBufferSize);

		for (u32 i = 0; i < ImagesCount; i++)
		{
			VpUniformBuffers[i] = VulkanMemoryManagementSystem::CreateBuffer(VpBufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}

		for (u32 i = 0; i < ImagesCount; i++)
		{
			LightBuffers[i] = VulkanMemoryManagementSystem::CreateBuffer(LightBufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}

		MaterialBuffer = VulkanMemoryManagementSystem::CreateBuffer(MaterialBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void BMRMainRenderPass::CreateSets(VkDescriptorPool Pool, VkDevice LogicalDevice, u32 ImagesCount)
	{
		VkDescriptorSetLayout* SetLayouts = Memory::BmMemoryManagementSystem::FrameAlloc<VkDescriptorSetLayout>(ImagesCount);
		VkDescriptorSetLayout* TerrainLayouts = Memory::BmMemoryManagementSystem::FrameAlloc<VkDescriptorSetLayout>(ImagesCount);
		VkDescriptorSetLayout* LightingSetLayouts = Memory::BmMemoryManagementSystem::FrameAlloc<VkDescriptorSetLayout>(ImagesCount);
		VkDescriptorSetLayout* DeferredSetLayouts = Memory::BmMemoryManagementSystem::FrameAlloc<VkDescriptorSetLayout>(ImagesCount);

		for (u32 i = 0; i < ImagesCount; i++)
		{
			SetLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::EntityVp];
			TerrainLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::TerrainVp];
			LightingSetLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::Light];
			DeferredSetLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::DeferredInput];
		}

		VulkanMemoryManagementSystem::AllocateSets(Pool, SetLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::EntityVp]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, TerrainLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::TerrainVp]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, LightingSetLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::EntityLigh]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, DeferredSetLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::DeferredInput]);

		for (u32 i = 0; i < ImagesCount; i++)
		{
			VkDescriptorBufferInfo VpBufferInfo = { };
			VpBufferInfo.buffer = VpUniformBuffers[i].Buffer;
			VpBufferInfo.offset = 0;
			VpBufferInfo.range = sizeof(BMRUboViewProjection);

			VkDescriptorBufferInfo LightBufferInfo = { };
			LightBufferInfo.buffer = LightBuffers[i].Buffer;
			LightBufferInfo.offset = 0;
			LightBufferInfo.range = sizeof(BMRLightBuffer);

			VkWriteDescriptorSet VpSetWrite = { };
			VpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			VpSetWrite.dstSet = DescriptorsToImages[DescriptorHandles::EntityVp][i];
			VpSetWrite.dstBinding = 0;
			VpSetWrite.dstArrayElement = 0;
			VpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			VpSetWrite.descriptorCount = 1;
			VpSetWrite.pBufferInfo = &VpBufferInfo;

			VkWriteDescriptorSet LightSetWrite = { };
			LightSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			LightSetWrite.dstSet = DescriptorsToImages[DescriptorHandles::EntityLigh][i];
			LightSetWrite.dstBinding = 0;
			LightSetWrite.dstArrayElement = 0;
			LightSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			LightSetWrite.descriptorCount = 1;
			LightSetWrite.pBufferInfo = &LightBufferInfo;

			VkWriteDescriptorSet VpTerrainWrite = VpSetWrite;
			VpTerrainWrite.dstSet = DescriptorsToImages[DescriptorHandles::TerrainVp][i];

			VkDescriptorImageInfo ColourAttachmentDescriptor = { };
			ColourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColourAttachmentDescriptor.imageView = ColorBufferViews[i];
			ColourAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkWriteDescriptorSet ColourWrite = { };
			ColourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ColourWrite.dstSet = DescriptorsToImages[DescriptorHandles::DeferredInput][i];
			ColourWrite.dstBinding = 0;
			ColourWrite.dstArrayElement = 0;
			ColourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			ColourWrite.descriptorCount = 1;
			ColourWrite.pImageInfo = &ColourAttachmentDescriptor;

			VkDescriptorImageInfo DepthAttachmentDescriptor = { };
			DepthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthAttachmentDescriptor.imageView = DepthBufferViews[i];
			DepthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkWriteDescriptorSet DepthWrite = { };
			DepthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DepthWrite.dstSet = DescriptorsToImages[DescriptorHandles::DeferredInput][i];
			DepthWrite.dstBinding = 1;
			DepthWrite.dstArrayElement = 0;
			DepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			DepthWrite.descriptorCount = 1;
			DepthWrite.pImageInfo = &DepthAttachmentDescriptor;

			const u32 WriteSetsCount = 5;
			VkWriteDescriptorSet WriteSets[WriteSetsCount] = { VpSetWrite, VpTerrainWrite, LightSetWrite,
				ColourWrite, DepthWrite };

			vkUpdateDescriptorSets(LogicalDevice, WriteSetsCount, WriteSets, 0, nullptr);
		}

		VulkanMemoryManagementSystem::AllocateSets(Pool, &DescriptorLayouts[DescriptorLayoutHandles::Material], 1, &MaterialSet);

		VkDescriptorBufferInfo MaterialBufferInfo = { };
		MaterialBufferInfo.buffer = MaterialBuffer.Buffer;
		MaterialBufferInfo.offset = 0;
		MaterialBufferInfo.range = sizeof(BMRMaterial);

		VkWriteDescriptorSet MaterialSetWrite = { };
		MaterialSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		MaterialSetWrite.dstSet = MaterialSet;
		MaterialSetWrite.dstBinding = 0;
		MaterialSetWrite.dstArrayElement = 0;
		MaterialSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		MaterialSetWrite.descriptorCount = 1;
		MaterialSetWrite.pBufferInfo = &MaterialBufferInfo;

		vkUpdateDescriptorSets(LogicalDevice, 1, &MaterialSetWrite, 0, nullptr);
	}
	VkShaderModule BMRShaderInput::FindShaderModuleByName(BMRShaderName Name, BMRShaderInput* ShaderInputs, u32 ShaderInputsCount)
	{
		for (u32 i = 0; i < ShaderInputsCount; ++i)
		{
			if (ShaderInputs[i].ShaderName == Name)
			{
				return ShaderInputs[i].Module;
			}
		}

		assert(false);
	}
}
