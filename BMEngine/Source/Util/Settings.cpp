#include "Settings.h"
#include "Engine/Systems/Render/StaticMeshRender.h"

VkExtent2D MainScreenExtent;
VkExtent2D DepthViewportExtent = { 1024, 1024 };

VkFormat ColorFormat = VK_FORMAT_R8G8B8A8_UNORM; // Todo: check if VK_FORMAT_R8G8B8A8_UNORM supported
VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

VulkanInterface::PipelineSettings DeferredPipelineSettings;
VulkanInterface::PipelineSettings SkyBoxPipelineSettings;
VulkanInterface::PipelineSettings DepthPipelineSettings;

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
}
