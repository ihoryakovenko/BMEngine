#include "Settings.h"
#include "Engine/Systems/StaticMeshRender.h"

VkExtent2D MainScreenExtent;
VkExtent2D DepthViewportExtent = { 1024, 1024 };

VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

VulkanInterface::VertexInput SkyBoxVertexInput;
VulkanInterface::VertexInput DepthVertexInput;

VulkanInterface::PipelineSettings DeferredPipelineSettings;
VulkanInterface::PipelineSettings SkyBoxPipelineSettings;
VulkanInterface::PipelineSettings DepthPipelineSettings;

VulkanInterface::RenderPassSettings MainRenderPassSettings;

VkImageCreateInfo ShadowMapArrayCreateInfo;

VulkanInterface::UniformImageInterfaceCreateInfo DeferredInputDepthUniformInterfaceCreateInfo;
VulkanInterface::UniformImageInterfaceCreateInfo DeferredInputUniformColorInterfaceCreateInfo;
VulkanInterface::UniformImageInterfaceCreateInfo ShadowMapElement1InterfaceCreateInfo;
VulkanInterface::UniformImageInterfaceCreateInfo ShadowMapElement2InterfaceCreateInfo;

VkClearValue MainPassClearValues[3];
VkClearValue DepthPassClearValues;

VkBufferCreateInfo VpBufferInfo;

enum SubpassIndex
{
	MainSubpass = 0,

	Count
};

enum Subpasses
{
	Depth,

	Subpasses_Count
};

static const u32 MainPathAttachmentDescriptionsCount = 3;
static VkAttachmentDescription MainPathAttachmentDescriptions[MainPathAttachmentDescriptionsCount];
static VkAttachmentReference EntitySubpassColourAttachmentReference = { };
static VkAttachmentReference EntitySubpassDepthAttachmentReference = { };
static VkAttachmentReference SwapchainColorAttachmentReference = { };
static const u32 InputReferencesCount = 2;
static VkAttachmentReference DeferredSubpassInputReferences[InputReferencesCount];
static const u32 ExitDependenciesIndex = SubpassIndex::Count;
static const u32 MainPassSubpassDependenciesCount = SubpassIndex::Count + 1;
static VkSubpassDependency MainPassSubpassDependencies[MainPassSubpassDependenciesCount];
static VulkanInterface::SubpassSettings MainRenderPassSubpasses[Count];

static const char DeferredPipelineName[] = "Deferred";
static const char SkyBoxPipelineName[] = "SkyBox";
static const char DepthPipelineName[] = "Depth";

static const char SkyBoxVertexInputName[] = "SkyBoxVertex";
static const char QuadSphereVertexInputName[] = "QuadSphereVertex";

static const char MainRenderPassName[] = "MainRenderPass";
static const char MainSubpassName[] = "MainSubpass";
static const char DeferredSubpassName[] = "DeferredSubpass";

static const char DepthRenderPassName[] = "DepthRenderPass";
static const char DepthSubpassName[] = "DepthSubpass";

void LoadSettings(u32 WindowWidth, u32 WindowHeight)
{
	MainScreenExtent = { WindowWidth, WindowHeight };

	VpBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	VpBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	VpBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// Images
	ShadowMapArrayCreateInfo = { };
	ShadowMapArrayCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ShadowMapArrayCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	ShadowMapArrayCreateInfo.extent.depth = 1;
	ShadowMapArrayCreateInfo.mipLevels = 1;
	ShadowMapArrayCreateInfo.arrayLayers = 1;
	ShadowMapArrayCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ShadowMapArrayCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ShadowMapArrayCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ShadowMapArrayCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ShadowMapArrayCreateInfo.flags = 0;
	ShadowMapArrayCreateInfo.format = ColorFormat;
	ShadowMapArrayCreateInfo.extent.width = DepthViewportExtent.width;
	ShadowMapArrayCreateInfo.extent.height = DepthViewportExtent.height;
	ShadowMapArrayCreateInfo.format = DepthFormat;
	ShadowMapArrayCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	ShadowMapArrayCreateInfo.arrayLayers = 2;

	// Image views
	DeferredInputDepthUniformInterfaceCreateInfo = { };
	DeferredInputDepthUniformInterfaceCreateInfo.Flags = 0; // No flags
	DeferredInputDepthUniformInterfaceCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	DeferredInputDepthUniformInterfaceCreateInfo.Format = DepthFormat;
	DeferredInputDepthUniformInterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	DeferredInputDepthUniformInterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	DeferredInputDepthUniformInterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	DeferredInputDepthUniformInterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	DeferredInputDepthUniformInterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	DeferredInputDepthUniformInterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
	DeferredInputDepthUniformInterfaceCreateInfo.SubresourceRange.levelCount = 1;
	DeferredInputDepthUniformInterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
	DeferredInputDepthUniformInterfaceCreateInfo.SubresourceRange.layerCount = 1;

	DeferredInputUniformColorInterfaceCreateInfo = DeferredInputDepthUniformInterfaceCreateInfo;
	DeferredInputUniformColorInterfaceCreateInfo.Format = ColorFormat;
	DeferredInputUniformColorInterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	ShadowMapElement1InterfaceCreateInfo = DeferredInputDepthUniformInterfaceCreateInfo;
	ShadowMapElement1InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;

	ShadowMapElement2InterfaceCreateInfo = ShadowMapElement1InterfaceCreateInfo;
	ShadowMapElement2InterfaceCreateInfo.SubresourceRange.baseArrayLayer = 1;

	// MAIN RENDERPASS
	MainPassClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	MainPassClearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	MainPassClearValues[2].depthStencil.depth = 1.0f;

	const u32 SwapchainColorAttachmentIndex = 0;
	const u32 SubpassColorAttachmentIndex = 1;
	const u32 SubpassDepthAttachmentIndex = 2;

	MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex] = { };
	MainPathAttachmentDescriptions[SwapchainColorAttachmentIndex].format = VK_FORMAT_UNDEFINED;  // Find format in runtime
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

	EntitySubpassColourAttachmentReference.attachment = SubpassColorAttachmentIndex;
	EntitySubpassColourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	EntitySubpassDepthAttachmentReference.attachment = SubpassDepthAttachmentIndex;
	EntitySubpassDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	SwapchainColorAttachmentReference.attachment = SwapchainColorAttachmentIndex;
	SwapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	DeferredSubpassInputReferences[0].attachment = 1;
	DeferredSubpassInputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	DeferredSubpassInputReferences[1].attachment = 2;
	DeferredSubpassInputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	MainPassSubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	MainPassSubpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	MainPassSubpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	MainPassSubpassDependencies[0].dstSubpass = SubpassIndex::MainSubpass;
	MainPassSubpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	MainPassSubpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	MainPassSubpassDependencies[0].dependencyFlags = 0;

	MainPassSubpassDependencies[1].srcSubpass = SubpassIndex::MainSubpass;
	MainPassSubpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	MainPassSubpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	MainPassSubpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	MainPassSubpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	MainPassSubpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	MainPassSubpassDependencies[1].dependencyFlags = 0;

	MainRenderPassSubpasses[MainSubpass].ColorAttachmentsReferences = &EntitySubpassColourAttachmentReference;
	MainRenderPassSubpasses[MainSubpass].ColorAttachmentsReferencesCount = 1;
	MainRenderPassSubpasses[MainSubpass].DepthAttachmentReferences = &EntitySubpassDepthAttachmentReference;
	MainRenderPassSubpasses[MainSubpass].SubpassName = MainSubpassName;

	MainRenderPassSettings.AttachmentDescriptions = MainPathAttachmentDescriptions;
	MainRenderPassSettings.AttachmentDescriptionsCount = MainPathAttachmentDescriptionsCount;
	MainRenderPassSettings.SubpassesSettings = MainRenderPassSubpasses;
	MainRenderPassSettings.SubpassSettingsCount = Count;
	MainRenderPassSettings.RenderPassName = MainRenderPassName;
	MainRenderPassSettings.SubpassDependencies = MainPassSubpassDependencies;
	MainRenderPassSettings.SubpassDependenciesCount = MainPassSubpassDependenciesCount;
	MainRenderPassSettings.ClearValues = MainPassClearValues;
	MainRenderPassSettings.ClearValuesCount = 3;


	// Dynamic states TODO
// Use vkCmdSetViewport(Commandbuffer, 0, 1, &Viewport) and vkCmdSetScissor(Commandbuffer, 0, 1, &Scissor)
//VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

//VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
//DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//DynamicStateCreateInfo.dynamicStateCount = static_cast<u32>(2);
//DynamicStateCreateInfo.pDynamicStates = DynamicStates;




	// TerrainPipeline

	// DeferredPipelineSettings
	DeferredPipelineSettings.PipelineName = DeferredPipelineName;
	// Rasterizer
	DeferredPipelineSettings.Extent = MainScreenExtent;
	DeferredPipelineSettings.DepthClampEnable = VK_FALSE;
	DeferredPipelineSettings.RasterizerDiscardEnable = VK_FALSE;
	DeferredPipelineSettings.PolygonMode = VK_POLYGON_MODE_FILL;
	DeferredPipelineSettings.LineWidth = 1.0f;
	DeferredPipelineSettings.CullMode = VK_CULL_MODE_NONE;
	DeferredPipelineSettings.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	DeferredPipelineSettings.DepthBiasEnable = VK_FALSE;
	// Multisampling
	DeferredPipelineSettings.BlendEnable = VK_FALSE;
	DeferredPipelineSettings.LogicOpEnable = VK_FALSE;
	DeferredPipelineSettings.AttachmentCount = 1;
	DeferredPipelineSettings.ColorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	DeferredPipelineSettings.SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	DeferredPipelineSettings.DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	DeferredPipelineSettings.ColorBlendOp = VK_BLEND_OP_ADD;
	DeferredPipelineSettings.SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	DeferredPipelineSettings.DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	DeferredPipelineSettings.AlphaBlendOp = VK_BLEND_OP_ADD;
	// Depth testing
	DeferredPipelineSettings.DepthTestEnable = VK_FALSE;
	DeferredPipelineSettings.DepthWriteEnable = VK_FALSE;
	DeferredPipelineSettings.DepthCompareOp = VK_COMPARE_OP_LESS;
	DeferredPipelineSettings.DepthBoundsTestEnable = VK_FALSE;
	DeferredPipelineSettings.StencilTestEnable = VK_FALSE;

	// SkyBoxPipelineSettings
	SkyBoxPipelineSettings.PipelineName = SkyBoxPipelineName;
	// Rasterizer
	SkyBoxPipelineSettings.Extent = MainScreenExtent;
	SkyBoxPipelineSettings.DepthClampEnable = VK_FALSE;
	SkyBoxPipelineSettings.RasterizerDiscardEnable = VK_FALSE;
	SkyBoxPipelineSettings.PolygonMode = VK_POLYGON_MODE_FILL;
	SkyBoxPipelineSettings.LineWidth = 1.0f;
	SkyBoxPipelineSettings.CullMode = VK_CULL_MODE_FRONT_BIT;
	SkyBoxPipelineSettings.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	SkyBoxPipelineSettings.DepthBiasEnable = VK_FALSE;
	// Multisampling
	SkyBoxPipelineSettings.BlendEnable = VK_TRUE;
	SkyBoxPipelineSettings.LogicOpEnable = VK_FALSE;
	SkyBoxPipelineSettings.AttachmentCount = 1;
	SkyBoxPipelineSettings.ColorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	SkyBoxPipelineSettings.SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	SkyBoxPipelineSettings.DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	SkyBoxPipelineSettings.ColorBlendOp = VK_BLEND_OP_ADD;
	SkyBoxPipelineSettings.SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	SkyBoxPipelineSettings.DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	SkyBoxPipelineSettings.AlphaBlendOp = VK_BLEND_OP_ADD;
	// Depth testing
	SkyBoxPipelineSettings.DepthTestEnable = VK_TRUE;
	SkyBoxPipelineSettings.DepthWriteEnable = VK_FALSE;
	SkyBoxPipelineSettings.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	SkyBoxPipelineSettings.DepthBoundsTestEnable = VK_FALSE;
	SkyBoxPipelineSettings.StencilTestEnable = VK_FALSE;

	// DepthPipelineSettings
	DepthPipelineSettings.PipelineName = DepthPipelineName;
	// Rasterizer
	DepthPipelineSettings.Extent = DepthViewportExtent;
	DepthPipelineSettings.DepthClampEnable = VK_FALSE;
	DepthPipelineSettings.RasterizerDiscardEnable = VK_FALSE;
	DepthPipelineSettings.PolygonMode = VK_POLYGON_MODE_FILL;
	DepthPipelineSettings.LineWidth = 1.0f;
	DepthPipelineSettings.CullMode = VK_CULL_MODE_BACK_BIT;
	DepthPipelineSettings.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	DepthPipelineSettings.DepthBiasEnable = VK_FALSE;
	// Multisampling
	DepthPipelineSettings.BlendEnable = VK_TRUE;
	DepthPipelineSettings.LogicOpEnable = VK_FALSE;
	DepthPipelineSettings.AttachmentCount = 1;
	DepthPipelineSettings.ColorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	DepthPipelineSettings.SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	DepthPipelineSettings.DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	DepthPipelineSettings.ColorBlendOp = VK_BLEND_OP_ADD;
	DepthPipelineSettings.SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	DepthPipelineSettings.DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	DepthPipelineSettings.AlphaBlendOp = VK_BLEND_OP_ADD;
	// Depth testing
	DepthPipelineSettings.DepthTestEnable = VK_TRUE;
	DepthPipelineSettings.DepthWriteEnable = VK_TRUE;
	DepthPipelineSettings.DepthCompareOp = VK_COMPARE_OP_LESS;
	DepthPipelineSettings.DepthBoundsTestEnable = VK_FALSE;
	DepthPipelineSettings.StencilTestEnable = VK_FALSE;

	// Vertices
	SkyBoxVertexInput.VertexInputBinding[0].InputAttributes[0] = { "Position", VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkyBoxVertex, Position) };
	SkyBoxVertexInput.VertexInputBinding[0].InputAttributesCount = 1;
	SkyBoxVertexInput.VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	SkyBoxVertexInput.VertexInputBinding[0].Stride = sizeof(SkyBoxVertex);
	SkyBoxVertexInput.VertexInputBinding[0].VertexInputBindingName = SkyBoxVertexInputName;
	SkyBoxVertexInput.VertexInputBindingCount = 1;

	DepthVertexInput.VertexInputBinding[0].InputAttributes[0] = { "Position", VK_FORMAT_R32G32B32_SFLOAT, offsetof(StaticMeshRender::StaticMeshVertex, Position) };
	DepthVertexInput.VertexInputBinding[0].InputAttributesCount = 1;
	DepthVertexInput.VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	DepthVertexInput.VertexInputBinding[0].Stride = sizeof(StaticMeshRender::StaticMeshVertex);
	DepthVertexInput.VertexInputBinding[0].VertexInputBindingName = "EntityVertex";
	DepthVertexInput.VertexInputBinding[0].InputAttributesCount = 1;
	DepthVertexInput.VertexInputBindingCount = 1;
}
