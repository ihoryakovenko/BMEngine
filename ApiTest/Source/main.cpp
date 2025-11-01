#include <Util/EngineTypes.h>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>

#include <Engine/Systems/Memory/MemoryManagmentSystem.h>
#include <Engine/Systems/Render/RenderInterface.h>
#include <Engine/Systems/Render/RenderTypes.h>


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

	struct Vertex
	{
		f32 x, y, z;
	};

	Vertex TriangleVertices[3] = {
		{ 0.0f, -0.5f, 0.0f },  // Bottom
		{ 0.5f,  0.5f, 0.0f },  // Top right
		{ -0.5f, 0.5f, 0.0f }   // Top left
	};

	const u64 VertexBufferSize = sizeof(TriangleVertices);
	BmRender_VertexStageBuffer VertexBuffer = BmRender_CreateVertexStageBuffer(
		VertexBufferSize,
		BmRender_BufferUpdateFrequency::Static
	);

	BmRender_StagingBuffer StagingBuffer = BmRender_CreateStagingBuffer(VertexBufferSize);
	BmRender_UpdateStagingBuffer(StagingBuffer, 0, VertexBufferSize, TriangleVertices);

	// TODO: Copy staging buffer to vertex buffer using transfer commands
	// This would require additional render interface functions or direct Vulkan calls

	VertexAttribute VertexAttrib = {};
	VertexAttrib.Type = BmRender_AttributeType::Vec3;
	VertexAttrib.Offset = 0;

	BmRender_VertexBinding VertexBinding = {};
	VertexBinding.Attributes = &VertexAttrib;
	VertexBinding.AttributesCount = 1;
	VertexBinding.Stride = sizeof(Vertex);
	VertexBinding.InputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	BmRender_PipelineLayoutDescription LayoutDesc = {};
	LayoutDesc.SetLayoutCount = 0;
	LayoutDesc.SetLayouts = nullptr;
	LayoutDesc.PushConstantRangeCount = 0;
	LayoutDesc.PushConstantRanges = nullptr;
	BmRender_PipelineLayout PipelineLayout = BmRender_CreatePipelineLayout(&LayoutDesc);

	BmRender_PipelineDescription PipelineDesc = {};
	PipelineDesc.PipelineLayout = PipelineLayout;

	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentCount = 1;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.ColorAttachmentFormats[0] = VK_FORMAT_R8G8B8A8_UNORM;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.DepthAttachmentFormat = VK_FORMAT_UNDEFINED;
	PipelineDesc.ResourceInfo.PipelineAttachmentData.StencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	BmRender_ShaderStageDescription ShaderStage = {};
	ShaderStage.Shader = VertexShader;
	ShaderStage.EntryPointFunction = "main";
	PipelineDesc.ShaderStages = &ShaderStage;
	PipelineDesc.ShaderStagesCount = 1;

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



	BmRender_DestroyPipeline(Pipeline);
	BmRender_DestroyPipelineLayout(PipelineLayout);
	BmRender_DestroyShader(VertexShader);
	BmRender_DeInit();

	return 0;
}