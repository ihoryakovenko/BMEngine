#include "Settings.h"

VkExtent2D MainScreenExtent;
VkExtent2D DepthViewportExtent = { 1024, 1024 };

VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

BMR::BMRVertexInput EntityVertexInput;
BMR::BMRVertexInput TerrainVertexInput;
BMR::BMRVertexInput SkyBoxVertexInput;
BMR::BMRVertexInput DepthVertexInput;

BMR::BMRPipelineSettings EntityPipelineSettings;
BMR::BMRPipelineSettings TerrainPipelineSettings;
BMR::BMRPipelineSettings DeferredPipelineSettings;
BMR::BMRPipelineSettings SkyBoxPipelineSettings;
BMR::BMRPipelineSettings DepthPipelineSettings;

BMR::BMRRenderPassSettings MainRenderPassSettings;
BMR::BMRRenderPassSettings DepthRenderPassSettings;

VkImageCreateInfo DeferredInputDepthUniformCreateInfo;
VkImageCreateInfo DeferredInputColorUniformCreateInfo;

BMR::BMRUniformImageInterfaceCreateInfo DeferredInputUniformInterfaceCreateInfo;
BMR::BMRUniformImageInterfaceCreateInfo DeferredInputUniformColorInterfaceCreateInfo;

VkBufferCreateInfo VpBufferInfo;

enum SubpassIndex
{
	MainSubpass = 0,
	DeferredSubpass,

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
static BMR::BMRSubpassSettings MainRenderPassSubpasses[Count];

static const u32 DepthPassAttachmentDescriptionsCount = 1;
static VkAttachmentDescription DepthPassAttachmentDescriptions[DepthPassAttachmentDescriptionsCount];
static VkAttachmentReference DepthSubpassDepthAttachmentReference = { };
static BMR::BMRSubpassSettings DepthSubpasses[Subpasses_Count];
static const u32 DepthPassExitDependenciesIndex = Subpasses::Subpasses_Count;
static const u32 DepthPassSubpassDependenciesCount = Subpasses::Subpasses_Count + 1;
static VkSubpassDependency DepthPassSubpassDependencies[DepthPassSubpassDependenciesCount];

static const char EntityPipelineName[] = "Entity";
static const char TerrainPipelineName[] = "Terrain";
static const char DeferredPipelineName[] = "Deferred";
static const char SkyBoxPipelineName[] = "SkyBox";
static const char DepthPipelineName[] = "Depth";

static const char EntityVertexInputName[] = "EntityVertex";
static const char TerrainVertexInputName[] = "TerrainVertex";
static const char SkyBoxVertexInputName[] = "SkyBoxVertex";

static const char PositionAttributeName[] = "Position";
static const char ColorAttributeName[] = "Color";
static const char TextureCoordsAttributeName[] = "TextureCoords";
static const char NormalAttributeName[] = "Normal";
static const char AltitudeAttributeName[] = "Altitude";

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

	DeferredInputDepthUniformCreateInfo = { };
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
	DeferredInputDepthUniformCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	DeferredInputColorUniformCreateInfo = DeferredInputDepthUniformCreateInfo;
	DeferredInputColorUniformCreateInfo.format = ColorFormat;
	DeferredInputColorUniformCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

	DeferredInputUniformInterfaceCreateInfo = { };
	DeferredInputUniformInterfaceCreateInfo.Flags = 0; // No flags
	DeferredInputUniformInterfaceCreateInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
	DeferredInputUniformInterfaceCreateInfo.Format = DepthFormat;
	DeferredInputUniformInterfaceCreateInfo.Components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	DeferredInputUniformInterfaceCreateInfo.Components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	DeferredInputUniformInterfaceCreateInfo.Components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	DeferredInputUniformInterfaceCreateInfo.Components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	DeferredInputUniformInterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	DeferredInputUniformInterfaceCreateInfo.SubresourceRange.baseMipLevel = 0;
	DeferredInputUniformInterfaceCreateInfo.SubresourceRange.levelCount = 1;
	DeferredInputUniformInterfaceCreateInfo.SubresourceRange.baseArrayLayer = 0;
	DeferredInputUniformInterfaceCreateInfo.SubresourceRange.layerCount = 1;

	DeferredInputUniformColorInterfaceCreateInfo = DeferredInputUniformInterfaceCreateInfo;
	DeferredInputUniformColorInterfaceCreateInfo.Format = ColorFormat;
	DeferredInputUniformColorInterfaceCreateInfo.SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	// MAIN RENDERPASS
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

	// Subpass dependencies
	// Transition must happen after...
	MainPassSubpassDependencies[SubpassIndex::MainSubpass].srcSubpass = VK_SUBPASS_EXTERNAL;
	MainPassSubpassDependencies[SubpassIndex::MainSubpass].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	MainPassSubpassDependencies[SubpassIndex::MainSubpass].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	// But must happen before...
	MainPassSubpassDependencies[SubpassIndex::MainSubpass].dstSubpass = SubpassIndex::DeferredSubpass;
	MainPassSubpassDependencies[SubpassIndex::MainSubpass].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	MainPassSubpassDependencies[SubpassIndex::MainSubpass].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	MainPassSubpassDependencies[SubpassIndex::MainSubpass].dependencyFlags = 0;

	// Transition must happen after...
	MainPassSubpassDependencies[SubpassIndex::DeferredSubpass].srcSubpass = SubpassIndex::MainSubpass;
	MainPassSubpassDependencies[SubpassIndex::DeferredSubpass].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	MainPassSubpassDependencies[SubpassIndex::DeferredSubpass].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// But must happen before...
	MainPassSubpassDependencies[SubpassIndex::DeferredSubpass].dstSubpass = SubpassIndex::DeferredSubpass;
	MainPassSubpassDependencies[SubpassIndex::DeferredSubpass].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	MainPassSubpassDependencies[SubpassIndex::DeferredSubpass].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	MainPassSubpassDependencies[SubpassIndex::DeferredSubpass].dependencyFlags = 0;

	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transition must happen after...
	MainPassSubpassDependencies[ExitDependenciesIndex].srcSubpass = SubpassIndex::DeferredSubpass;
	MainPassSubpassDependencies[ExitDependenciesIndex].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	MainPassSubpassDependencies[ExitDependenciesIndex].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// But must happen before...
	MainPassSubpassDependencies[ExitDependenciesIndex].dstSubpass = VK_SUBPASS_EXTERNAL;
	MainPassSubpassDependencies[ExitDependenciesIndex].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	MainPassSubpassDependencies[ExitDependenciesIndex].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	MainPassSubpassDependencies[ExitDependenciesIndex].dependencyFlags = 0;

	MainRenderPassSubpasses[MainSubpass].ColorAttachmentsReferences = &EntitySubpassColourAttachmentReference;
	MainRenderPassSubpasses[MainSubpass].ColorAttachmentsReferencesCount = 1;
	MainRenderPassSubpasses[MainSubpass].DepthAttachmentReferences = &EntitySubpassDepthAttachmentReference;
	MainRenderPassSubpasses[MainSubpass].SubpassName = MainSubpassName;

	MainRenderPassSubpasses[DeferredSubpass].ColorAttachmentsReferences = &SwapchainColorAttachmentReference;
	MainRenderPassSubpasses[DeferredSubpass].ColorAttachmentsReferencesCount = 1;
	MainRenderPassSubpasses[DeferredSubpass].InputAttachmentsReferences = DeferredSubpassInputReferences;
	MainRenderPassSubpasses[DeferredSubpass].InputAttachmentsReferencesCount = InputReferencesCount;
	MainRenderPassSubpasses[DeferredSubpass].SubpassName = DeferredSubpassName;

	MainRenderPassSettings.AttachmentDescriptions = MainPathAttachmentDescriptions;
	MainRenderPassSettings.AttachmentDescriptionsCount = MainPathAttachmentDescriptionsCount;
	MainRenderPassSettings.SubpassesSettings = MainRenderPassSubpasses;
	MainRenderPassSettings.SubpassSettingsCount = Count;
	MainRenderPassSettings.RenderPassName = MainRenderPassName;
	MainRenderPassSettings.SubpassDependencies = MainPassSubpassDependencies;
	MainRenderPassSettings.SubpassDependenciesCount = MainPassSubpassDependenciesCount;

	// DEPTH RENDERPASS
	const u32 DepthSubpassAttachmentIndex = 0;

	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex] = { };
	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].format = DepthFormat;
	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	DepthPassAttachmentDescriptions[DepthSubpassAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	DepthSubpassDepthAttachmentReference.attachment = DepthSubpassAttachmentIndex;
	DepthSubpassDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Subpass dependencies
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

	DepthSubpasses[Depth].DepthAttachmentReferences = &DepthSubpassDepthAttachmentReference;
	DepthSubpasses[Depth].SubpassName = DepthSubpassName;

	DepthRenderPassSettings.AttachmentDescriptions = DepthPassAttachmentDescriptions;
	DepthRenderPassSettings.AttachmentDescriptionsCount = DepthPassAttachmentDescriptionsCount;
	DepthRenderPassSettings.SubpassesSettings = DepthSubpasses;
	DepthRenderPassSettings.SubpassSettingsCount = Subpasses_Count;
	DepthRenderPassSettings.RenderPassName = DepthRenderPassName;
	DepthRenderPassSettings.SubpassDependencies = DepthPassSubpassDependencies;
	DepthRenderPassSettings.SubpassDependenciesCount = DepthPassSubpassDependenciesCount;

	// Dynamic states TODO
// Use vkCmdSetViewport(Commandbuffer, 0, 1, &Viewport) and vkCmdSetScissor(Commandbuffer, 0, 1, &Scissor)
//VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

//VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
//DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//DynamicStateCreateInfo.dynamicStateCount = static_cast<u32>(2);
//DynamicStateCreateInfo.pDynamicStates = DynamicStates;

	// MainPipeline
	EntityPipelineSettings.PipelineName = EntityPipelineName;
	// Rasterizer
	EntityPipelineSettings.Extent = MainScreenExtent;
	EntityPipelineSettings.DepthClampEnable = VK_FALSE;
	EntityPipelineSettings.RasterizerDiscardEnable = VK_FALSE;
	EntityPipelineSettings.PolygonMode = VK_POLYGON_MODE_FILL;
	EntityPipelineSettings.LineWidth = 1.0f;
	EntityPipelineSettings.CullMode = VK_CULL_MODE_BACK_BIT;
	EntityPipelineSettings.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	EntityPipelineSettings.DepthBiasEnable = VK_FALSE;
	// Multisampling
	EntityPipelineSettings.BlendEnable = VK_TRUE;
	EntityPipelineSettings.LogicOpEnable = VK_FALSE;
	EntityPipelineSettings.AttachmentCount = 1;
	EntityPipelineSettings.ColorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	EntityPipelineSettings.SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	EntityPipelineSettings.DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	EntityPipelineSettings.ColorBlendOp = VK_BLEND_OP_ADD;
	EntityPipelineSettings.SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	EntityPipelineSettings.DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	EntityPipelineSettings.AlphaBlendOp = VK_BLEND_OP_ADD;
	// Depth testing
	EntityPipelineSettings.DepthTestEnable = VK_TRUE;
	EntityPipelineSettings.DepthWriteEnable = VK_TRUE;
	EntityPipelineSettings.DepthCompareOp = VK_COMPARE_OP_LESS;
	EntityPipelineSettings.DepthBoundsTestEnable = VK_FALSE;
	EntityPipelineSettings.StencilTestEnable = VK_FALSE;

	// TerrainPipeline
	TerrainPipelineSettings.PipelineName = TerrainPipelineName;
	// Rasterizer
	TerrainPipelineSettings.Extent = MainScreenExtent;
	TerrainPipelineSettings.DepthClampEnable = VK_FALSE;
	TerrainPipelineSettings.RasterizerDiscardEnable = VK_FALSE;
	TerrainPipelineSettings.PolygonMode = VK_POLYGON_MODE_FILL;
	TerrainPipelineSettings.LineWidth = 1.0f;
	TerrainPipelineSettings.CullMode = VK_CULL_MODE_BACK_BIT;
	TerrainPipelineSettings.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	TerrainPipelineSettings.DepthBiasEnable = VK_FALSE;
	// Multisampling
	TerrainPipelineSettings.BlendEnable = VK_TRUE;
	TerrainPipelineSettings.LogicOpEnable = VK_FALSE;
	TerrainPipelineSettings.AttachmentCount = 1;
	TerrainPipelineSettings.ColorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	TerrainPipelineSettings.SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	TerrainPipelineSettings.DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	TerrainPipelineSettings.ColorBlendOp = VK_BLEND_OP_ADD;
	TerrainPipelineSettings.SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	TerrainPipelineSettings.DstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	TerrainPipelineSettings.AlphaBlendOp = VK_BLEND_OP_ADD;
	// Depth testing
	TerrainPipelineSettings.DepthTestEnable = VK_TRUE;
	TerrainPipelineSettings.DepthWriteEnable = VK_TRUE;
	TerrainPipelineSettings.DepthCompareOp = VK_COMPARE_OP_LESS;
	TerrainPipelineSettings.DepthBoundsTestEnable = VK_FALSE;
	TerrainPipelineSettings.StencilTestEnable = VK_FALSE;

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
	EntityVertexInput.VertexInputBinding[0].InputAttributes[0] = { PositionAttributeName, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EntityVertex, Position) };
	EntityVertexInput.VertexInputBinding[0].InputAttributes[1] = { ColorAttributeName, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EntityVertex, Color) };
	EntityVertexInput.VertexInputBinding[0].InputAttributes[2] = { TextureCoordsAttributeName, VK_FORMAT_R32G32_SFLOAT, offsetof(EntityVertex, TextureCoords) };
	EntityVertexInput.VertexInputBinding[0].InputAttributes[3] = { NormalAttributeName, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EntityVertex, Normal) };
	EntityVertexInput.VertexInputBinding[0].InputAttributesCount = 4;
	EntityVertexInput.VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	EntityVertexInput.VertexInputBinding[0].Stride = sizeof(EntityVertex);
	EntityVertexInput.VertexInputBinding[0].VertexInputBindingName = EntityVertexInputName;
	EntityVertexInput.VertexInputBindingCount = 1;

	TerrainVertexInput.VertexInputBinding[0].InputAttributes[0] = { AltitudeAttributeName, VK_FORMAT_R32_SFLOAT, offsetof(TerrainVertex, Altitude) };
	TerrainVertexInput.VertexInputBinding[0].InputAttributesCount = 1;
	TerrainVertexInput.VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	TerrainVertexInput.VertexInputBinding[0].Stride = sizeof(TerrainVertex);
	TerrainVertexInput.VertexInputBinding[0].VertexInputBindingName = TerrainVertexInputName;
	TerrainVertexInput.VertexInputBindingCount = 1;

	SkyBoxVertexInput.VertexInputBinding[0].InputAttributes[0] = { PositionAttributeName, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkyBoxVertex, Position) };
	SkyBoxVertexInput.VertexInputBinding[0].InputAttributesCount = 1;
	SkyBoxVertexInput.VertexInputBinding[0].InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	SkyBoxVertexInput.VertexInputBinding[0].Stride = sizeof(SkyBoxVertex);
	SkyBoxVertexInput.VertexInputBinding[0].VertexInputBindingName = SkyBoxVertexInputName;
	SkyBoxVertexInput.VertexInputBindingCount = 1;

	DepthVertexInput = EntityVertexInput;
	DepthVertexInput.VertexInputBinding[0].InputAttributesCount = 1;
}
