#include "Settings.h"

VkExtent2D MainScreenExtent;
VkExtent2D DepthViewportExtent = { 1024, 1024 };

VkViewport MainViewport;
VkViewport DepthViewport;

BMR::BMRPipelineSettings EntityPipelineSettings;
BMR::BMRPipelineSettings TerrainPipelineSettings;
BMR::BMRPipelineSettings DeferredPipelineSettings;
BMR::BMRPipelineSettings SkyBoxPipelineSettings;
BMR::BMRPipelineSettings DepthPipelineSettings;

void LoadSettings(u32 WindowWidth, u32 WindowHeight)
{
	// Dynamic states TODO
// Use vkCmdSetViewport(Commandbuffer, 0, 1, &Viewport) and vkCmdSetScissor(Commandbuffer, 0, 1, &Scissor)
//VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

//VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {};
//DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//DynamicStateCreateInfo.dynamicStateCount = static_cast<u32>(2);
//DynamicStateCreateInfo.pDynamicStates = DynamicStates;

	MainScreenExtent = { WindowWidth, WindowHeight };

	MainViewport = { 0.0f, 0.0f, (float)MainScreenExtent.width, (float)MainScreenExtent.height, 0.0f, 1.0f };
	DepthViewport = { 0.0f, 0.0f, (float)DepthViewportExtent.width, (float)DepthViewportExtent.height, 0.0f, 1.0f };

	VkRect2D MainViewportScissor = { { 0, 0 }, { MainScreenExtent.width, MainScreenExtent.height } };
	VkRect2D DepthViewportScissor = { { 0, 0 }, { DepthViewportExtent.width, DepthViewportExtent.height } };

	// MainPipeline
	// Rasterizer
	EntityPipelineSettings.Viewport = MainViewport;
	EntityPipelineSettings.Scissor = MainViewportScissor;
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
	// Rasterizer
	TerrainPipelineSettings.Viewport = MainViewport;
	TerrainPipelineSettings.Scissor = MainViewportScissor;
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
	// Rasterizer
	DeferredPipelineSettings.Viewport = MainViewport;
	DeferredPipelineSettings.Scissor = MainViewportScissor;
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
	DeferredPipelineSettings.DepthBoundsTestEnable = VK_FALSE;
	DeferredPipelineSettings.StencilTestEnable = VK_FALSE;

	// SkyBoxPipelineSettings
	// Rasterizer
	SkyBoxPipelineSettings.Viewport = MainViewport;
	SkyBoxPipelineSettings.Scissor = MainViewportScissor;
	SkyBoxPipelineSettings.DepthClampEnable = VK_FALSE;
	SkyBoxPipelineSettings.RasterizerDiscardEnable = VK_FALSE;
	SkyBoxPipelineSettings.PolygonMode = VK_POLYGON_MODE_FILL;
	SkyBoxPipelineSettings.LineWidth = 1.0f;
	SkyBoxPipelineSettings.CullMode = VK_CULL_MODE_BACK_BIT;
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
	// Rasterizer
	DepthPipelineSettings.Viewport = DepthViewport;
	DepthPipelineSettings.Scissor = DepthViewportScissor;
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
