#include "MainRenderPass.h"

#include "Util/Util.h"
#include "VulkanHelper.h"
#include "VulkanMemoryManagementSystem.h"

#include "Memory/MemoryManagmentSystem.h"

namespace BMR
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

	static const u32 TmpDepthSubpassIndex = 0;

	struct BMRSPipelineShaderInfo
	{
		VkPipelineShaderStageCreateInfo Infos[BMRShaderStages::ShaderStagesCount];
		u32 InfosCounter = 0;
	};

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
			VulkanMemoryManagementSystem::DestroyBuffer(DepthLightSpaceBuffers[i]);

			vkDestroyImageView(LogicalDevice, ShadowDepthBufferViews[i], nullptr);
			VulkanMemoryManagementSystem::DestroyImageBuffer(ShadowDepthBuffers[i]);
		}

		for (u32 i = 0; i < DescriptorLayoutHandles::Count; ++i)
		{
			vkDestroyDescriptorSetLayout(LogicalDevice, DescriptorLayouts[i], nullptr);
		}

		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			vkDestroyPipelineLayout(LogicalDevice, PipelineLayouts[i], nullptr);
			vkDestroyPipeline(LogicalDevice, Pipelines[i], nullptr);
		}

		VulkanMemoryManagementSystem::DestroyBuffer(MaterialBuffer);

		for (u32 i = 0; i < RenderPasses::Count; ++i)
		{
			vkDestroyRenderPass(LogicalDevice, RenderPasses[i], nullptr);

			for (u32 j = 0; j < ImagesCount; ++j)
			{
				vkDestroyFramebuffer(LogicalDevice, Framebuffers[i][j], nullptr);
			}
		}
	}

	void BMRMainRenderPass::CreateVulkanPass(VkDevice LogicalDevice, VkFormat ColorFormat, VkFormat DepthFormat, VkSurfaceFormatKHR SurfaceFormat)
	{
		VkSubpassDescription MainSubpasses[SubpassIndex::Count];

		const u32 SwapchainColorAttachmentIndex = 0;
		const u32 SubpassColorAttachmentIndex = 1;
		const u32 SubpassDepthAttachmentIndex = 2;

		const u32 MainPathAttachmentDescriptionsCount = 3;
		VkAttachmentDescription MainPathAttachmentDescriptions[MainPathAttachmentDescriptionsCount];

		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex] = { };
		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].format = SurfaceFormat.format;  // Use the appropriate surface format
		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;  // Typically single sample for swapchain
		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear the color buffer at the beginning
		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // Store the result so it can be presented
		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // No need for initial content
		MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Make it ready for presentation

		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex] = { };
		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex].format = ColorFormat;
		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		MainPathAttachmentDescriptions[SubpassColorAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex] = { };
		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex].format = DepthFormat;
		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		MainPathAttachmentDescriptions[SubpassDepthAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Define the Entity subpass, which will read the color and depth attachments
		VkAttachmentReference EntitySubpassColourAttachmentReference = { };
		EntitySubpassColourAttachmentReference.attachment = SubpassColorAttachmentIndex; // Index of the color attachment (shared with Terrain subpass)
		EntitySubpassColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference EntitySubpassDepthAttachmentReference = { };
		EntitySubpassDepthAttachmentReference.attachment = SubpassDepthAttachmentIndex; // Depth attachment (shared with Terrain subpass)
		EntitySubpassDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		MainSubpasses[SubpassIndex::MainSubpass] = { };
		MainSubpasses[SubpassIndex::MainSubpass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		MainSubpasses[SubpassIndex::MainSubpass].colorAttachmentCount = 1;
		MainSubpasses[SubpassIndex::MainSubpass].pColorAttachments = &EntitySubpassColourAttachmentReference;
		MainSubpasses[SubpassIndex::MainSubpass].pDepthStencilAttachment = &EntitySubpassDepthAttachmentReference;

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

		MainSubpasses[SubpassIndex::DeferredSubpas] = { };
		MainSubpasses[SubpassIndex::DeferredSubpas].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		MainSubpasses[SubpassIndex::DeferredSubpas].colorAttachmentCount = 1;
		MainSubpasses[SubpassIndex::DeferredSubpas].pColorAttachments = &SwapchainColorAttachmentReference;
		MainSubpasses[SubpassIndex::DeferredSubpas].inputAttachmentCount = InputReferencesCount;
		MainSubpasses[SubpassIndex::DeferredSubpas].pInputAttachments = InputReferences;

		// Subpass dependencies
		const u32 ExitDependenciesIndex = SubpassIndex::Count;
		const u32 MainPassSubpassDependenciesCount = SubpassIndex::Count + 1;
		VkSubpassDependency MainPassSubpassDependencies[MainPassSubpassDependenciesCount];

		// Transition from external to Terrain subpass
		// Transition must happen after...
		MainPassSubpassDependencies[SubpassIndex::MainSubpass].srcSubpass = VK_SUBPASS_EXTERNAL;
		MainPassSubpassDependencies[SubpassIndex::MainSubpass].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		MainPassSubpassDependencies[SubpassIndex::MainSubpass].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// But must happen before...
		MainPassSubpassDependencies[SubpassIndex::MainSubpass].dstSubpass = SubpassIndex::DeferredSubpas;
		MainPassSubpassDependencies[SubpassIndex::MainSubpass].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		MainPassSubpassDependencies[SubpassIndex::MainSubpass].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		MainPassSubpassDependencies[SubpassIndex::MainSubpass].dependencyFlags = 0;

		// Transition from Entity subpass to Deferred subpass
		// Transition must happen after...
		MainPassSubpassDependencies[SubpassIndex::DeferredSubpas].srcSubpass = SubpassIndex::MainSubpass;
		MainPassSubpassDependencies[SubpassIndex::DeferredSubpas].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		MainPassSubpassDependencies[SubpassIndex::DeferredSubpas].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// But must happen before...
		MainPassSubpassDependencies[SubpassIndex::DeferredSubpas].dstSubpass = SubpassIndex::DeferredSubpas;
		MainPassSubpassDependencies[SubpassIndex::DeferredSubpas].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		MainPassSubpassDependencies[SubpassIndex::DeferredSubpas].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		MainPassSubpassDependencies[SubpassIndex::DeferredSubpas].dependencyFlags = 0;

		// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		// Transition must happen after...
		MainPassSubpassDependencies[ExitDependenciesIndex].srcSubpass = SubpassIndex::DeferredSubpas;
		MainPassSubpassDependencies[ExitDependenciesIndex].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		MainPassSubpassDependencies[ExitDependenciesIndex].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// But must happen before...
		MainPassSubpassDependencies[ExitDependenciesIndex].dstSubpass = VK_SUBPASS_EXTERNAL;
		MainPassSubpassDependencies[ExitDependenciesIndex].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		MainPassSubpassDependencies[ExitDependenciesIndex].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		MainPassSubpassDependencies[ExitDependenciesIndex].dependencyFlags = 0;

		enum Subpasses
		{
			Depth,

			Count
		};

		VkSubpassDescription DepthSubpasses[Subpasses::Count];

		const u32 DepthSubpassAttachmentIndex = 0;

		const u32 DepthPassAttachmentDescriptionsCount = 1;
		VkAttachmentDescription DepthPassAttachmentDescriptions[DepthPassAttachmentDescriptionsCount];

		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex] = { };
		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].format = DepthFormat;
		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference DepthSubpassDepthAttachmentReference = { };
		DepthSubpassDepthAttachmentReference.attachment = DepthSubpassAttachmentIndex; // Depth attachment (shared with Terrain subpass)
		DepthSubpassDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		DepthSubpasses[Subpasses::Depth] = { };
		DepthSubpasses[Subpasses::Depth].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		DepthSubpasses[Subpasses::Depth].colorAttachmentCount = 0;
		DepthSubpasses[Subpasses::Depth].pColorAttachments = nullptr;
		DepthSubpasses[Subpasses::Depth].pDepthStencilAttachment = &DepthSubpassDepthAttachmentReference;

		// Subpass dependencies
		const u32 DepthPassExitDependenciesIndex = Subpasses::Count;
		const u32 DepthPassSubpassDependenciesCount = Subpasses::Count + 1;
		VkSubpassDependency DepthPassSubpassDependencies[DepthPassSubpassDependenciesCount];

		// Transition must happen after...
		DepthPassSubpassDependencies[Subpasses::Depth].srcSubpass = VK_SUBPASS_EXTERNAL;
		DepthPassSubpassDependencies[Subpasses::Depth].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		DepthPassSubpassDependencies[Subpasses::Depth].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// But must happen before...
		DepthPassSubpassDependencies[Subpasses::Depth].dstSubpass = Subpasses::Depth;
		DepthPassSubpassDependencies[Subpasses::Depth].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		DepthPassSubpassDependencies[Subpasses::Depth].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		DepthPassSubpassDependencies[Subpasses::Depth].dependencyFlags = 0;

		// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		// Transition must happen after...
		DepthPassSubpassDependencies[DepthPassExitDependenciesIndex].srcSubpass = Subpasses::Depth;
		DepthPassSubpassDependencies[DepthPassExitDependenciesIndex].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		DepthPassSubpassDependencies[DepthPassExitDependenciesIndex].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// But must happen before...
		DepthPassSubpassDependencies[DepthPassExitDependenciesIndex].dstSubpass = VK_SUBPASS_EXTERNAL;
		DepthPassSubpassDependencies[DepthPassExitDependenciesIndex].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		DepthPassSubpassDependencies[DepthPassExitDependenciesIndex].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		DepthPassSubpassDependencies[DepthPassExitDependenciesIndex].dependencyFlags = 0;

		u32 AttachmentCountTable[RenderPasses::Count];
		AttachmentCountTable[RenderPasses::Depth] = DepthPassAttachmentDescriptionsCount;
		AttachmentCountTable[RenderPasses::Main] = MainPathAttachmentDescriptionsCount;

		const VkAttachmentDescription* AttachmentsTable[RenderPasses::Count];
		AttachmentsTable[RenderPasses::Depth] = DepthPassAttachmentDescriptions;
		AttachmentsTable[RenderPasses::Main] = MainPathAttachmentDescriptions;

		u32 SubpassCountTable[RenderPasses::Count];
		SubpassCountTable[RenderPasses::Depth] = Subpasses::Count;
		SubpassCountTable[RenderPasses::Main] = SubpassIndex::Count;

		const VkSubpassDescription* SubPassesTable[RenderPasses::Count];
		SubPassesTable[RenderPasses::Depth] = DepthSubpasses;
		SubPassesTable[RenderPasses::Main] = MainSubpasses;

		u32 DependenciesCountTable[RenderPasses::Count];
		DependenciesCountTable[RenderPasses::Depth] = DepthPassSubpassDependenciesCount;
		DependenciesCountTable[RenderPasses::Main] = MainPassSubpassDependenciesCount;

		VkSubpassDependency* SubpassDependenciesTable[RenderPasses::Count];
		SubpassDependenciesTable[RenderPasses::Depth] = DepthPassSubpassDependencies;
		SubpassDependenciesTable[RenderPasses::Main] = MainPassSubpassDependencies;
			
		VkRenderPassCreateInfo RenderPassCreateInfo[RenderPasses::Count];
		for (u32 i = 0; i < RenderPasses::Count; ++i)
		{
			RenderPassCreateInfo[i] = { };
			RenderPassCreateInfo[i].sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			RenderPassCreateInfo[i].attachmentCount = AttachmentCountTable[i];
			RenderPassCreateInfo[i].pAttachments = AttachmentsTable[i];
			RenderPassCreateInfo[i].subpassCount = SubpassCountTable[i];
			RenderPassCreateInfo[i].pSubpasses = SubPassesTable[i];
			RenderPassCreateInfo[i].dependencyCount = DependenciesCountTable[i];
			RenderPassCreateInfo[i].pDependencies = SubpassDependenciesTable[i];

			VkResult Result = vkCreateRenderPass(LogicalDevice, RenderPassCreateInfo + i, nullptr, RenderPasses + i);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkCreateRenderPass result is {}", static_cast<int>(Result));
				assert(false);
			}
		}
	}

	void BMRMainRenderPass::CreateFrameBuffer(VkDevice LogicalDevice, VkExtent2D FrameBufferSizes, u32 ImagesCount,
		VkImageView SwapchainImageViews[MAX_SWAPCHAIN_IMAGES_COUNT])
	{
		VkRenderPass RenderPassTable[RenderPasses::Count];
		RenderPassTable[RenderPasses::Depth] = RenderPasses[RenderPasses::Depth];
		RenderPassTable[RenderPasses::Main] = RenderPasses[RenderPasses::Main];

		VkExtent2D ExtentTable[RenderPasses::Count];
		ExtentTable[RenderPasses::Depth] = DepthPassSwapExtent;
		ExtentTable[RenderPasses::Main] = FrameBufferSizes;

		VkFramebufferCreateInfo CreateInfo = { };
		CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

		for (u32 i = 0; i < RenderPasses::Count; ++i)
		{
			for (u32 j = 0; j < ImagesCount; j++)
			{
				const u32 MainPassAttachmentsCount = 3;
				VkImageView MainPassAttachments[MainPassAttachmentsCount] = {
					SwapchainImageViews[j],
					// Do not forget about position in array AttachmentDescriptions
					ColorBufferViews[j],
					DepthBufferViews[j]
				};

				const u32 DepthAttachmentsCount = 1;
				VkImageView DepthAttachments[DepthAttachmentsCount] = {
					ShadowDepthBufferViews[j]
				};

				u32 AttachmentCountTable[RenderPasses::Count];
				AttachmentCountTable[RenderPasses::Depth] = DepthAttachmentsCount;
				AttachmentCountTable[RenderPasses::Main] = MainPassAttachmentsCount;

				const VkImageView* AttachmentsTable[RenderPasses::Count];
				AttachmentsTable[RenderPasses::Depth] = DepthAttachments;
				AttachmentsTable[RenderPasses::Main] = MainPassAttachments;

				CreateInfo.renderPass = RenderPassTable[i];
				CreateInfo.attachmentCount = AttachmentCountTable[i];
				CreateInfo.pAttachments = AttachmentsTable[i]; // List of attachments (1:1 with Render Pass)
				CreateInfo.width = ExtentTable[i].width;
				CreateInfo.height = ExtentTable[i].height;
				CreateInfo.layers = 1;

				VkResult Result = vkCreateFramebuffer(LogicalDevice, &CreateInfo, nullptr, &(Framebuffers[i][j]));
				if (Result != VK_SUCCESS)
				{
					Util::Log().Error("vkCreateFramebuffer result is {}", static_cast<int>(Result));
					assert(false);
				}
			}
		}
	}

	void BMRMainRenderPass::SetupPushConstants()
	{
		PushConstants[PushConstantHandles::Model].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		PushConstants[PushConstantHandles::Model].offset = 0;
		// Todo: check constant and model size?
		PushConstants[PushConstantHandles::Model].size = sizeof(BMRModel);
	}

	void BMRMainRenderPass::CreateDescriptorLayouts(VkDevice LogicalDevice)
	{
		const u32 VpLayoutBindingBindingCount = 1;
		VkDescriptorSetLayoutBinding VpLayoutBinding = { };
		VpLayoutBinding.binding = 0;
		VpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VpLayoutBinding.descriptorCount = 1;
		VpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		VpLayoutBinding.pImmutableSamplers = nullptr; // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

		const u32 EntitySamplerLayoutBindingCount = 2;
		VkDescriptorSetLayoutBinding EntitySamplerLayoutBinding[EntitySamplerLayoutBindingCount];
		EntitySamplerLayoutBinding[0].binding = 0;
		EntitySamplerLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		EntitySamplerLayoutBinding[0].descriptorCount = 1;
		EntitySamplerLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		EntitySamplerLayoutBinding[0].pImmutableSamplers = nullptr;
		EntitySamplerLayoutBinding[1] = EntitySamplerLayoutBinding[0];
		EntitySamplerLayoutBinding[1].binding = 1;

		VkDescriptorSetLayoutBinding TerrainSamplerLayoutBinding = { };
		TerrainSamplerLayoutBinding.binding = 0;
		TerrainSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		TerrainSamplerLayoutBinding.descriptorCount = 1;
		TerrainSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		TerrainSamplerLayoutBinding.pImmutableSamplers = nullptr;

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

		const u32 MaterialBindingsCount = 1;
		VkDescriptorSetLayoutBinding MaterialLayoutBinding = { };
		MaterialLayoutBinding.binding = 0;
		MaterialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		MaterialLayoutBinding.descriptorCount = 1;
		MaterialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		MaterialLayoutBinding.pImmutableSamplers = nullptr;

		const u32 DeferredAttachmentBindingsCount = 2;
		VkDescriptorSetLayoutBinding DeferredInputBindings[DeferredAttachmentBindingsCount];
		DeferredInputBindings[0].binding = 0;
		DeferredInputBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		DeferredInputBindings[0].descriptorCount = 1;
		DeferredInputBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		DeferredInputBindings[1] = DeferredInputBindings[0];
		DeferredInputBindings[1].binding = 1;

		VkDescriptorSetLayoutBinding SkyBoxSamplerLayoutBinding = { };
		SkyBoxSamplerLayoutBinding.binding = 0;
		SkyBoxSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		SkyBoxSamplerLayoutBinding.descriptorCount = 1;
		SkyBoxSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		SkyBoxSamplerLayoutBinding.pImmutableSamplers = nullptr;

		const u32 DepthVpLayoutBindingBindingCount = 1;
		VkDescriptorSetLayoutBinding DepthVpLayoutBinding = { };
		DepthVpLayoutBinding.binding = 0;
		DepthVpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		DepthVpLayoutBinding.descriptorCount = 1;
		DepthVpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		DepthVpLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding ShadowMapAttachmentSamplerLayoutBinding = { };
		ShadowMapAttachmentSamplerLayoutBinding.binding = 0;
		ShadowMapAttachmentSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ShadowMapAttachmentSamplerLayoutBinding.descriptorCount = 1;
		ShadowMapAttachmentSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		ShadowMapAttachmentSamplerLayoutBinding.pImmutableSamplers = nullptr;

		u32 BindingCountTable[DescriptorLayoutHandles::Count];
		BindingCountTable[DescriptorLayoutHandles::TerrainVp] = VpLayoutBindingBindingCount;
		BindingCountTable[DescriptorLayoutHandles::TerrainSampler] = 1;
		BindingCountTable[DescriptorLayoutHandles::EntityVp] = VpLayoutBindingBindingCount;
		BindingCountTable[DescriptorLayoutHandles::EntitySampler] = EntitySamplerLayoutBindingCount;
		BindingCountTable[DescriptorLayoutHandles::Light] = LightBindingsCount;
		BindingCountTable[DescriptorLayoutHandles::Material] = MaterialBindingsCount;
		BindingCountTable[DescriptorLayoutHandles::DeferredInput] = DeferredAttachmentBindingsCount;
		BindingCountTable[DescriptorLayoutHandles::SkyBoxVp] = VpLayoutBindingBindingCount;
		BindingCountTable[DescriptorLayoutHandles::SkyBoxSampler] = 1;
		BindingCountTable[DescriptorLayoutHandles::DepthLightSpace] = DepthVpLayoutBindingBindingCount;
		BindingCountTable[DescriptorLayoutHandles::ShadowMapInput] = 1;

		const VkDescriptorSetLayoutBinding* BindingsTable[DescriptorLayoutHandles::Count];
		BindingsTable[DescriptorLayoutHandles::TerrainVp] = &VpLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::TerrainSampler] = &TerrainSamplerLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::EntityVp] = &VpLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::EntitySampler] = EntitySamplerLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::Light] = LightBindings;
		BindingsTable[DescriptorLayoutHandles::Material] = &MaterialLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::DeferredInput] = DeferredInputBindings;
		BindingsTable[DescriptorLayoutHandles::SkyBoxVp] = &VpLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::SkyBoxSampler] = &SkyBoxSamplerLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::DepthLightSpace] = &DepthVpLayoutBinding;
		BindingsTable[DescriptorLayoutHandles::ShadowMapInput] = &ShadowMapAttachmentSamplerLayoutBinding;

		VkDescriptorSetLayoutCreateInfo LayoutCreateInfos[DescriptorLayoutHandles::Count];
		for (u32 i = 0; i < DescriptorLayoutHandles::Count; ++i)
		{
			LayoutCreateInfos[i] = { };
			LayoutCreateInfos[i].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			LayoutCreateInfos[i].bindingCount = BindingCountTable[i];
			LayoutCreateInfos[i].pBindings = BindingsTable[i];

			const VkResult Result = vkCreateDescriptorSetLayout(LogicalDevice, &(LayoutCreateInfos[i]), nullptr, &(DescriptorLayouts[i]));
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkCreateDescriptorSetLayout result is {}", static_cast<int>(Result));
				assert(false);
			}
		}
	}

	void BMRMainRenderPass::CreatePipelineLayouts(VkDevice LogicalDevice)
	{
		const u32 TerrainDescriptorLayoutsCount = 2;
		VkDescriptorSetLayout TerrainDescriptorLayouts[TerrainDescriptorLayoutsCount] = {
			DescriptorLayouts[DescriptorLayoutHandles::TerrainVp],
			DescriptorLayouts[DescriptorLayoutHandles::TerrainSampler]
		};

		const u32 EntityDescriptorLayoutCount = 6;
		VkDescriptorSetLayout EntityDescriptorLayouts[EntityDescriptorLayoutCount] = {
			DescriptorLayouts[DescriptorLayoutHandles::EntityVp],
			DescriptorLayouts[DescriptorLayoutHandles::EntitySampler],
			DescriptorLayouts[DescriptorLayoutHandles::Light],
			DescriptorLayouts[DescriptorLayoutHandles::Material],
			DescriptorLayouts[DescriptorLayoutHandles::DepthLightSpace],
			DescriptorLayouts[DescriptorLayoutHandles::ShadowMapInput],
		};

		const u32 SkyBoxDescriptorLayoutCount = 2;
		VkDescriptorSetLayout SkyBoxDescriptorLayouts[SkyBoxDescriptorLayoutCount] = {
			DescriptorLayouts[DescriptorLayoutHandles::SkyBoxVp],
			DescriptorLayouts[DescriptorLayoutHandles::SkyBoxSampler],
		};

		u32 SetLayoutCountTable[BMRPipelineHandles::PipelineHandlesCount];
		SetLayoutCountTable[BMRPipelineHandles::Entity] = EntityDescriptorLayoutCount;
		SetLayoutCountTable[BMRPipelineHandles::Terrain] = TerrainDescriptorLayoutsCount;
		SetLayoutCountTable[BMRPipelineHandles::Deferred] = 1;
		SetLayoutCountTable[BMRPipelineHandles::SkyBox] = SkyBoxDescriptorLayoutCount;
		SetLayoutCountTable[BMRPipelineHandles::Depth] = 1;

		const VkDescriptorSetLayout* SetLayouts[BMRPipelineHandles::PipelineHandlesCount];
		SetLayouts[BMRPipelineHandles::Entity] = EntityDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Terrain] = TerrainDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Deferred] = DescriptorLayouts + DescriptorLayoutHandles::DeferredInput;
		SetLayouts[BMRPipelineHandles::SkyBox] = SkyBoxDescriptorLayouts;
		SetLayouts[BMRPipelineHandles::Depth] = DescriptorLayouts + DescriptorLayoutHandles::DepthLightSpace;

		u32 PushConstantRangeCountTable[BMRPipelineHandles::PipelineHandlesCount];
		PushConstantRangeCountTable[BMRPipelineHandles::Entity] = 1;
		PushConstantRangeCountTable[BMRPipelineHandles::Terrain] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::Deferred] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::SkyBox] = 0;
		PushConstantRangeCountTable[BMRPipelineHandles::Depth] = 1;

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo[BMRPipelineHandles::PipelineHandlesCount];
		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			PipelineLayoutCreateInfo[i] = { };
			PipelineLayoutCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			PipelineLayoutCreateInfo[i].setLayoutCount = SetLayoutCountTable[i];
			PipelineLayoutCreateInfo[i].pSetLayouts = SetLayouts[i];
			PipelineLayoutCreateInfo[i].pushConstantRangeCount = PushConstantRangeCountTable[i];
			PipelineLayoutCreateInfo[i].pPushConstantRanges = &PushConstants[PushConstantHandles::Model];

			const VkResult Result = vkCreatePipelineLayout(LogicalDevice, PipelineLayoutCreateInfo + i, nullptr,
				PipelineLayouts + i);
			if (Result != VK_SUCCESS)
			{
				Util::Log().Error("vkCreatePipelineLayout result is {}", static_cast<int>(Result));
				assert(false);
			}
		}
	}

	void BMRMainRenderPass::CreatePipelines(VkDevice LogicalDevice, VkExtent2D SwapExtent,
		BMRPipelineShaderInput ShaderInputs[BMRShaderNames::ShaderNamesCount])
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

		VkViewport DepthViewport = Viewport;
		DepthViewport.width = DepthPassSwapExtent.width;
		DepthViewport.height = DepthPassSwapExtent.height;

		const VkShaderStageFlagBits NamesToStagesTable[] =
		{
			VK_SHADER_STAGE_VERTEX_BIT,
			VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		BMRSPipelineShaderInfo ShaderInfos[BMRPipelineHandles::PipelineHandlesCount];
		for (u32 i = 0; i < BMRShaderNames::ShaderNamesCount; ++i)
		{
			const BMRPipelineShaderInput& Input = ShaderInputs[i];
			BMRSPipelineShaderInfo& Info = ShaderInfos[Input.Handle];

			Info.Infos[Info.InfosCounter] = { };
			Info.Infos[Info.InfosCounter].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			Info.Infos[Info.InfosCounter].stage = NamesToStagesTable[Input.Stage];
			Info.Infos[Info.InfosCounter].pName = Input.EntryPoint;
			CreateShader(LogicalDevice, Input.Code, Input.CodeSize, Info.Infos[Info.InfosCounter].module);
			++Info.InfosCounter;
		}

		VkVertexInputBindingDescription EntityInputBindingDescription = { };
		EntityInputBindingDescription.binding = 0; // Can bind multiple streams of data, this defines which one
		EntityInputBindingDescription.stride = sizeof(BMREntityVertex);
		EntityInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // How to move between data after each vertex.
		// VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex
		// VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance

		VkVertexInputBindingDescription TerrainVertexInputBindingDescription = { };
		TerrainVertexInputBindingDescription.binding = 0;
		TerrainVertexInputBindingDescription.stride = sizeof(BMRTerrainVertex);
		TerrainVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputBindingDescription SkyBoxVertexInputBindingDescription = { };
		SkyBoxVertexInputBindingDescription.binding = 0;
		SkyBoxVertexInputBindingDescription.stride = sizeof(BMRSkyBoxVertex);
		SkyBoxVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		const u32 TerrainVertexInputBindingDescriptionCount = 1;
		VkVertexInputAttributeDescription TerrainAttributeDescriptions[TerrainVertexInputBindingDescriptionCount];
		TerrainAttributeDescriptions[0].binding = 0;
		TerrainAttributeDescriptions[0].location = 0;
		TerrainAttributeDescriptions[0].format = VK_FORMAT_R32_SFLOAT;
		TerrainAttributeDescriptions[0].offset = offsetof(BMRTerrainVertex, Altitude);

		const u32 EntityVertexInputBindingDescriptionCount = 4;
		VkVertexInputAttributeDescription EntityAttributeDescriptions[EntityVertexInputBindingDescriptionCount];
		EntityAttributeDescriptions[0].binding = 0;	
		EntityAttributeDescriptions[0].location = 0;
		EntityAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		EntityAttributeDescriptions[0].offset = offsetof(BMREntityVertex, Position);
		EntityAttributeDescriptions[1].binding = 0;
		EntityAttributeDescriptions[1].location = 1;
		EntityAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		EntityAttributeDescriptions[1].offset = offsetof(BMREntityVertex, Color);
		EntityAttributeDescriptions[2].binding = 0;
		EntityAttributeDescriptions[2].location = 2;
		EntityAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		EntityAttributeDescriptions[2].offset = offsetof(BMREntityVertex, TextureCoords);
		EntityAttributeDescriptions[3].binding = 0;
		EntityAttributeDescriptions[3].location = 3;
		EntityAttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		EntityAttributeDescriptions[3].offset = offsetof(BMREntityVertex, Normal);

		const u32 SkyBoxVertexInputBindingDescriptionCount = 1;
		VkVertexInputAttributeDescription SkyBoxAttributeDescriptions[SkyBoxVertexInputBindingDescriptionCount];
		SkyBoxAttributeDescriptions[0].binding = 0;
		SkyBoxAttributeDescriptions[0].location = 0;
		SkyBoxAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		SkyBoxAttributeDescriptions[0].offset = offsetof(BMRSkyBoxVertex, Position);

		u32 ViewportCountTable[BMRPipelineHandles::PipelineHandlesCount];
		ViewportCountTable[BMRPipelineHandles::Entity] = 1;
		ViewportCountTable[BMRPipelineHandles::Terrain] = 1;
		ViewportCountTable[BMRPipelineHandles::Deferred] = 1;
		ViewportCountTable[BMRPipelineHandles::SkyBox] = 1;
		ViewportCountTable[BMRPipelineHandles::Depth] = 1;

		const VkViewport* ViewportTable[BMRPipelineHandles::PipelineHandlesCount];
		ViewportTable[BMRPipelineHandles::Entity] = &Viewport;
		ViewportTable[BMRPipelineHandles::Terrain] = &Viewport;
		ViewportTable[BMRPipelineHandles::Deferred] = &Viewport;
		ViewportTable[BMRPipelineHandles::SkyBox] = &Viewport;
		ViewportTable[BMRPipelineHandles::Depth] = &DepthViewport;

		const VkRect2D* ScissorTable[BMRPipelineHandles::PipelineHandlesCount];
		ScissorTable[BMRPipelineHandles::Entity] = &Scissor;
		ScissorTable[BMRPipelineHandles::Terrain] = &Scissor;
		ScissorTable[BMRPipelineHandles::Deferred] = &Scissor;
		ScissorTable[BMRPipelineHandles::SkyBox] = &Scissor;
		ScissorTable[BMRPipelineHandles::Depth] = &Scissor;

		u32 VertexBindingDescriptionCountTable[BMRPipelineHandles::PipelineHandlesCount];
		VertexBindingDescriptionCountTable[BMRPipelineHandles::Entity] = 1;
		VertexBindingDescriptionCountTable[BMRPipelineHandles::Terrain] = 1;
		VertexBindingDescriptionCountTable[BMRPipelineHandles::Deferred] = 0;
		VertexBindingDescriptionCountTable[BMRPipelineHandles::SkyBox] = 1;
		VertexBindingDescriptionCountTable[BMRPipelineHandles::Depth] = 1;

		const VkVertexInputBindingDescription* VertexBindingDescriptionsTable[BMRPipelineHandles::PipelineHandlesCount];
		VertexBindingDescriptionsTable[BMRPipelineHandles::Entity] = &EntityInputBindingDescription;
		VertexBindingDescriptionsTable[BMRPipelineHandles::Terrain] = &TerrainVertexInputBindingDescription;
		VertexBindingDescriptionsTable[BMRPipelineHandles::Deferred] = nullptr;
		VertexBindingDescriptionsTable[BMRPipelineHandles::SkyBox] = &SkyBoxVertexInputBindingDescription;
		VertexBindingDescriptionsTable[BMRPipelineHandles::Depth] = &EntityInputBindingDescription;

		u32 VertexAttributeDescriptionCountTable[BMRPipelineHandles::PipelineHandlesCount];
		VertexAttributeDescriptionCountTable[BMRPipelineHandles::Entity] = EntityVertexInputBindingDescriptionCount;
		VertexAttributeDescriptionCountTable[BMRPipelineHandles::Terrain] = TerrainVertexInputBindingDescriptionCount;
		VertexAttributeDescriptionCountTable[BMRPipelineHandles::Deferred] = 0;
		VertexAttributeDescriptionCountTable[BMRPipelineHandles::SkyBox] = SkyBoxVertexInputBindingDescriptionCount;
		VertexAttributeDescriptionCountTable[BMRPipelineHandles::Depth] = 1;

		const VkVertexInputAttributeDescription* VertexAttributeDescriptionsTable[BMRPipelineHandles::PipelineHandlesCount];
		VertexAttributeDescriptionsTable[BMRPipelineHandles::Entity] = EntityAttributeDescriptions;
		VertexAttributeDescriptionsTable[BMRPipelineHandles::Terrain] = TerrainAttributeDescriptions;
		VertexAttributeDescriptionsTable[BMRPipelineHandles::Deferred] = nullptr;
		VertexAttributeDescriptionsTable[BMRPipelineHandles::SkyBox] = SkyBoxAttributeDescriptions;
		VertexAttributeDescriptionsTable[BMRPipelineHandles::Depth] = EntityAttributeDescriptions;

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
		// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
		VkBool32 DepthClampEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		DepthClampEnableTable[BMRPipelineHandles::Entity] = VK_FALSE;
		DepthClampEnableTable[BMRPipelineHandles::Terrain] = VK_FALSE;
		DepthClampEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		DepthClampEnableTable[BMRPipelineHandles::SkyBox] = VK_FALSE;
		DepthClampEnableTable[BMRPipelineHandles::Depth] = VK_FALSE;

		// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		VkBool32 RasterizerDiscardEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		RasterizerDiscardEnableTable[BMRPipelineHandles::Entity] = VK_FALSE;
		RasterizerDiscardEnableTable[BMRPipelineHandles::Terrain] = VK_FALSE;
		RasterizerDiscardEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		RasterizerDiscardEnableTable[BMRPipelineHandles::SkyBox] = VK_FALSE;
		RasterizerDiscardEnableTable[BMRPipelineHandles::Depth] = VK_FALSE;

		VkPolygonMode PolygonModeTable[BMRPipelineHandles::PipelineHandlesCount];
		PolygonModeTable[BMRPipelineHandles::Entity] = VK_POLYGON_MODE_FILL;
		PolygonModeTable[BMRPipelineHandles::Terrain] = VK_POLYGON_MODE_FILL;
		PolygonModeTable[BMRPipelineHandles::Deferred] = VK_POLYGON_MODE_FILL;
		PolygonModeTable[BMRPipelineHandles::SkyBox] = VK_POLYGON_MODE_FILL;
		PolygonModeTable[BMRPipelineHandles::Depth] = VK_POLYGON_MODE_FILL;

		f32 LineWidthTable[BMRPipelineHandles::PipelineHandlesCount];
		LineWidthTable[BMRPipelineHandles::Entity] = 1.0f;
		LineWidthTable[BMRPipelineHandles::Terrain] = 1.0f;
		LineWidthTable[BMRPipelineHandles::Deferred] = 1.0f;
		LineWidthTable[BMRPipelineHandles::SkyBox] = 1.0f;
		LineWidthTable[BMRPipelineHandles::Depth] = 1.0f;

		VkCullModeFlags CullModeTable[BMRPipelineHandles::PipelineHandlesCount];
		CullModeTable[BMRPipelineHandles::Entity] = VK_CULL_MODE_BACK_BIT;
		CullModeTable[BMRPipelineHandles::Terrain] = VK_CULL_MODE_BACK_BIT;
		CullModeTable[BMRPipelineHandles::Deferred] = VK_CULL_MODE_NONE;
		CullModeTable[BMRPipelineHandles::SkyBox] = VK_CULL_MODE_FRONT_BIT;
		CullModeTable[BMRPipelineHandles::Depth] = VK_CULL_MODE_BACK_BIT;

		VkFrontFace FrontFaceTable[BMRPipelineHandles::PipelineHandlesCount];
		FrontFaceTable[BMRPipelineHandles::Entity] = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		FrontFaceTable[BMRPipelineHandles::Terrain] = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		FrontFaceTable[BMRPipelineHandles::Deferred] = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		FrontFaceTable[BMRPipelineHandles::SkyBox] = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		FrontFaceTable[BMRPipelineHandles::Depth] = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
		VkBool32 DepthBiasEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		DepthBiasEnableTable[BMRPipelineHandles::Entity] = VK_FALSE;
		DepthBiasEnableTable[BMRPipelineHandles::Terrain] = VK_FALSE;
		DepthBiasEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		DepthBiasEnableTable[BMRPipelineHandles::SkyBox] = VK_FALSE;
		DepthBiasEnableTable[BMRPipelineHandles::Depth] = VK_FALSE;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = { };
		MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
		MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

		VkBool32 BlendEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		BlendEnableTable[BMRPipelineHandles::Entity] = VK_TRUE;
		BlendEnableTable[BMRPipelineHandles::Terrain] = VK_TRUE;
		BlendEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		BlendEnableTable[BMRPipelineHandles::SkyBox] = VK_TRUE;
		BlendEnableTable[BMRPipelineHandles::Depth] = VK_TRUE;

		// Colors to apply blending to
		VkColorComponentFlags ColorWriteMaskTable[BMRPipelineHandles::PipelineHandlesCount];
		ColorWriteMaskTable[BMRPipelineHandles::Entity] = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorWriteMaskTable[BMRPipelineHandles::Terrain] = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorWriteMaskTable[BMRPipelineHandles::Deferred] = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorWriteMaskTable[BMRPipelineHandles::SkyBox] = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorWriteMaskTable[BMRPipelineHandles::Depth] = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)

		VkBlendFactor SrcColorBlendFactorTable[BMRPipelineHandles::PipelineHandlesCount];
		SrcColorBlendFactorTable[BMRPipelineHandles::Entity] = VK_BLEND_FACTOR_SRC_ALPHA;
		SrcColorBlendFactorTable[BMRPipelineHandles::Terrain] = VK_BLEND_FACTOR_SRC_ALPHA;
		SrcColorBlendFactorTable[BMRPipelineHandles::Deferred] = VK_BLEND_FACTOR_SRC_ALPHA;
		SrcColorBlendFactorTable[BMRPipelineHandles::SkyBox] = VK_BLEND_FACTOR_SRC_ALPHA;
		SrcColorBlendFactorTable[BMRPipelineHandles::Depth] = VK_BLEND_FACTOR_SRC_ALPHA;

		VkBlendFactor DstColorBlendFactorTable[BMRPipelineHandles::PipelineHandlesCount];
		DstColorBlendFactorTable[BMRPipelineHandles::Entity] = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		DstColorBlendFactorTable[BMRPipelineHandles::Terrain] = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		DstColorBlendFactorTable[BMRPipelineHandles::Deferred] = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		DstColorBlendFactorTable[BMRPipelineHandles::SkyBox] = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		DstColorBlendFactorTable[BMRPipelineHandles::Depth] = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		VkBlendOp ColorBlendOpTable[BMRPipelineHandles::PipelineHandlesCount];
		ColorBlendOpTable[BMRPipelineHandles::Entity] = VK_BLEND_OP_ADD;
		ColorBlendOpTable[BMRPipelineHandles::Terrain] = VK_BLEND_OP_ADD;
		ColorBlendOpTable[BMRPipelineHandles::Deferred] = VK_BLEND_OP_ADD;
		ColorBlendOpTable[BMRPipelineHandles::SkyBox] = VK_BLEND_OP_ADD;
		ColorBlendOpTable[BMRPipelineHandles::Depth] = VK_BLEND_OP_ADD;

		VkBlendFactor SrcAlphaBlendFactorTable[BMRPipelineHandles::PipelineHandlesCount];
		SrcAlphaBlendFactorTable[BMRPipelineHandles::Entity] = VK_BLEND_FACTOR_ONE;
		SrcAlphaBlendFactorTable[BMRPipelineHandles::Terrain] = VK_BLEND_FACTOR_ONE;
		SrcAlphaBlendFactorTable[BMRPipelineHandles::Deferred] = VK_BLEND_FACTOR_ONE;
		SrcAlphaBlendFactorTable[BMRPipelineHandles::SkyBox] = VK_BLEND_FACTOR_ONE;
		SrcAlphaBlendFactorTable[BMRPipelineHandles::Depth] = VK_BLEND_FACTOR_ONE;

		// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
//			   (new color alpha * new color) + ((1 - new color alpha) * old color)

		VkBlendFactor DstAlphaBlendFactorTable[BMRPipelineHandles::PipelineHandlesCount];
		DstAlphaBlendFactorTable[BMRPipelineHandles::Entity] = VK_BLEND_FACTOR_ZERO;
		DstAlphaBlendFactorTable[BMRPipelineHandles::Terrain] = VK_BLEND_FACTOR_ZERO;
		DstAlphaBlendFactorTable[BMRPipelineHandles::Deferred] = VK_BLEND_FACTOR_ZERO;
		DstAlphaBlendFactorTable[BMRPipelineHandles::SkyBox] = VK_BLEND_FACTOR_ZERO;
		DstAlphaBlendFactorTable[BMRPipelineHandles::Depth] = VK_BLEND_FACTOR_ZERO;

		VkBlendOp AlphaBlendOpTable[BMRPipelineHandles::PipelineHandlesCount];
		AlphaBlendOpTable[BMRPipelineHandles::Entity] = VK_BLEND_OP_ADD;
		AlphaBlendOpTable[BMRPipelineHandles::Terrain] = VK_BLEND_OP_ADD;
		AlphaBlendOpTable[BMRPipelineHandles::Deferred] = VK_BLEND_OP_ADD;
		AlphaBlendOpTable[BMRPipelineHandles::SkyBox] = VK_BLEND_OP_ADD;
		AlphaBlendOpTable[BMRPipelineHandles::Depth] = VK_BLEND_OP_ADD;
		// Summarised: (1 * new alpha) + (0 * old alpha) = new alpharesult != VK_SUCCESS

		VkBool32 LogicOpEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		LogicOpEnableTable[BMRPipelineHandles::Entity] = VK_FALSE;
		LogicOpEnableTable[BMRPipelineHandles::Terrain] = VK_FALSE;
		LogicOpEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		LogicOpEnableTable[BMRPipelineHandles::SkyBox] = VK_FALSE;
		LogicOpEnableTable[BMRPipelineHandles::Depth] = VK_FALSE;

		u32 AttachmentCountTable[BMRPipelineHandles::PipelineHandlesCount];
		AttachmentCountTable[BMRPipelineHandles::Entity] = 1;
		AttachmentCountTable[BMRPipelineHandles::Terrain] = 1;
		AttachmentCountTable[BMRPipelineHandles::Deferred] = 1;
		AttachmentCountTable[BMRPipelineHandles::SkyBox] = 1;
		AttachmentCountTable[BMRPipelineHandles::Depth] = 1;

		VkRenderPass RenderPassToPipelineTable[BMRPipelineHandles::PipelineHandlesCount];
		RenderPassToPipelineTable[BMRPipelineHandles::Entity] = RenderPasses[RenderPasses::Main];
		RenderPassToPipelineTable[BMRPipelineHandles::Terrain] = RenderPasses[RenderPasses::Main];
		RenderPassToPipelineTable[BMRPipelineHandles::Deferred] = RenderPasses[RenderPasses::Main];
		RenderPassToPipelineTable[BMRPipelineHandles::SkyBox] = RenderPasses[RenderPasses::Main];
		RenderPassToPipelineTable[BMRPipelineHandles::Depth] = RenderPasses[RenderPasses::Depth];

		u32 SubpassesToPipelineTable[BMRPipelineHandles::PipelineHandlesCount];
		SubpassesToPipelineTable[BMRPipelineHandles::Entity] = SubpassIndex::MainSubpass;
		SubpassesToPipelineTable[BMRPipelineHandles::Terrain] = SubpassIndex::MainSubpass;
		SubpassesToPipelineTable[BMRPipelineHandles::Deferred] = SubpassIndex::DeferredSubpas;
		SubpassesToPipelineTable[BMRPipelineHandles::SkyBox] = SubpassIndex::MainSubpass;
		SubpassesToPipelineTable[BMRPipelineHandles::Depth] = TmpDepthSubpassIndex;

		VkBool32 DepthTestEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		DepthTestEnableTable[BMRPipelineHandles::Entity] = VK_TRUE;
		DepthTestEnableTable[BMRPipelineHandles::Terrain] = VK_TRUE;
		DepthTestEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		DepthTestEnableTable[BMRPipelineHandles::SkyBox] = VK_TRUE;
		DepthTestEnableTable[BMRPipelineHandles::Depth] = VK_TRUE;

		VkBool32 DepthWriteEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		DepthWriteEnableTable[BMRPipelineHandles::Entity] = VK_TRUE;
		DepthWriteEnableTable[BMRPipelineHandles::Terrain] = VK_TRUE;
		DepthWriteEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		DepthWriteEnableTable[BMRPipelineHandles::SkyBox] = VK_FALSE;
		DepthWriteEnableTable[BMRPipelineHandles::Depth] = VK_TRUE;

		VkCompareOp DepthCompareOpTable[BMRPipelineHandles::PipelineHandlesCount];
		DepthCompareOpTable[BMRPipelineHandles::Entity] = VK_COMPARE_OP_LESS;
		DepthCompareOpTable[BMRPipelineHandles::Terrain] = VK_COMPARE_OP_LESS;
		DepthCompareOpTable[BMRPipelineHandles::SkyBox] = VK_COMPARE_OP_LESS_OR_EQUAL;
		DepthCompareOpTable[BMRPipelineHandles::Depth] = VK_COMPARE_OP_LESS;

		VkBool32 DepthBoundsTestEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		DepthBoundsTestEnableTable[BMRPipelineHandles::Entity] = VK_FALSE;
		DepthBoundsTestEnableTable[BMRPipelineHandles::Terrain] = VK_FALSE;
		DepthBoundsTestEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		DepthBoundsTestEnableTable[BMRPipelineHandles::SkyBox] = VK_FALSE;
		DepthBoundsTestEnableTable[BMRPipelineHandles::Depth] = VK_FALSE;

		VkBool32 StencilTestEnableTable[BMRPipelineHandles::PipelineHandlesCount];
		StencilTestEnableTable[BMRPipelineHandles::Entity] = VK_FALSE;
		StencilTestEnableTable[BMRPipelineHandles::Terrain] = VK_FALSE;
		StencilTestEnableTable[BMRPipelineHandles::Deferred] = VK_FALSE;
		StencilTestEnableTable[BMRPipelineHandles::SkyBox] = VK_FALSE;
		StencilTestEnableTable[BMRPipelineHandles::Depth] = VK_FALSE;

		VkPipelineViewportStateCreateInfo ViewportStateCreateInfo[BMRPipelineHandles::PipelineHandlesCount];
		VkPipelineVertexInputStateCreateInfo VertexInputInfo[BMRPipelineHandles::PipelineHandlesCount];
		VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo[BMRPipelineHandles::PipelineHandlesCount];
		const VkPipelineColorBlendAttachmentState* AttachmentsTable[BMRPipelineHandles::PipelineHandlesCount];
		VkPipelineColorBlendAttachmentState ColorBlendAttachmentState[BMRPipelineHandles::PipelineHandlesCount];
		VkPipelineColorBlendStateCreateInfo ColorBlendInfo[BMRPipelineHandles::PipelineHandlesCount];
		VkPipelineDepthStencilStateCreateInfo DepthStencilInfo[BMRPipelineHandles::PipelineHandlesCount];
		VkGraphicsPipelineCreateInfo PipelineCreateInfos[BMRPipelineHandles::PipelineHandlesCount];

		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			ViewportStateCreateInfo[i] = { };
			ViewportStateCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			ViewportStateCreateInfo[i].viewportCount = 1;
			ViewportStateCreateInfo[i].pViewports = &Viewport;
			ViewportStateCreateInfo[i].scissorCount = 1;
			ViewportStateCreateInfo[i].pScissors = &Scissor;

			RasterizationStateCreateInfo[i] = { };
			RasterizationStateCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			RasterizationStateCreateInfo[i].depthClampEnable = DepthClampEnableTable[i];
			RasterizationStateCreateInfo[i].rasterizerDiscardEnable = RasterizerDiscardEnableTable[i];
			RasterizationStateCreateInfo[i].polygonMode = PolygonModeTable[i];
			RasterizationStateCreateInfo[i].lineWidth = LineWidthTable[i];
			RasterizationStateCreateInfo[i].cullMode = CullModeTable[i];
			RasterizationStateCreateInfo[i].frontFace = FrontFaceTable[i];
			RasterizationStateCreateInfo[i].depthBiasEnable = DepthBiasEnableTable[i];

			VertexInputInfo[i] = { };
			VertexInputInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			VertexInputInfo[i].vertexBindingDescriptionCount = VertexBindingDescriptionCountTable[i];
			VertexInputInfo[i].pVertexBindingDescriptions = VertexBindingDescriptionsTable[i];
			VertexInputInfo[i].vertexAttributeDescriptionCount = VertexAttributeDescriptionCountTable[i];
			VertexInputInfo[i].pVertexAttributeDescriptions = VertexAttributeDescriptionsTable[i];

			ColorBlendAttachmentState[i].colorWriteMask = ColorWriteMaskTable[i];
			ColorBlendAttachmentState[i].blendEnable = BlendEnableTable[i];													// Enable blending
			ColorBlendAttachmentState[i].srcColorBlendFactor = SrcColorBlendFactorTable[i];
			ColorBlendAttachmentState[i].dstColorBlendFactor = DstColorBlendFactorTable[i];
			ColorBlendAttachmentState[i].colorBlendOp = ColorBlendOpTable[i];
			ColorBlendAttachmentState[i].srcAlphaBlendFactor = SrcAlphaBlendFactorTable[i];
			ColorBlendAttachmentState[i].dstAlphaBlendFactor = DstAlphaBlendFactorTable[i];
			ColorBlendAttachmentState[i].alphaBlendOp = AlphaBlendOpTable[i];

			AttachmentsTable[i] = ColorBlendAttachmentState + i;

			ColorBlendInfo[i] = { };
			ColorBlendInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			ColorBlendInfo[i].logicOpEnable = LogicOpEnableTable[i]; // Alternative to calculations is to use logical operations
			ColorBlendInfo[i].attachmentCount = AttachmentCountTable[i];
			ColorBlendInfo[i].pAttachments = AttachmentsTable[i];

			DepthStencilInfo[i] = { };
			DepthStencilInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			DepthStencilInfo[i].depthTestEnable = DepthTestEnableTable[i];
			DepthStencilInfo[i].depthWriteEnable = DepthWriteEnableTable[i];
			DepthStencilInfo[i].depthCompareOp = DepthCompareOpTable[i];
			DepthStencilInfo[i].depthBoundsTestEnable = DepthBoundsTestEnableTable[i];
			DepthStencilInfo[i].stencilTestEnable = StencilTestEnableTable[i];

			PipelineCreateInfos[i] = { };
			PipelineCreateInfos[i].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			PipelineCreateInfos[i].stageCount = ShaderInfos[i].InfosCounter;
			PipelineCreateInfos[i].pStages = ShaderInfos[i].Infos;
			PipelineCreateInfos[i].pVertexInputState = VertexInputInfo + i;
			PipelineCreateInfos[i].pInputAssemblyState = &InputAssemblyStateCreateInfo;
			PipelineCreateInfos[i].pViewportState = ViewportStateCreateInfo;
			PipelineCreateInfos[i].pDynamicState = nullptr;
			PipelineCreateInfos[i].pRasterizationState = RasterizationStateCreateInfo + i;
			PipelineCreateInfos[i].pMultisampleState = &MultisampleStateCreateInfo;
			PipelineCreateInfos[i].pColorBlendState = ColorBlendInfo + i;
			PipelineCreateInfos[i].pDepthStencilState = DepthStencilInfo + i;
			PipelineCreateInfos[i].layout = PipelineLayouts[i];
			PipelineCreateInfos[i].renderPass = RenderPassToPipelineTable[i];
			PipelineCreateInfos[i].subpass = SubpassesToPipelineTable[i];

			// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
			PipelineCreateInfos[i].basePipelineHandle = VK_NULL_HANDLE;
			PipelineCreateInfos[i].basePipelineIndex = -1;
		}

		const VkResult Result = vkCreateGraphicsPipelines(LogicalDevice, VK_NULL_HANDLE, BMRPipelineHandles::PipelineHandlesCount,
			PipelineCreateInfos, nullptr, Pipelines);

		for (u32 i = 0; i < BMRPipelineHandles::PipelineHandlesCount; ++i)
		{
			for (u32 j = 0; j < ShaderInfos[i].InfosCounter; ++j)
			{
				vkDestroyShaderModule(LogicalDevice, ShaderInfos[i].Infos[j].module, nullptr);
			}
		}

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
				DepthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&DepthBuffers[i].Memory);

			DepthBufferViews[i] = CreateImageView(LogicalDevice, DepthBuffers[i].Image,
				DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

			// VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT says that image can be used only as attachment. Used only for sub pass
			ColorBuffers[i].Image = CreateImage(PhysicalDevice, LogicalDevice, SwapExtent.width, SwapExtent.height,
				ColorFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&ColorBuffers[i].Memory);

			ColorBufferViews[i] = CreateImageView(LogicalDevice, ColorBuffers[i].Image,
				ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);



			ShadowDepthBuffers[i].Image = CreateImage(PhysicalDevice, LogicalDevice, DepthPassSwapExtent.width, DepthPassSwapExtent.height,
				DepthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&ShadowDepthBuffers[i].Memory);

			ShadowDepthBufferViews[i] = CreateImageView(LogicalDevice, ShadowDepthBuffers[i].Image,
				DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);
		}
	}

	void BMRMainRenderPass::CreateUniformBuffers(VkPhysicalDevice PhysicalDevice, VkDevice LogicalDevice,
		u32 ImagesCount)
	{
		const VkDeviceSize VpBufferSize = sizeof(BMRUboViewProjection);
		const VkDeviceSize LightBufferSize = sizeof(BMRLightBuffer);
		const VkDeviceSize MaterialBufferSize = sizeof(BMRMaterial);
		const VkDeviceSize LightSpaceBufferSize = sizeof(BMRLightSpaceMatrix);

		//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);
		//const VkDeviceSize AlignedAmbientLightSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(AmbientLightBufferSize);
		//const VkDeviceSize AlignedPointLightSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(PointLightBufferSize);

		for (u32 i = 0; i < ImagesCount; i++)
		{
			VpUniformBuffers[i] = VulkanMemoryManagementSystem::CreateBuffer(VpBufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			LightBuffers[i] = VulkanMemoryManagementSystem::CreateBuffer(LightBufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


			DepthLightSpaceBuffers[i] = VulkanMemoryManagementSystem::CreateBuffer(LightSpaceBufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}

		MaterialBuffer = VulkanMemoryManagementSystem::CreateBuffer(MaterialBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	void BMRMainRenderPass::CreateSets(VkDescriptorPool Pool, VkDevice LogicalDevice, u32 ImagesCount,
		VkSampler ShadowMapSampler)
	{
		VkDescriptorSetLayout Layouts[DescriptorLayoutHandles::Count][MAX_SWAPCHAIN_IMAGES_COUNT];

		VkDescriptorSetLayout SetLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout TerrainLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout LightingSetLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout DeferredSetLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout SkyBoxVpLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout DepthLightSpaceLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];
		VkDescriptorSetLayout ShadowMapLayouts[MAX_SWAPCHAIN_IMAGES_COUNT];

		for (u32 i = 0; i < ImagesCount; i++)
		{
			SetLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::EntityVp];
			TerrainLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::TerrainVp];
			LightingSetLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::Light];
			DeferredSetLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::DeferredInput];
			SkyBoxVpLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::SkyBoxVp];
			DepthLightSpaceLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::DepthLightSpace];
			ShadowMapLayouts[i] = DescriptorLayouts[DescriptorLayoutHandles::ShadowMapInput];
		}

		VulkanMemoryManagementSystem::AllocateSets(Pool, SetLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::EntityVp]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, TerrainLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::TerrainVp]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, LightingSetLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::EntityLigh]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, DeferredSetLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::DeferredInput]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, SkyBoxVpLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::SkyBoxVp]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, DepthLightSpaceLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::DepthLightSpace]);
		VulkanMemoryManagementSystem::AllocateSets(Pool, ShadowMapLayouts, ImagesCount, DescriptorsToImages[DescriptorHandles::ShadowMap]);

		for (u32 i = 0; i < ImagesCount; i++)
		{
			VkDescriptorBufferInfo DepthLightSpaceBufferInfo = { };
			DepthLightSpaceBufferInfo.buffer = DepthLightSpaceBuffers[i].Buffer;
			DepthLightSpaceBufferInfo.offset = 0;
			DepthLightSpaceBufferInfo.range = sizeof(BMRLightSpaceMatrix);

			VkDescriptorBufferInfo VpBufferInfo = { };
			VpBufferInfo.buffer = VpUniformBuffers[i].Buffer;
			VpBufferInfo.offset = 0;
			VpBufferInfo.range = sizeof(BMRUboViewProjection);

			VkDescriptorBufferInfo LightBufferInfo = { };
			LightBufferInfo.buffer = LightBuffers[i].Buffer;
			LightBufferInfo.offset = 0;
			LightBufferInfo.range = sizeof(BMRLightBuffer);

			VkDescriptorImageInfo ColourAttachmentDescriptor = { };
			ColourAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ColourAttachmentDescriptor.imageView = ColorBufferViews[i];
			ColourAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkDescriptorImageInfo DepthAttachmentDescriptor = { };
			DepthAttachmentDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DepthAttachmentDescriptor.imageView = DepthBufferViews[i];
			DepthAttachmentDescriptor.sampler = VK_NULL_HANDLE;

			VkDescriptorImageInfo ShadowMapTextureDescriptor = { };
			ShadowMapTextureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ShadowMapTextureDescriptor.imageView = ShadowDepthBufferViews[i];
			ShadowMapTextureDescriptor.sampler = ShadowMapSampler;

			VkWriteDescriptorSet ShadowDepthWrite = { };
			ShadowDepthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ShadowDepthWrite.dstSet = DescriptorsToImages[DescriptorHandles::ShadowMap][i];
			ShadowDepthWrite.dstBinding = 0;
			ShadowDepthWrite.dstArrayElement = 0;
			ShadowDepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			ShadowDepthWrite.descriptorCount = 1;
			ShadowDepthWrite.pImageInfo = &ShadowMapTextureDescriptor;

			VkWriteDescriptorSet DepthLightSpaceWrite = { };
			DepthLightSpaceWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DepthLightSpaceWrite.dstSet = DescriptorsToImages[DescriptorHandles::DepthLightSpace][i];
			DepthLightSpaceWrite.dstBinding = 0;
			DepthLightSpaceWrite.dstArrayElement = 0;
			DepthLightSpaceWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			DepthLightSpaceWrite.descriptorCount = 1;
			DepthLightSpaceWrite.pBufferInfo = &DepthLightSpaceBufferInfo;

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

			VkWriteDescriptorSet VpSkyBoxWrite = VpSetWrite;
			VpSkyBoxWrite.dstSet = DescriptorsToImages[DescriptorHandles::SkyBoxVp][i];

			VkWriteDescriptorSet ColourWrite = { };
			ColourWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ColourWrite.dstSet = DescriptorsToImages[DescriptorHandles::DeferredInput][i];
			ColourWrite.dstBinding = 0;
			ColourWrite.dstArrayElement = 0;
			ColourWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			ColourWrite.descriptorCount = 1;
			ColourWrite.pImageInfo = &ColourAttachmentDescriptor;

			VkWriteDescriptorSet DepthWrite = { };
			DepthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			DepthWrite.dstSet = DescriptorsToImages[DescriptorHandles::DeferredInput][i];
			DepthWrite.dstBinding = 1;
			DepthWrite.dstArrayElement = 0;
			DepthWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			DepthWrite.descriptorCount = 1;
			DepthWrite.pImageInfo = &DepthAttachmentDescriptor;

			const u32 WriteSetsCount = 8;
			VkWriteDescriptorSet WriteSets[WriteSetsCount] = { VpSetWrite, VpTerrainWrite, LightSetWrite,
				ColourWrite, DepthWrite, VpSkyBoxWrite, DepthLightSpaceWrite, ShadowDepthWrite };

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
}
