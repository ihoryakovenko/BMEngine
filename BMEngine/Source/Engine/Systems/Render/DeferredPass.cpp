#include "DeferredPass.h"

#include "Render/VulkanInterface/VulkanInterface.h"

#include "Util/Settings.h"
#include "Util/Util.h"

namespace DeferredPass
{
	static VulkanInterface::RenderPipeline Pipeline;

	static VkDescriptorSetLayout DeferredInputLayout;

	static VulkanInterface::UniformImage DeferredInputDepthImage[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VulkanInterface::UniformImage DeferredInputColorImage[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkImageView DeferredInputDepthImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];
	static VkImageView DeferredInputColorImageInterface[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkDescriptorSet DeferredInputSet[VulkanInterface::MAX_SWAPCHAIN_IMAGES_COUNT];

	static VkSampler ColorSampler;
	static VkSampler DepthSampler;

	void Init()
	{
		VkSamplerCreateInfo ColorSamplerCreateInfo = { };
		ColorSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		ColorSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		ColorSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		ColorSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		ColorSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		ColorSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		ColorSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		ColorSamplerCreateInfo.minLod = 0.0f;
		ColorSamplerCreateInfo.maxLod = 0.0f;
		ColorSamplerCreateInfo.mipLodBias = 0.0f;
		ColorSamplerCreateInfo.anisotropyEnable = VK_TRUE;
		ColorSamplerCreateInfo.maxAnisotropy = 16.0f;
		ColorSamplerCreateInfo.compareEnable = VK_FALSE;
		ColorSamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		ColorSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		VkSamplerCreateInfo DepthSamplerInfo = ColorSamplerCreateInfo;
		DepthSamplerInfo.magFilter = VK_FILTER_NEAREST;
		DepthSamplerInfo.minFilter = VK_FILTER_NEAREST;
		DepthSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		DepthSamplerInfo.anisotropyEnable = VK_FALSE;

		const VkDescriptorType DeferredInputDescriptorType[2] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
		const VkShaderStageFlags DeferredInputFlags[2] = { VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
		const VkDescriptorBindingFlags BindingFlags[2] = { };
		DeferredInputLayout = VulkanInterface::CreateUniformLayout(DeferredInputDescriptorType, DeferredInputFlags, BindingFlags, 2, 1);

		VkImageCreateInfo DeferredInputDepthUniformCreateInfo = { };
		DeferredInputDepthUniformCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		DeferredInputDepthUniformCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		DeferredInputDepthUniformCreateInfo.extent.depth = 1;
		DeferredInputDepthUniformCreateInfo.mipLevels = 1;
		DeferredInputDepthUniformCreateInfo.arrayLayers = 1;
		DeferredInputDepthUniformCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		DeferredInputDepthUniformCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DeferredInputDepthUniformCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		DeferredInputDepthUniformCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		DeferredInputDepthUniformCreateInfo.flags = 0;
		DeferredInputDepthUniformCreateInfo.extent.width = MainScreenExtent.width;
		DeferredInputDepthUniformCreateInfo.extent.height = MainScreenExtent.height;
		DeferredInputDepthUniformCreateInfo.format = DepthFormat;
		DeferredInputDepthUniformCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageCreateInfo DeferredInputColorUniformCreateInfo = DeferredInputDepthUniformCreateInfo;
		DeferredInputColorUniformCreateInfo.format = ColorFormat;
		DeferredInputColorUniformCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VulkanInterface::UniformImageInterfaceCreateInfo InputDepthUniformInterfaceCreateInfo = { };
		InputDepthUniformInterfaceCreateInfo = { };
		InputDepthUniformInterfaceCreateInfo.Flags = 0; // No flags
		InputDepthUniformInterfaceCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		InputDepthUniformInterfaceCreateInfo.Format = DepthFormat;
		InputDepthUniformInterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		InputDepthUniformInterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		InputDepthUniformInterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		InputDepthUniformInterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		InputDepthUniformInterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		InputDepthUniformInterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
		InputDepthUniformInterfaceCreateInfo.SubresourceRange.levelCount = 1;
		InputDepthUniformInterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
		InputDepthUniformInterfaceCreateInfo.SubresourceRange.layerCount = 1;

		VulkanInterface::UniformImageInterfaceCreateInfo InputUniformColorInterfaceCreateInfo = InputDepthUniformInterfaceCreateInfo;
		InputUniformColorInterfaceCreateInfo.Format = ColorFormat;
		InputUniformColorInterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		ColorSampler = VulkanInterface::CreateSampler(&ColorSamplerCreateInfo);
		DepthSampler = VulkanInterface::CreateSampler(&DepthSamplerInfo);

		for (u32 i = 0; i < VulkanInterface::GetImageCount(); i++)
		{
			//const VkDeviceSize AlignedVpSize = VulkanMemoryManagementSystem::CalculateBufferAlignedSize(VpBufferSize);

			DeferredInputDepthImage[i] = VulkanInterface::CreateUniformImage(&DeferredInputDepthUniformCreateInfo);
			DeferredInputColorImage[i] = VulkanInterface::CreateUniformImage(&DeferredInputColorUniformCreateInfo);

			DeferredInputDepthImageInterface[i] = VulkanInterface::CreateImageInterface(&InputDepthUniformInterfaceCreateInfo,
				DeferredInputDepthImage[i].Image);
			DeferredInputColorImageInterface[i] = VulkanInterface::CreateImageInterface(&InputUniformColorInterfaceCreateInfo,
				DeferredInputColorImage[i].Image);

			VulkanInterface::CreateUniformSets(&DeferredInputLayout, 1, DeferredInputSet + i);


			VulkanInterface::UniformSetAttachmentInfo DeferredInputAttachmentInfo[2];
			DeferredInputAttachmentInfo[0].ImageInfo.imageView = DeferredInputColorImageInterface[i];
			DeferredInputAttachmentInfo[0].ImageInfo.sampler = ColorSampler;
			DeferredInputAttachmentInfo[0].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[0].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			DeferredInputAttachmentInfo[1].ImageInfo.imageView = DeferredInputDepthImageInterface[i];
			DeferredInputAttachmentInfo[1].ImageInfo.sampler = DepthSampler;
			DeferredInputAttachmentInfo[1].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DeferredInputAttachmentInfo[1].Type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			VulkanInterface::AttachUniformsToSet(DeferredInputSet[i], DeferredInputAttachmentInfo, 2);
		}




		Pipeline.PipelineLayout = VulkanInterface::CreatePipelineLayout(&DeferredInputLayout, 1, nullptr, 0);

		std::vector<char> VertexShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/second_vert.spv", VertexShaderCode, "rb");
		std::vector<char> FragmentShaderCode;
		Util::OpenAndReadFileFull("./Resources/Shaders/second_frag.spv", FragmentShaderCode, "rb");

		const u32 ShaderCount = 2;
		VulkanInterface::Shader Shaders[ShaderCount];

		Shaders[0].Stage = VK_SHADER_STAGE_VERTEX_BIT;
		Shaders[0].Code = VertexShaderCode.data();
		Shaders[0].CodeSize = VertexShaderCode.size();

		Shaders[1].Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		Shaders[1].Code = FragmentShaderCode.data();
		Shaders[1].CodeSize = FragmentShaderCode.size();

		//VulkanInterface::PipelineSettings PipelineSettings;
		//Util::LoadPipelineSettings(PipelineSettings, "./Resources/Settings/StaticMeshRender.ini");
		//PipelineSettings.Extent = MainScreenExtent;

		VulkanInterface::PipelineResourceInfo ResourceInfo;
		ResourceInfo.PipelineLayout = Pipeline.PipelineLayout;
		ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 1;
		ResourceInfo.PipelineAttachmentData.ColorAttachmentFormats[0] = VulkanInterface::GetSurfaceFormat();
		ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
		ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		Pipeline.Pipeline = VulkanInterface::BatchPipelineCreation(Shaders, ShaderCount, nullptr, 0,
			&DeferredPipelineSettings, &ResourceInfo);
	}

	void DeInit()
	{
		VulkanInterface::DestroySampler(ColorSampler);
		VulkanInterface::DestroySampler(DepthSampler);
	}

	void Draw()
	{
		VkCommandBuffer CmdBuffer = VulkanInterface::GetCommandBuffer();

		VkRect2D RenderArea;
		RenderArea.extent = MainScreenExtent;
		RenderArea.offset = { 0, 0 };

		VkRenderingAttachmentInfo SwapchainColorAttachment = { };
		SwapchainColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		SwapchainColorAttachment.imageView = VulkanInterface::GetSwapchainImageViews()[VulkanInterface::TestGetImageIndex()];
		SwapchainColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		SwapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingInfo RenderingInfo{ };
		RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		RenderingInfo.renderArea = RenderArea;
		RenderingInfo.layerCount = 1;
		RenderingInfo.colorAttachmentCount = 1;
		RenderingInfo.pColorAttachments = &SwapchainColorAttachment;
		RenderingInfo.pDepthAttachment = nullptr;

		VkImageMemoryBarrier2 ColorBarrier = { };
		ColorBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		ColorBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ColorBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ColorBarrier.image = DeferredInputColorImage[VulkanInterface::TestGetImageIndex()].Image;
		ColorBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ColorBarrier.subresourceRange.baseMipLevel = 0;
		ColorBarrier.subresourceRange.levelCount = 1;
		ColorBarrier.subresourceRange.baseArrayLayer = 0;
		ColorBarrier.subresourceRange.layerCount = 1;
		// RELEASE: wait for all color-attachmet writes to finish
		ColorBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		ColorBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// ACQUIRE: make image ready for sampling in the fragment shader
		ColorBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		ColorBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkImageMemoryBarrier2 DepthBarrier = { };
		DepthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		DepthBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DepthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		DepthBarrier.image = DeferredInputDepthImage[VulkanInterface::TestGetImageIndex()].Image;
		DepthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		DepthBarrier.subresourceRange.baseMipLevel = 0;
		DepthBarrier.subresourceRange.levelCount = 1;
		DepthBarrier.subresourceRange.baseArrayLayer = 0;
		DepthBarrier.subresourceRange.layerCount = 1;
		// RELEASE: wait for all color-attachmet writes to finish
		DepthBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		DepthBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// ACQUIRE: make image ready for sampling in the fragment shader
		DepthBarrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		DepthBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkImageMemoryBarrier2 SwapchainAcquireBarrier = { };
		SwapchainAcquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainAcquireBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SwapchainAcquireBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainAcquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainAcquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainAcquireBarrier.image = VulkanInterface::GetSwapchainImages()[VulkanInterface::TestGetImageIndex()];
		SwapchainAcquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SwapchainAcquireBarrier.subresourceRange.baseMipLevel = 0;
		SwapchainAcquireBarrier.subresourceRange.levelCount = 1;
		SwapchainAcquireBarrier.subresourceRange.baseArrayLayer = 0;
		SwapchainAcquireBarrier.subresourceRange.layerCount = 1;
		// RELEASE nothing — we don’t depend on any earlier writes to the swap image
		SwapchainAcquireBarrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		SwapchainAcquireBarrier.srcAccessMask = 0;
		// ACQUIRE for our upcoming color-attachment writes
		SwapchainAcquireBarrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainAcquireBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkImageMemoryBarrier2 Barriers[] = {
			ColorBarrier,
			DepthBarrier,
			SwapchainAcquireBarrier
		};

		VkDependencyInfo DepInfo = { };
		DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DepInfo.imageMemoryBarrierCount = 3;
		DepInfo.pImageMemoryBarriers = Barriers;

		vkCmdPipelineBarrier2(CmdBuffer, &DepInfo);

		vkCmdBeginRendering(CmdBuffer, &RenderingInfo);

		vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.Pipeline);

		vkCmdBindDescriptorSets(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline.PipelineLayout,
			0, 1, &DeferredInputSet[VulkanInterface::TestGetImageIndex()], 0, nullptr);

		vkCmdDraw(CmdBuffer, 3, 1, 0, 0); // 3 hardcoded vertices

		vkCmdEndRendering(CmdBuffer);

		VkImageMemoryBarrier2 SwapchainPresentBarrier = { };
		SwapchainPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainPresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		SwapchainPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainPresentBarrier.image = VulkanInterface::GetSwapchainImages()[VulkanInterface::TestGetImageIndex()];
		SwapchainPresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SwapchainPresentBarrier.subresourceRange.baseMipLevel = 0;
		SwapchainPresentBarrier.subresourceRange.levelCount = 1;
		SwapchainPresentBarrier.subresourceRange.baseArrayLayer = 0;
		SwapchainPresentBarrier.subresourceRange.layerCount = 1;
		// RELEASE: finish all your color-attachment writes
		SwapchainPresentBarrier.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainPresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// ACQUIRE: nothing—present engine will take it
		SwapchainPresentBarrier.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		SwapchainPresentBarrier.dstAccessMask = VK_PIPELINE_STAGE_NONE;

		VkDependencyInfo PresentDepInfo = { };
		PresentDepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		PresentDepInfo.imageMemoryBarrierCount = 1;
		PresentDepInfo.pImageMemoryBarriers = &SwapchainPresentBarrier;

		vkCmdPipelineBarrier2(CmdBuffer, &PresentDepInfo);
	}





	VkImageView* TestDeferredInputColorImageInterface()
	{
		return DeferredInputColorImageInterface;
	}

	VkImageView* TestDeferredInputDepthImageInterface()
	{
		return DeferredInputDepthImageInterface;
	}

	VulkanInterface::UniformImage* TestDeferredInputColorImage()
	{
		return DeferredInputColorImage;
	}

	VulkanInterface::UniformImage* TestDeferredInputDepthImage()
	{
		return DeferredInputDepthImage;
	}
}
