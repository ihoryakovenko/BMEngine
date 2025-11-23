#include <Util/EngineTypes.h>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

#include <RenderInterface.h>

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
	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentFormats[0] = BmRender_GetSurfaceFormat().Format;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = BmRender_Format::Undefined;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = BmRender_Format::Undefined;

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
	PipelineDesc.RasterizationState.depthClampEnable = false;
	PipelineDesc.RasterizationState.rasterizerDiscardEnable = false;
	PipelineDesc.RasterizationState.polygonMode = BmRender_PolygonMode::Fill;
	PipelineDesc.RasterizationState.lineWidth = 1.0f;
	PipelineDesc.RasterizationState.cullMode = BmRender_CullModeFlags::None;
	PipelineDesc.RasterizationState.frontFace = BmRender_FrontFace::CounterClockwise;
	PipelineDesc.RasterizationState.depthBiasEnable = false;

	PipelineDesc.ColorBlendAttachment = {};
	PipelineDesc.ColorBlendAttachment.colorWriteMask = BmRender_ColorComponentFlags::RGBA;
	PipelineDesc.ColorBlendAttachment.blendEnable = false;
	PipelineDesc.ColorBlendAttachment.srcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
	PipelineDesc.ColorBlendAttachment.dstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
	PipelineDesc.ColorBlendAttachment.colorBlendOp = BmRender_BlendOp::Add;
	PipelineDesc.ColorBlendAttachment.srcAlphaBlendFactor = BmRender_BlendFactor::One;
	PipelineDesc.ColorBlendAttachment.dstAlphaBlendFactor = BmRender_BlendFactor::Zero;
	PipelineDesc.ColorBlendAttachment.alphaBlendOp = BmRender_BlendOp::Add;

	PipelineDesc.ColorBlendState = {};
	PipelineDesc.ColorBlendState.logicOpEnable = false;
	PipelineDesc.ColorBlendState.attachmentCount = 1;

	PipelineDesc.DepthStencilState = {};
	PipelineDesc.DepthStencilState.depthTestEnable = false;
	PipelineDesc.DepthStencilState.depthWriteEnable = false;
	PipelineDesc.DepthStencilState.depthCompareOp = BmRender_CompareOp::Less;
	PipelineDesc.DepthStencilState.depthBoundsTestEnable = false;
	PipelineDesc.DepthStencilState.stencilTestEnable = false;

	PipelineDesc.MultisampleState = {};
	PipelineDesc.MultisampleState.sampleShadingEnable = false;
	PipelineDesc.MultisampleState.rasterizationSamples = BmRender_SampleCount::Count1;

	PipelineDesc.InputAssemblyState = {};
	PipelineDesc.InputAssemblyState.topology = BmRender_PrimitiveTopology::TriangleList;
	PipelineDesc.InputAssemblyState.primitiveRestartEnable = false;

	PipelineDesc.Extent = { (u32)WindowWidth, (u32)WindowHeight };
	BmRender_Viewport Viewport = {};
	Viewport.X = 0.0f;
	Viewport.Y = 0.0f;
	Viewport.Width = (f32)WindowWidth;
	Viewport.Height = (f32)WindowHeight;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	PipelineDesc.Viewport = Viewport;

	BmRender_Rect2D Scissor = {};
	Scissor.Offset = { 0, 0 };
	Scissor.Extent = { (u32)WindowWidth, (u32)WindowHeight };
	PipelineDesc.Scissor = Scissor;

	PipelineDesc.ViewportState = {};
	PipelineDesc.ViewportState.viewportCount = 1;
	PipelineDesc.ViewportState.scissorCount = 1;

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

		BmRender_WaitForFences(InFlightFence, true, UINT64_MAX);
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
		ColorAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
		ColorAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
		ColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		BmRender_RenderingInfo RenderingInfo = {};
		RenderingInfo.Offset = { 0, 0 }; // BmRender_Offset2D
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

		BmRender_PipelineSyncStage WaitStages[] = { BmRender_PipelineSyncStage::ColorAttachmentOutput };

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