#include "Settings.h"

BMR::BMRExtent2D MainScreenExtent;
BMR::BMRExtent2D DepthViewportExtent = { 1024, 1024 };

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

	// MainPipeline
	// Rasterizer
	EntityPipelineSettings.Extent = MainScreenExtent;
	EntityPipelineSettings.DepthClampEnable = false;
	EntityPipelineSettings.RasterizerDiscardEnable = false;
	EntityPipelineSettings.PolygonMode = BMR::BMRPolygonMode::Fill;
	EntityPipelineSettings.LineWidth = 1.0f;
	EntityPipelineSettings.CullMode = BMR::BMRCullMode::Back;
	EntityPipelineSettings.FrontFace = BMR::BMRFrontFace::CounterClockwise;
	EntityPipelineSettings.DepthBiasEnable = false;
	// Multisampling
	EntityPipelineSettings.BlendEnable = true;
	EntityPipelineSettings.LogicOpEnable = false;
	EntityPipelineSettings.AttachmentCount = 1;
	EntityPipelineSettings.ColorWriteMask = BMR::BMRColorComponentFlagBits::All;
	EntityPipelineSettings.SrcColorBlendFactor = BMR::BMRBlendFactor::SrcAlpha;
	EntityPipelineSettings.DstColorBlendFactor = BMR::BMRBlendFactor::OneMinusSrcAlpha;
	EntityPipelineSettings.ColorBlendOp = BMR::BMRBlendOp::Add;
	EntityPipelineSettings.SrcAlphaBlendFactor = BMR::BMRBlendFactor::One;
	EntityPipelineSettings.DstAlphaBlendFactor = BMR::BMRBlendFactor::Zero;
	EntityPipelineSettings.AlphaBlendOp = BMR::BMRBlendOp::Add;
	// Depth testing
	EntityPipelineSettings.DepthTestEnable = true;
	EntityPipelineSettings.DepthWriteEnable = true;
	EntityPipelineSettings.DepthCompareOp = BMR::BMRCompareOp::Less;
	EntityPipelineSettings.DepthBoundsTestEnable = false;
	EntityPipelineSettings.StencilTestEnable = false;

	// TerrainPipeline
	// Rasterizer
	TerrainPipelineSettings.Extent = MainScreenExtent;
	TerrainPipelineSettings.DepthClampEnable = false;
	TerrainPipelineSettings.RasterizerDiscardEnable = false;
	TerrainPipelineSettings.PolygonMode = BMR::BMRPolygonMode::Fill;
	TerrainPipelineSettings.LineWidth = 1.0f;
	TerrainPipelineSettings.CullMode = BMR::BMRCullMode::Back;
	TerrainPipelineSettings.FrontFace = BMR::BMRFrontFace::CounterClockwise;
	TerrainPipelineSettings.DepthBiasEnable = false;
	// Multisampling
	TerrainPipelineSettings.BlendEnable = true;
	TerrainPipelineSettings.LogicOpEnable = false;
	TerrainPipelineSettings.AttachmentCount = 1;
	TerrainPipelineSettings.ColorWriteMask = BMR::BMRColorComponentFlagBits::All;
	TerrainPipelineSettings.SrcColorBlendFactor = BMR::BMRBlendFactor::SrcAlpha;
	TerrainPipelineSettings.DstColorBlendFactor = BMR::BMRBlendFactor::OneMinusSrcAlpha;
	TerrainPipelineSettings.ColorBlendOp = BMR::BMRBlendOp::Add;
	TerrainPipelineSettings.SrcAlphaBlendFactor = BMR::BMRBlendFactor::One;
	TerrainPipelineSettings.DstAlphaBlendFactor = BMR::BMRBlendFactor::Zero;
	TerrainPipelineSettings.AlphaBlendOp = BMR::BMRBlendOp::Add;
	// Depth testing
	TerrainPipelineSettings.DepthTestEnable = true;
	TerrainPipelineSettings.DepthWriteEnable = true;
	TerrainPipelineSettings.DepthCompareOp = BMR::BMRCompareOp::Less;
	TerrainPipelineSettings.DepthBoundsTestEnable = false;
	TerrainPipelineSettings.StencilTestEnable = false;

	// DeferredPipelineSettings
	// Rasterizer
	DeferredPipelineSettings.Extent = MainScreenExtent;
	DeferredPipelineSettings.DepthClampEnable = false;
	DeferredPipelineSettings.RasterizerDiscardEnable = false;
	DeferredPipelineSettings.PolygonMode = BMR::BMRPolygonMode::Fill;
	DeferredPipelineSettings.LineWidth = 1.0f;
	DeferredPipelineSettings.CullMode = BMR::BMRCullMode::None;
	DeferredPipelineSettings.FrontFace = BMR::BMRFrontFace::CounterClockwise;
	DeferredPipelineSettings.DepthBiasEnable = false;
	// Multisampling
	DeferredPipelineSettings.BlendEnable = false;
	DeferredPipelineSettings.LogicOpEnable = false;
	DeferredPipelineSettings.AttachmentCount = 1;
	DeferredPipelineSettings.ColorWriteMask = BMR::BMRColorComponentFlagBits::All;
	DeferredPipelineSettings.SrcColorBlendFactor = BMR::BMRBlendFactor::SrcAlpha;
	DeferredPipelineSettings.DstColorBlendFactor = BMR::BMRBlendFactor::OneMinusSrcAlpha;
	DeferredPipelineSettings.ColorBlendOp = BMR::BMRBlendOp::Add;
	DeferredPipelineSettings.SrcAlphaBlendFactor = BMR::BMRBlendFactor::One;
	DeferredPipelineSettings.DstAlphaBlendFactor = BMR::BMRBlendFactor::Zero;
	DeferredPipelineSettings.AlphaBlendOp = BMR::BMRBlendOp::Add;
	// Depth testing
	DeferredPipelineSettings.DepthTestEnable = false;
	DeferredPipelineSettings.DepthWriteEnable = false;
	DeferredPipelineSettings.DepthCompareOp = BMR::BMRCompareOp::Less;
	DeferredPipelineSettings.DepthBoundsTestEnable = false;
	DeferredPipelineSettings.StencilTestEnable = false;

	// SkyBoxPipelineSettings
	// Rasterizer
	SkyBoxPipelineSettings.Extent = MainScreenExtent;
	SkyBoxPipelineSettings.DepthClampEnable = false;
	SkyBoxPipelineSettings.RasterizerDiscardEnable = false;
	SkyBoxPipelineSettings.PolygonMode = BMR::BMRPolygonMode::Fill;
	SkyBoxPipelineSettings.LineWidth = 1.0f;
	SkyBoxPipelineSettings.CullMode = BMR::BMRCullMode::Front;
	SkyBoxPipelineSettings.FrontFace = BMR::BMRFrontFace::CounterClockwise;
	SkyBoxPipelineSettings.DepthBiasEnable = false;
	// Multisampling
	SkyBoxPipelineSettings.BlendEnable = true;
	SkyBoxPipelineSettings.LogicOpEnable = false;
	SkyBoxPipelineSettings.AttachmentCount = 1;
	SkyBoxPipelineSettings.ColorWriteMask = BMR::BMRColorComponentFlagBits::All;
	SkyBoxPipelineSettings.SrcColorBlendFactor = BMR::BMRBlendFactor::SrcAlpha;
	SkyBoxPipelineSettings.DstColorBlendFactor = BMR::BMRBlendFactor::OneMinusSrcAlpha;
	SkyBoxPipelineSettings.ColorBlendOp = BMR::BMRBlendOp::Add;
	SkyBoxPipelineSettings.SrcAlphaBlendFactor = BMR::BMRBlendFactor::One;
	SkyBoxPipelineSettings.DstAlphaBlendFactor = BMR::BMRBlendFactor::Zero;
	SkyBoxPipelineSettings.AlphaBlendOp = BMR::BMRBlendOp::Add;
	// Depth testing
	SkyBoxPipelineSettings.DepthTestEnable = true;
	SkyBoxPipelineSettings.DepthWriteEnable = false;
	SkyBoxPipelineSettings.DepthCompareOp = BMR::BMRCompareOp::LessOrEqual;
	SkyBoxPipelineSettings.DepthBoundsTestEnable = false;
	SkyBoxPipelineSettings.StencilTestEnable = false;

	// DepthPipelineSettings
	// Rasterizer
	DepthPipelineSettings.Extent = DepthViewportExtent;
	DepthPipelineSettings.DepthClampEnable = false;
	DepthPipelineSettings.RasterizerDiscardEnable = false;
	DepthPipelineSettings.PolygonMode = BMR::BMRPolygonMode::Fill;
	DepthPipelineSettings.LineWidth = 1.0f;
	DepthPipelineSettings.CullMode = BMR::BMRCullMode::Back;
	DepthPipelineSettings.FrontFace = BMR::BMRFrontFace::CounterClockwise;
	DepthPipelineSettings.DepthBiasEnable = false;
	// Multisampling
	DepthPipelineSettings.BlendEnable = true;
	DepthPipelineSettings.LogicOpEnable = false;
	DepthPipelineSettings.AttachmentCount = 1;
	DepthPipelineSettings.ColorWriteMask = BMR::BMRColorComponentFlagBits::All;
	DepthPipelineSettings.SrcColorBlendFactor = BMR::BMRBlendFactor::SrcAlpha;
	DepthPipelineSettings.DstColorBlendFactor = BMR::BMRBlendFactor::OneMinusSrcAlpha;
	DepthPipelineSettings.ColorBlendOp = BMR::BMRBlendOp::Add;
	DepthPipelineSettings.SrcAlphaBlendFactor = BMR::BMRBlendFactor::One;
	DepthPipelineSettings.DstAlphaBlendFactor = BMR::BMRBlendFactor::Zero;
	DepthPipelineSettings.AlphaBlendOp = BMR::BMRBlendOp::Add;
	// Depth testing
	DepthPipelineSettings.DepthTestEnable = true;
	DepthPipelineSettings.DepthWriteEnable = true;
	DepthPipelineSettings.DepthCompareOp = BMR::BMRCompareOp::Less;
	DepthPipelineSettings.DepthBoundsTestEnable = false;
	DepthPipelineSettings.StencilTestEnable = false;
}
