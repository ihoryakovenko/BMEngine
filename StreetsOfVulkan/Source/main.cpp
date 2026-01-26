#include <Util/EngineTypes.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <RenderInterface.h>

#include <ShortTypes.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Vertex {
	f32 pos[4];
	f32 texCoord[4];
};

Vertex CubeVertices[36] = {
	// Front face (z = 0.5, viewed from +Z, CCW)
	{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
	{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
	{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},

	// Back face (z = -0.5, viewed from -Z, CCW)
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
	{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
	{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},

	// Left face (x = -0.5, viewed from -X, CCW)
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
	{{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
	{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
	{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
	{{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},

	// Right face (x = 0.5, viewed from +X, CCW)
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
	{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},

	// Top face (y = 0.5, viewed from +Y, CCW)
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},
	{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},

	// Bottom face (y = -0.5, viewed from -Y, CCW)
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
	{{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}},
	{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
	{{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}},
	{{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}},
};

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

u64 ReadFile(FILE* File, void** OutData)
{
	fseek(File, 0, SEEK_END);
	const u32 FileSize = ftell(File);
	if (FileSize < 0)
	{
		return 0;
	}

	*OutData = (char*)malloc(FileSize);

	fseek(File, 0, SEEK_SET);
	return fread(*OutData, 1, FileSize, File);
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

	BmRender_DescriptorSetLayoutBinding DescriptorBinding = {};
	DescriptorBinding.DescriptorType = BmRender_DescriptorType::CombinedImageSampler;
	DescriptorBinding.DescriptorCount = 1;
	DescriptorBinding.StageFlags = BmRender_DescriptorShaderStage::Fragment;
	BmRender_DescriptorSetLayout DescriptorSetLayout = BmRender_CreateDescriptorSetLayout(&DescriptorBinding, 1);

	BmRender_PushConstant PushConstantRange = BmRender_CreatePushConstant(BmRender_DescriptorShaderStage::Vertex, 0, sizeof(glm::mat4) + sizeof(u64));

	BmRender_Extent2D SwapchainExtent = BmRender_GetSwapchainExtent();
	BmRender_Image DepthImage = BmRender_CreateImage2D(SwapchainExtent.Width, SwapchainExtent.Height, BmRender_Format::D32_SFLOAT_S8_UINT, BmRender_ImageType::DepthSamplad);
	BmRender_ImageView DepthImageView = BmRender_CreateImageView2D(DepthImage);

	BmRender_PipelineLayoutDescription LayoutDesc = {};
	LayoutDesc.SetLayoutCount = 1;
	LayoutDesc.SetLayouts = &DescriptorSetLayout;
	LayoutDesc.PushConstantRangeCount = 1;
	LayoutDesc.PushConstantRanges = &PushConstantRange;
	LayoutDesc.PipelineType = BmRender_PipelineType::Graphics;
	BmRender_PipelineLayout PipelineLayout = BmRender_CreatePipelineLayout(&LayoutDesc);
	
	BmRender_PipelineDescription PipelineDesc = {};
	PipelineDesc.PipelineLayout = PipelineLayout;

	BmRender_ImageView PipelineColorAttachments[1];
	PipelineColorAttachments[0] = BmRender_GetSwapchainImageView(0);

	PipelineDesc.Attachment.ColorAttachmentCount = 1;
	PipelineDesc.Attachment.ColorAttachments = PipelineColorAttachments;
	PipelineDesc.Attachment.DepthAttachment = DepthImageView;
	PipelineDesc.Attachment.StencilAttachment = nullptr;

	BmRender_ShaderStageDescription ShaderStages[2] = {};
	ShaderStages[0].Shader = VertexShader;
	ShaderStages[0].EntryPointFunction = "main";
	ShaderStages[1].Shader = FragmentShader;
	ShaderStages[1].EntryPointFunction = "main";
	PipelineDesc.ShaderStages = ShaderStages;
	PipelineDesc.ShaderStagesCount = 2;

	PipelineDesc.VertexBindings = nullptr;
	PipelineDesc.VertexBindingsCount = 0;

	PipelineDesc.DescriptorSetLayouts = &DescriptorSetLayout;
	PipelineDesc.DescriptorSetLayoutsCount = 1;
	PipelineDesc.PushConstantRanges = &PushConstantRange;
	PipelineDesc.PushConstantRangesCount = 1;

	PipelineDesc.RasterizationState = {};
	PipelineDesc.RasterizationState.depthClampEnable = false;
	PipelineDesc.RasterizationState.rasterizerDiscardEnable = false;
	PipelineDesc.RasterizationState.polygonMode = BmRender_PolygonMode::Fill;
	PipelineDesc.RasterizationState.lineWidth = 1.0f;
	PipelineDesc.RasterizationState.cullMode = BmRender_CullModeFlags::Back;
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
	PipelineDesc.DepthStencilState.depthTestEnable = true;
	PipelineDesc.DepthStencilState.depthWriteEnable = true;
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

	const u64 VertexBufferSize = sizeof(CubeVertices);
	BmRender_GPUBuffer StagingBuffer = BmRender_CreateStagingBuffer(VertexBufferSize);
	BmRender_GPUBuffer VertexBuffer = BmRender_CreateVertexStageBuffer(VertexBufferSize, MemoryPropertyFlag::GPULocal);

	BmRender_Pipeline Pipeline = BmRender_CreatePipeline(&PipelineDesc);
	BmRender_Queue GraphicsQueue = BmRender_CreateQueue(BmRender_QueueType::Graphic);
	BmRender_CommandPool CommandPool = BmRender_CreateCommandPool(BmRender_QueueType::Graphic);
	BmRender_CommandBuffer CommandBuffer = BmRender_AllocateCommandBuffer(CommandPool);
	BmRender_Semaphore ImageAvailableSemaphore = BmRender_CreateSemaphore();
	BmRender_Semaphore RenderFinishedSemaphore = BmRender_CreateSemaphore();
	BmRender_Fence InFlightFence = BmRender_CreateFence();

	BmRHI_SamplerDescription SamplerDesc = {};
	SamplerDesc.MagFilter = BmRender_Filter::Linear;
	SamplerDesc.MinFilter = BmRender_Filter::Linear;
	SamplerDesc.MipmapMode = BmRender_SamplerMipmapMode::Linear;
	SamplerDesc.AddressModeU = BmRender_SamplerAddressMode::ClampToEdge;
	SamplerDesc.AddressModeV = BmRender_SamplerAddressMode::ClampToEdge;
	SamplerDesc.AddressModeW = BmRender_SamplerAddressMode::ClampToEdge;
	SamplerDesc.MipLodBias = 0.0f;
	SamplerDesc.AnisotropyEnable = false;
	SamplerDesc.MaxAnisotropy = 1.0f;
	SamplerDesc.CompareEnable = false;
	SamplerDesc.CompareOp = BmRender_CompareOp::Always;
	SamplerDesc.MinLod = 0.0f;
	SamplerDesc.MaxLod = 0.0f;
	SamplerDesc.BorderColor = BmRender_BorderColor::FloatOpaqueBlack;
	SamplerDesc.UnnormalizedCoordinates = false;
	BmRender_Sampler AtlasSampler = BmRender_CreateSampler(&SamplerDesc);

	BmRender_UpdateHostCompatibleBuffer(StagingBuffer, 0, VertexBufferSize, CubeVertices);

	BmRender_BeginCommandBuffer(CommandBuffer);
	BmRender_RecordUpdateGPULocalBuffer(CommandBuffer, VertexBuffer, StagingBuffer, 0, 0, VertexBufferSize);
	BmRender_EndCommandBuffer(CommandBuffer);

	BmRender_SubmitInfo TransferSubmitInfo = {};
	TransferSubmitInfo.CommandBuffers = &CommandBuffer;
	TransferSubmitInfo.CommandBufferCount = 1;
	TransferSubmitInfo.WaitDstStageFlags = nullptr;
	TransferSubmitInfo.WaitSemaphores = nullptr;
	TransferSubmitInfo.WaitSemaphoreCount = 0;
	TransferSubmitInfo.SignalSemaphores = nullptr;
	TransferSubmitInfo.SignalSemaphoreCount = 0;
	TransferSubmitInfo.WaitTimelineSemaphores = nullptr;
	TransferSubmitInfo.WaitTimelineSemaphoreCount = 0;
	TransferSubmitInfo.SignalTimelineSemaphores = nullptr;
	TransferSubmitInfo.SignalTimelineSemaphoreCount = 0;

	BmRender_QueueSubmit(GraphicsQueue, 1, &TransferSubmitInfo, nullptr);
	BmRender_DeviceWaitIdle();

	f32 aspect = (f32)WindowWidth / (f32)WindowHeight;
	f32 fov = glm::radians(45.0f);
	f32 near = 0.1f;
	f32 far = 100.0f;
	
	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	f32 cameraSpeed = 0.05f;

	f32 time = 0.0f;

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
		time += 0.016f;

		if (glfwGetKey(Window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPos += cameraSpeed * cameraFront;
		if (glfwGetKey(Window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPos -= cameraSpeed * cameraFront;
		if (glfwGetKey(Window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(Window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(Window, GLFW_KEY_Q) == GLFW_PRESS)
			cameraPos -= cameraUp * cameraSpeed;
		if (glfwGetKey(Window, GLFW_KEY_E) == GLFW_PRESS)
			cameraPos += cameraUp * cameraSpeed;

		glm::mat4 proj = glm::perspective(fov, aspect, near, far);
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		f32 rotY = time * 0.5f;
		f32 rotX = time * 0.3f;
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(model, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rotX, glm::vec3(1.0f, 0.0f, 0.0f));

		glm::mat4 mvp = proj * view * model;

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
		BmRender_TransitionImageForRendering(CommandBuffer, DepthImage);

		BmRender_RenderingColorAttachment ColorAttachment = {};
		ColorAttachment.ImageView = BmRender_GetSwapchainImageView(ImageIndex);
		ColorAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
		ColorAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
		ColorAttachment.ClearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		BmRender_RenderingDepthAttachment DepthAttachment = {};
		DepthAttachment.ImageView = DepthImageView;
		DepthAttachment.LoadOp = BmRender_AttachmentLoadOp::Clear;
		DepthAttachment.StoreOp = BmRender_AttachmentStoreOp::Store;
		DepthAttachment.ClearValue = { 1.0f, 0 };  // Clear to max depth

		BmRender_RenderingInfo RenderingInfo = {};
		RenderingInfo.Offset = { 0, 0 };
		RenderingInfo.Extent = BmRender_GetSwapchainExtent();
		RenderingInfo.ColorAttachments = &ColorAttachment;
		RenderingInfo.ColorAttachmentCount = 1;
		RenderingInfo.DepthAttachment = &DepthAttachment;

		BmRender_BeginRendering(CommandBuffer, &RenderingInfo);

		BmRender_BindPipeline(CommandBuffer, Pipeline);
		
		struct PushConstants {
			glm::mat4 mvp;
			u64 vertexBufferAddress;
		};
		PushConstants pc;
		pc.mvp = mvp;
		pc.vertexBufferAddress = BmRender_GetBufferDeviceAddress(VertexBuffer);
		BmRender_RecordPushConstants(CommandBuffer, PipelineLayout, BmRender_DescriptorShaderStage::Vertex, 0, sizeof(PushConstants), &pc);

		BmRender_Draw(CommandBuffer, 36, 1, 0, 0);

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

		BmRender_FrameFree();
	}

	BmRender_QueueWaitIdle(GraphicsQueue);

	BmRender_DestroyImageView(DepthImageView);
	BmRender_DestroyImage(DepthImage);
	BmRender_DestroyGPUBuffer(VertexBuffer);
	BmRender_DestroySampler(AtlasSampler);
	BmRender_DestroyDescriptorSetLayout(DescriptorSetLayout);
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