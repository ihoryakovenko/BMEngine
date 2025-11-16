#include <Util/EngineTypes.h>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

#include <Engine/Systems/Render/RenderInterface.h>

#include <ShortTypes.h>

static bool LoadShaderFile(const char* FilePath, char** OutCode, size_t* OutCodeSize)
{
	FILE* file = fopen(FilePath, "rb");
	if (!file)
	{
		return false;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	if (fileSize < 0)
	{
		fclose(file);
		return false;
	}

	size_t codeSize = (size_t)fileSize;
	char* code = (char*)malloc(codeSize);
	if (!code)
	{
		fclose(file);
		return false;
	}

	fseek(file, 0, SEEK_SET);
	size_t readSize = fread(code, 1, codeSize, file);
	fclose(file);

	if (readSize != codeSize)
	{
		free(code);
		return false;
	}

	*OutCode = code;
	*OutCodeSize = codeSize;
	return true;
}

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
	if (!LoadShaderFile("./test_vertex.vert.spv", &VertexShaderCode, &VertexShaderCodeSize))
	{
		return -1;
	}

	BmRender_ShaderDescription ShaderDesc = {};
	ShaderDesc.Code = reinterpret_cast<const u32*>(VertexShaderCode);
	ShaderDesc.CodeSize = VertexShaderCodeSize;
	ShaderDesc.Stage = BmRender_PipelineShaderStage::Vertex;
	BmRender_Shader VertexShader = BmRender_CreateShader(&ShaderDesc);

	free(VertexShaderCode);

	char* FragmentShaderCode = nullptr;
	size_t FragmentShaderCodeSize = 0;
	if (!LoadShaderFile("./test_fragment.frag.spv", &FragmentShaderCode, &FragmentShaderCodeSize))
	{
		return -1;
	}

	BmRender_ShaderDescription FragShaderDesc = {};
	FragShaderDesc.Code = reinterpret_cast<const u32*>(FragmentShaderCode);
	FragShaderDesc.CodeSize = FragmentShaderCodeSize;
	FragShaderDesc.Stage = BmRender_PipelineShaderStage::Fragment;
	BmRender_Shader FragmentShader = BmRender_CreateShader(&FragShaderDesc);

	free(FragmentShaderCode);

	BmRender_PipelineLayoutDescription LayoutDesc = {};
	LayoutDesc.SetLayoutCount = 0;
	LayoutDesc.SetLayouts = nullptr;
	LayoutDesc.PushConstantRangeCount = 0;
	LayoutDesc.PushConstantRanges = nullptr;
	LayoutDesc.PipelineType = BmRender_PipelineType::Graphics;
	BmRender_PipelineLayout PipelineLayout = BmRender_CreatePipelineLayout(&LayoutDesc);
	
	BmRender_PipelineDescription PipelineDesc = {};
	PipelineDesc.PipelineLayout = PipelineLayout;

	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 1;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentFormats[0] = BmRender_GetSurfaceFormat().format;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	BmRender_ShaderStageDescription ShaderStages[2] = {};
	ShaderStages[0].Shader = VertexShader;
	ShaderStages[0].EntryPointFunction = "main";
	ShaderStages[1].Shader = FragmentShader;
	ShaderStages[1].EntryPointFunction = "main";
	PipelineDesc.ShaderStages = ShaderStages;
	PipelineDesc.ShaderStagesCount = 2;

	PipelineDesc.VertexBindings = nullptr;
	PipelineDesc.VertexBindingsCount = 0;

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

	BmRender_Queue GraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);

	BmRender_CommandPool CommandPool = BmRender_CreateCommandPool(BmRender_QueueType::Graphic);
	BmRender_CommandBuffer CommandBuffer = BmRender_AllocateCommandBuffer(CommandPool);

	BmRender_Semaphore ImageAvailableSemaphore = BmRender_CreateSemaphore();
	BmRender_Semaphore RenderFinishedSemaphore = BmRender_CreateSemaphore();
	BmRender_Fence InFlightFence = BmRender_CreateFence();

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();

		BmRender_WaitForFences(InFlightFence, VK_TRUE, UINT64_MAX);
		BmRender_ResetFences(InFlightFence);

		u32 ImageIndex;
		BmRender_SwapchainResult AcquireResult = BmRender_AcquireNextSwapchainImage(UINT64_MAX, ImageAvailableSemaphore, nullptr, &ImageIndex);

		if (AcquireResult == BmRender_SwapchainResult::OutOfDate || AcquireResult == BmRender_SwapchainResult::Suboptimal)
		{
			continue;
		}

		BmRender_BeginCommandBuffer(CommandBuffer);
		BmRender_TransitionImageForRendering(CommandBuffer, BmRender_GetSwapchainImage(ImageIndex));

		BmRender_RenderingColorAttachment ColorAttachment = {};
		ColorAttachment.ImageView = BmRender_GetSwapchainImageView(ImageIndex);
		ColorAttachment.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		BmRender_RenderingInfo RenderingInfo = {};
		RenderingInfo.Offset = { 0, 0 };
		RenderingInfo.Extent = BmRender_GetSwapchainExtent();
		RenderingInfo.ColorAttachments = &ColorAttachment;
		RenderingInfo.ColorAttachmentCount = 1;
		RenderingInfo.DepthAttachment = nullptr;

		BmRender_BeginRendering(CommandBuffer, &RenderingInfo);

		BmRender_BindPipeline(CommandBuffer, Pipeline);

		BmRender_Draw(CommandBuffer, 6, 1, 0, 0);

		BmRender_EndRendering(CommandBuffer);

		BmRender_TransitionImageForPresentation(CommandBuffer, BmRender_GetSwapchainImage(ImageIndex));

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

	BmRender_QueueWaitIdle(GraphicsQueue);

	BmRender_DestroyShader(VertexShader);
	BmRender_DestroyShader(FragmentShader);
	BmRender_DestroySemaphore(ImageAvailableSemaphore);
	BmRender_DestroySemaphore(RenderFinishedSemaphore);
	BmRender_DestroyCommandPool(CommandPool);
	BmRender_DestroyFence(InFlightFence);
	BmRender_DestroyPipeline(Pipeline);
	BmRender_DestroyPipelineLayout(PipelineLayout);

	BmRender_DeInit();

	return 0;
}