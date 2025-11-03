#include <Util/EngineTypes.h>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

#include <Engine/Systems/Memory/MemoryManagmentSystem.h>
#include <Engine/Systems/Render/RenderInterface.h>
#include <Engine/Systems/Render/RenderTypes.h>
#include <Engine/Systems/Render/Handles.h>
#include <Engine/Systems/Render/VulkanCoreContext.h>


int main()
{
	s32 WindowWidth = 1920;
	s32 WindowHeight = 1080;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* Window = glfwCreateWindow(WindowWidth, WindowHeight, "BMEngine", nullptr, nullptr);

	BmRender_Init(Window, 1);

	char* VertexShaderCode = nullptr;
	size_t VertexShaderCodeSize = 0;
	{
		FILE* file = fopen("./test_vertex.vert.spv", "rb");
		if (!file)
		{
			return -1;
		}

		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		if (fileSize < 0)
		{
			fclose(file);
			return -1;
		}

		VertexShaderCodeSize = (size_t)fileSize;
		VertexShaderCode = (char*)malloc(VertexShaderCodeSize);
		if (!VertexShaderCode)
		{
			fclose(file);
			return -1;
		}

		fseek(file, 0, SEEK_SET);
		size_t readSize = fread(VertexShaderCode, 1, VertexShaderCodeSize, file);
		fclose(file);

		if (readSize != VertexShaderCodeSize)
		{
			free(VertexShaderCode);
			return -1;
		}
	}

	BmRender_ShaderDescription ShaderDesc = {};
	ShaderDesc.Code = reinterpret_cast<const u32*>(VertexShaderCode);
	ShaderDesc.CodeSize = VertexShaderCodeSize;
	ShaderDesc.Stage = BmRender_PipelineShaderStage::Vertex;
	BmRender_Shader VertexShader = BmRender_CreateShader(&ShaderDesc);

	free(VertexShaderCode);

	// Load fragment shader
	char* FragmentShaderCode = nullptr;
	size_t FragmentShaderCodeSize = 0;
	{
		FILE* file = fopen("./test_fragment.frag.spv", "rb");
		if (!file)
		{
			return -1;
		}

		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		if (fileSize < 0)
		{
			fclose(file);
			return -1;
		}

		FragmentShaderCodeSize = (size_t)fileSize;
		FragmentShaderCode = (char*)malloc(FragmentShaderCodeSize);
		if (!FragmentShaderCode)
		{
			fclose(file);
			return -1;
		}

		fseek(file, 0, SEEK_SET);
		size_t readSize = fread(FragmentShaderCode, 1, FragmentShaderCodeSize, file);
		fclose(file);

		if (readSize != FragmentShaderCodeSize)
		{
			free(FragmentShaderCode);
			return -1;
		}
	}

	BmRender_ShaderDescription FragShaderDesc = {};
	FragShaderDesc.Code = reinterpret_cast<const u32*>(FragmentShaderCode);
	FragShaderDesc.CodeSize = FragmentShaderCodeSize;
	FragShaderDesc.Stage = BmRender_PipelineShaderStage::Fragment;
	BmRender_Shader FragmentShader = BmRender_CreateShader(&FragShaderDesc);

	free(FragmentShaderCode);

	struct Vertex
	{
		f32 x, y, z;
		f32 u, v;
	};

	Vertex TriangleVertices[3] = {
		{ 0.0f, -0.5f, 0.0f, 0.5f, 0.0f },
		{ 0.5f,  0.5f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, 0.5f, 0.0f, 0.0f, 1.0f }
	};

	const u64 VertexBufferSize = sizeof(TriangleVertices);
	BmRender_GPUBuffer VertexBuffer = BmRender_CreateVertexStageBuffer(VertexBufferSize, MemoryPropertyFlag::HostCompatible);
	BmRender_UpdateHostCompatibleBuffer(VertexBuffer, 0, VertexBufferSize, TriangleVertices);

	struct VkDrawIndirectCommand
	{
		u32 vertexCount;
		u32 instanceCount;
		u32 firstVertex;
		u32 firstInstance;
	};

	VkDrawIndirectCommand DrawCommand = {};
	DrawCommand.vertexCount = 3;
	DrawCommand.instanceCount = 1;
	DrawCommand.firstVertex = 0;
	DrawCommand.firstInstance = 0;

	BmRender_GPUBuffer IndirectDrawBuffer = BmRender_CreateIndirectDrawBuffer(sizeof(VkDrawIndirectCommand), MemoryPropertyFlag::HostCompatible);
	BmRender_UpdateHostCompatibleBuffer(IndirectDrawBuffer, 0, sizeof(VkDrawIndirectCommand), &DrawCommand);

	BmRender_VertexBinding VertexBinding = {};
	VertexBinding.Attributes = nullptr;
	VertexBinding.AttributesCount = 0;
	VertexBinding.Stride = 0;
	VertexBinding.InputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	struct PushConstants
	{
		u64 vertexBufferAddress;  // 8 bytes, offset 0
		u32 vertexStride;         // 4 bytes, offset 8
		u32 padding;              // 4 bytes padding to maintain alignment
	};

	BmRender_PushConstant PushConstant = BmRender_CreatePushConstant(
		BmRender_DescriptorShaderStage::Vertex,
		0,
		sizeof(PushConstants)
	);

	BmRender_PipelineLayoutDescription LayoutDesc = {};
	LayoutDesc.SetLayoutCount = 0;
	LayoutDesc.SetLayouts = nullptr;
	LayoutDesc.PushConstantRangeCount = 1;
	LayoutDesc.PushConstantRanges = &PushConstant;
	BmRender_PipelineLayout PipelineLayout = BmRender_CreatePipelineLayout(&LayoutDesc);

	VulkanCoreContext::VulkanCoreContext* CoreContext = GetCoreContext();
	
	BmRender_PipelineDescription PipelineDesc = {};
	PipelineDesc.PipelineLayout = PipelineLayout;

	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 1;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentFormats[0] = CoreContext->SurfaceFormat.format;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	BmRender_ShaderStageDescription ShaderStages[2] = {};
	ShaderStages[0].Shader = VertexShader;
	ShaderStages[0].EntryPointFunction = "main";
	ShaderStages[1].Shader = FragmentShader;
	ShaderStages[1].EntryPointFunction = "main";
	PipelineDesc.ShaderStages = ShaderStages;
	PipelineDesc.ShaderStagesCount = 2;

	PipelineDesc.VertexBindings = &VertexBinding;
	PipelineDesc.VertexBindingsCount = 1;

	PipelineDesc.DescriptorSetLayouts = nullptr;
	PipelineDesc.DescriptorSetLayoutsCount = 0;
	PipelineDesc.PushConstantRanges = nullptr;
	PipelineDesc.PushConstantRangesCount = 0;

	PipelineDesc.RasterizationState = {};
	PipelineDesc.RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	PipelineDesc.RasterizationState.pNext = nullptr;
	PipelineDesc.RasterizationState.flags = 0;
	PipelineDesc.RasterizationState.depthClampEnable = VK_FALSE;
	PipelineDesc.RasterizationState.rasterizerDiscardEnable = VK_FALSE;
	PipelineDesc.RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	PipelineDesc.RasterizationState.lineWidth = 1.0f;
	PipelineDesc.RasterizationState.cullMode = VK_CULL_MODE_NONE;
	PipelineDesc.RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	PipelineDesc.RasterizationState.depthBiasEnable = VK_FALSE;
	PipelineDesc.RasterizationState.depthBiasConstantFactor = 0.0f;
	PipelineDesc.RasterizationState.depthBiasClamp = 0.0f;
	PipelineDesc.RasterizationState.depthBiasSlopeFactor = 0.0f;

	PipelineDesc.ColorBlendAttachment = {};
	PipelineDesc.ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
	                                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	PipelineDesc.ColorBlendAttachment.blendEnable = VK_FALSE;
	PipelineDesc.ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	PipelineDesc.ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	PipelineDesc.ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	PipelineDesc.ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	PipelineDesc.ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	PipelineDesc.ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	PipelineDesc.ColorBlendState = {};
	PipelineDesc.ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	PipelineDesc.ColorBlendState.pNext = nullptr;
	PipelineDesc.ColorBlendState.flags = 0;
	PipelineDesc.ColorBlendState.logicOpEnable = VK_FALSE;
	PipelineDesc.ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
	PipelineDesc.ColorBlendState.attachmentCount = 1;
	PipelineDesc.ColorBlendState.pAttachments = &PipelineDesc.ColorBlendAttachment;
	PipelineDesc.ColorBlendState.blendConstants[0] = 0.0f;
	PipelineDesc.ColorBlendState.blendConstants[1] = 0.0f;
	PipelineDesc.ColorBlendState.blendConstants[2] = 0.0f;
	PipelineDesc.ColorBlendState.blendConstants[3] = 0.0f;

	PipelineDesc.DepthStencilState = {};
	PipelineDesc.DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	PipelineDesc.DepthStencilState.pNext = nullptr;
	PipelineDesc.DepthStencilState.flags = 0;
	PipelineDesc.DepthStencilState.depthTestEnable = VK_FALSE;
	PipelineDesc.DepthStencilState.depthWriteEnable = VK_FALSE;
	PipelineDesc.DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	PipelineDesc.DepthStencilState.depthBoundsTestEnable = VK_FALSE;
	PipelineDesc.DepthStencilState.stencilTestEnable = VK_FALSE;
	PipelineDesc.DepthStencilState.front = {};
	PipelineDesc.DepthStencilState.back = {};

	PipelineDesc.MultisampleState = {};
	PipelineDesc.MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	PipelineDesc.MultisampleState.pNext = nullptr;
	PipelineDesc.MultisampleState.flags = 0;
	PipelineDesc.MultisampleState.sampleShadingEnable = VK_FALSE;
	PipelineDesc.MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	PipelineDesc.MultisampleState.minSampleShading = 1.0f;
	PipelineDesc.MultisampleState.pSampleMask = nullptr;
	PipelineDesc.MultisampleState.alphaToCoverageEnable = VK_FALSE;
	PipelineDesc.MultisampleState.alphaToOneEnable = VK_FALSE;

	PipelineDesc.InputAssemblyState = {};
	PipelineDesc.InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	PipelineDesc.InputAssemblyState.pNext = nullptr;
	PipelineDesc.InputAssemblyState.flags = 0;
	PipelineDesc.InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PipelineDesc.InputAssemblyState.primitiveRestartEnable = VK_FALSE;

	PipelineDesc.Extent = { (u32)WindowWidth, (u32)WindowHeight };
	VkViewport Viewport = {};
	Viewport.x = 0.0f;
	Viewport.y = 0.0f;
	Viewport.width = (f32)WindowWidth;
	Viewport.height = (f32)WindowHeight;
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;
	PipelineDesc.Viewport = Viewport;

	VkRect2D Scissor = {};
	Scissor.offset = { 0, 0 };
	Scissor.extent = { (u32)WindowWidth, (u32)WindowHeight };
	PipelineDesc.Scissor = Scissor;

	PipelineDesc.ViewportState = {};
	PipelineDesc.ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	PipelineDesc.ViewportState.pNext = nullptr;
	PipelineDesc.ViewportState.flags = 0;
	PipelineDesc.ViewportState.viewportCount = 1;
	PipelineDesc.ViewportState.scissorCount = 1;
	PipelineDesc.ViewportState.pViewports = &PipelineDesc.Viewport;
	PipelineDesc.ViewportState.pScissors = &PipelineDesc.Scissor;

	BmRender_Pipeline Pipeline = BmRender_CreatePipeline(&PipelineDesc);

	VkQueue GraphicsQueue;
	vkGetDeviceQueue(CoreContext->LogicalDevice, (u32)CoreContext->Indices.GraphicsFamily, 0, &GraphicsQueue);

	BmRender_CommandPool CommandPool = BmRender_CreateCommandPool(
		(u32)CoreContext->Indices.GraphicsFamily,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	);
	BmRender_CommandBuffer CommandBuffer = BmRender_AllocateCommandBuffer(CommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	BmRender_BinarySemaphore ImageAvailableSemaphore = BmRender_CreateSemaphore();
	BmRender_BinarySemaphore RenderFinishedSemaphore = BmRender_CreateSemaphore();
	BmRender_Fence InFlightFence = BmRender_CreateFence();

	CommandBufferData* CmdBufferData = GetCommandBufferData(CommandBuffer);
	VkCommandBuffer VkCmdBuffer = CmdBufferData->VulkanCommandBuffer;
	GPUBufferData* VtxBufferData = GetGPUBufferData(VertexBuffer);
	VkBuffer VkVertexBuffer = VtxBufferData->Buffer;
	GPUBufferData* IndirectDrawBufferData = GetGPUBufferData(IndirectDrawBuffer);
	VkBuffer VkIndirectDrawBuffer = IndirectDrawBufferData->Buffer;
	PipelineData* PipelineData = GetPipelineData(Pipeline);
	VkPipeline VkPipeline = PipelineData->VulkanPipeline;

	VkBufferDeviceAddressInfo BufferDeviceAddressInfo = {};
	BufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	BufferDeviceAddressInfo.buffer = VkVertexBuffer;
	VkDeviceAddress VertexBufferAddress = vkGetBufferDeviceAddress(CoreContext->LogicalDevice, &BufferDeviceAddressInfo);
	
	// Verify device address is valid
	if (VertexBufferAddress == 0)
	{
		printf("ERROR: Vertex buffer device address is 0!\n");
		return -1;
	}

	printf("Vertex buffer device address: 0x%llx\n", (unsigned long long)VertexBufferAddress);
	printf("Vertex stride: %u\n", (u32)sizeof(Vertex));

	PushConstants PushConstantsData = {};
	PushConstantsData.vertexBufferAddress = VertexBufferAddress;
	PushConstantsData.vertexStride = sizeof(Vertex);
	PushConstantsData.padding = 0;  // Initialize padding

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();

		BmRender_WaitForFences(InFlightFence, VK_TRUE, UINT64_MAX);
		BmRender_ResetFences(InFlightFence);

		u32 ImageIndex;
		BmRender_SwapchainResult AcquireResult = BmRender_AcquireNextSwapchainImage(UINT64_MAX, ImageAvailableSemaphore,
			nullptr, &ImageIndex
		);

		if (AcquireResult == BmRender_SwapchainResult::OutOfDate || AcquireResult == BmRender_SwapchainResult::Suboptimal)
		{
			continue;
		}

		ImageViewData* ImageViewData = GetImageViewData(CoreContext->ImageViews[ImageIndex]);
		VkImageView SwapchainImageView = ImageViewData->View;

		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		BmRender_BeginCommandBuffer(CommandBuffer, &BeginInfo);

		VkImageMemoryBarrier2 SwapchainBarrier = {};
		SwapchainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		SwapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		SwapchainBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		SwapchainBarrier.image = GetImageData(CoreContext->Images[ImageIndex])->Image;
		SwapchainBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		SwapchainBarrier.subresourceRange.baseMipLevel = 0;
		SwapchainBarrier.subresourceRange.levelCount = 1;
		SwapchainBarrier.subresourceRange.baseArrayLayer = 0;
		SwapchainBarrier.subresourceRange.layerCount = 1;
		SwapchainBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainBarrier.srcAccessMask = 0;
		SwapchainBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

		VkDependencyInfo DepInfo = {};
		DepInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		DepInfo.imageMemoryBarrierCount = 1;
		DepInfo.pImageMemoryBarriers = &SwapchainBarrier;
		vkCmdPipelineBarrier2(VkCmdBuffer, &DepInfo);

		VkRenderingAttachmentInfo ColorAttachment = {};
		ColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		ColorAttachment.imageView = SwapchainImageView;
		ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingInfo RenderingInfo = {};
		RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		RenderingInfo.renderArea.offset = { 0, 0 };
		RenderingInfo.renderArea.extent = CoreContext->SwapExtent;
		RenderingInfo.layerCount = 1;
		RenderingInfo.colorAttachmentCount = 1;
		RenderingInfo.pColorAttachments = &ColorAttachment;
		RenderingInfo.pDepthAttachment = nullptr;
		RenderingInfo.pStencilAttachment = nullptr;

		vkCmdBeginRendering(VkCmdBuffer, &RenderingInfo);

		vkCmdBindPipeline(VkCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkPipeline);

		PipelineLayoutData* PipelineLayoutData = GetPipelineLayoutData(PipelineLayout);
		vkCmdPushConstants(
			VkCmdBuffer,
			PipelineLayoutData->VulkanPipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(PushConstants),
			&PushConstantsData
		);

		vkCmdDrawIndirect(VkCmdBuffer, VkIndirectDrawBuffer, 0, 1, sizeof(VkDrawIndirectCommand));

		vkCmdEndRendering(VkCmdBuffer);

		SwapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		SwapchainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		SwapchainBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		SwapchainBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		SwapchainBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		SwapchainBarrier.dstAccessMask = 0;
		vkCmdPipelineBarrier2(VkCmdBuffer, &DepInfo);

		BmRender_EndCommandBuffer(CommandBuffer);

		VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		BmRender_SubmitInfo SubmitInfo = {};
		SubmitInfo.WaitDstStageFlags = WaitStages;
		SubmitInfo.WaitSemaphores = &ImageAvailableSemaphore;
		SubmitInfo.WaitSemaphoreCount = 1;
		SubmitInfo.SignalSemaphores = &RenderFinishedSemaphore;
		SubmitInfo.SignalSemaphoreCount = 1;
		SubmitInfo.CommandBuffers = &CommandBuffer;
		SubmitInfo.CommandBufferCount = 1;
		SubmitInfo.WaitTimelineSemaphores = nullptr;
		SubmitInfo.WaitTimelineSemaphoreCount = 0;
		SubmitInfo.SignalTimelineSemaphores = nullptr;
		SubmitInfo.SignalTimelineSemaphoreCount = 0;

		BmRender_QueueSubmit(GraphicsQueue, 1, &SubmitInfo, InFlightFence);

		BmRender_PresentInfo PresentInfo = {};
		PresentInfo.WaitSemaphores = &RenderFinishedSemaphore;
		PresentInfo.WaitSemaphoreCount = 1;
		PresentInfo.ImageIndices = &ImageIndex;

		BmRender_SwapchainResult PresentResult = BmRender_QueuePresent(GraphicsQueue, &PresentInfo);

	}

	vkDeviceWaitIdle(CoreContext->LogicalDevice);

	BmRender_DeInit();

	return 0;
}