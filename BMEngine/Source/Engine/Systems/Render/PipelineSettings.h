#pragma once

#include <RenderInterface.h>
#include <Util/Settings.h>

BmRender_PipelineSettings GetStaticPipelineDescription()
{
	BmRender_PipelineSettings PipelineDesc = {};
	PipelineDesc.Extent = MainScreenExtent;

	PipelineDesc.RasterizationState = {};
	PipelineDesc.RasterizationState.DepthClampEnable = false;
	PipelineDesc.RasterizationState.RasterizerDiscardEnable = false;
	PipelineDesc.RasterizationState.PolygonMode = BmRender_PolygonMode::Fill;
	PipelineDesc.RasterizationState.LineWidth = 1.0f;
	PipelineDesc.RasterizationState.CullMode = BmRender_CullModeFlags::Back;
	PipelineDesc.RasterizationState.FrontFace = BmRender_FrontFace::CounterClockwise;
	PipelineDesc.RasterizationState.DepthBiasEnable = false;

	PipelineDesc.ColorBlendState = {};
	PipelineDesc.ColorBlendState.LogicOpEnable = false;
	PipelineDesc.ColorBlendState.AttachmentCount = 1;

	PipelineDesc.ColorBlendAttachment = {};
	PipelineDesc.ColorBlendAttachment.ColorWriteMask = BmRender_ColorComponentFlags::RGBA;
	PipelineDesc.ColorBlendAttachment.BlendEnable = true;
	PipelineDesc.ColorBlendAttachment.SrcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
	PipelineDesc.ColorBlendAttachment.DstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
	PipelineDesc.ColorBlendAttachment.ColorBlendOp = BmRender_BlendOp::Add;
	PipelineDesc.ColorBlendAttachment.SrcAlphaBlendFactor = BmRender_BlendFactor::One;
	PipelineDesc.ColorBlendAttachment.DstAlphaBlendFactor = BmRender_BlendFactor::Zero;
	PipelineDesc.ColorBlendAttachment.AlphaBlendOp = BmRender_BlendOp::Add;

	PipelineDesc.DepthStencilState = {};
	PipelineDesc.DepthStencilState.DepthTestEnable = true;
	PipelineDesc.DepthStencilState.DepthWriteEnable = true;
	PipelineDesc.DepthStencilState.DepthCompareOp = BmRender_CompareOp::Less;
	PipelineDesc.DepthStencilState.DepthBoundsTestEnable = false;
	PipelineDesc.DepthStencilState.StencilTestEnable = false;

	PipelineDesc.MultisampleState = {};
	PipelineDesc.MultisampleState.SampleShadingEnable = false;

	PipelineDesc.InputAssemblyState = {};
	PipelineDesc.InputAssemblyState.Topology = BmRender_PrimitiveTopology::TriangleList;
	PipelineDesc.InputAssemblyState.PrimitiveRestartEnable = false;

	PipelineDesc.ViewportState = {};
	PipelineDesc.ViewportState.ViewportCount = 1;
	PipelineDesc.ViewportState.ScissorCount = 1;

	PipelineDesc.Viewport = {};
	PipelineDesc.Viewport.MinDepth = 0.0f;
	PipelineDesc.Viewport.MaxDepth = 1.0f;
	PipelineDesc.Viewport.X = 0.0f;
	PipelineDesc.Viewport.Y = 0.0f;
	PipelineDesc.Viewport.Width = static_cast<f32>(MainScreenExtent.Width);
	PipelineDesc.Viewport.Height = static_cast<f32>(MainScreenExtent.Height);

	PipelineDesc.Scissor = {};
	PipelineDesc.Scissor.Offset.X = 0;
	PipelineDesc.Scissor.Offset.Y = 0;
	PipelineDesc.Scissor.Extent.Width = MainScreenExtent.Width;
	PipelineDesc.Scissor.Extent.Height = MainScreenExtent.Height;

	return PipelineDesc;
}

BmRender_PipelineSettings GetDeferredPipelineDescription()
{
	BmRender_PipelineSettings PipelineDesc = {};
	PipelineDesc.Extent = MainScreenExtent;

	PipelineDesc.RasterizationState = {};
	PipelineDesc.RasterizationState.DepthClampEnable = false;
	PipelineDesc.RasterizationState.RasterizerDiscardEnable = false;
	PipelineDesc.RasterizationState.PolygonMode = BmRender_PolygonMode::Fill;
	PipelineDesc.RasterizationState.LineWidth = 1.0f;
	PipelineDesc.RasterizationState.CullMode = BmRender_CullModeFlags::None;
	PipelineDesc.RasterizationState.FrontFace = BmRender_FrontFace::CounterClockwise;
	PipelineDesc.RasterizationState.DepthBiasEnable = false;

	PipelineDesc.ColorBlendState = {};
	PipelineDesc.ColorBlendState.LogicOpEnable = false;
	PipelineDesc.ColorBlendState.AttachmentCount = 1;

	PipelineDesc.ColorBlendAttachment = {};
	PipelineDesc.ColorBlendAttachment.ColorWriteMask = BmRender_ColorComponentFlags::RGBA;
	PipelineDesc.ColorBlendAttachment.BlendEnable = false;
	PipelineDesc.ColorBlendAttachment.SrcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
	PipelineDesc.ColorBlendAttachment.DstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
	PipelineDesc.ColorBlendAttachment.ColorBlendOp = BmRender_BlendOp::Add;
	PipelineDesc.ColorBlendAttachment.SrcAlphaBlendFactor = BmRender_BlendFactor::One;
	PipelineDesc.ColorBlendAttachment.DstAlphaBlendFactor = BmRender_BlendFactor::Zero;
	PipelineDesc.ColorBlendAttachment.AlphaBlendOp = BmRender_BlendOp::Add;

	PipelineDesc.DepthStencilState = {};
	PipelineDesc.DepthStencilState.DepthTestEnable = false;
	PipelineDesc.DepthStencilState.DepthWriteEnable = false;
	PipelineDesc.DepthStencilState.DepthCompareOp = BmRender_CompareOp::Less;
	PipelineDesc.DepthStencilState.DepthBoundsTestEnable = false;
	PipelineDesc.DepthStencilState.StencilTestEnable = false;

	PipelineDesc.MultisampleState = {};
	PipelineDesc.MultisampleState.SampleShadingEnable = false;

	PipelineDesc.InputAssemblyState = {};
	PipelineDesc.InputAssemblyState.Topology = BmRender_PrimitiveTopology::TriangleList;
	PipelineDesc.InputAssemblyState.PrimitiveRestartEnable = false;

	PipelineDesc.ViewportState = {};
	PipelineDesc.ViewportState.ViewportCount = 1;
	PipelineDesc.ViewportState.ScissorCount = 1;

	PipelineDesc.Viewport = {};
	PipelineDesc.Viewport.MinDepth = 0.0f;
	PipelineDesc.Viewport.MaxDepth = 1.0f;
	PipelineDesc.Viewport.X = 0.0f;
	PipelineDesc.Viewport.Y = 0.0f;
	PipelineDesc.Viewport.Width = static_cast<f32>(MainScreenExtent.Width);
	PipelineDesc.Viewport.Height = static_cast<f32>(MainScreenExtent.Height);

	PipelineDesc.Scissor = {};
	PipelineDesc.Scissor.Offset.X = 0;
	PipelineDesc.Scissor.Offset.Y = 0;
	PipelineDesc.Scissor.Extent.Width = MainScreenExtent.Width;
	PipelineDesc.Scissor.Extent.Height = MainScreenExtent.Height;

	return PipelineDesc;
}

BmRender_PipelineSettings GetDepthPipelineDescription()
{
	BmRender_PipelineSettings PipelineDesc = {};
	PipelineDesc.Extent = DepthViewportExtent;

	PipelineDesc.RasterizationState = {};
	PipelineDesc.RasterizationState.DepthClampEnable = false;
	PipelineDesc.RasterizationState.RasterizerDiscardEnable = false;
	PipelineDesc.RasterizationState.PolygonMode = BmRender_PolygonMode::Fill;
	PipelineDesc.RasterizationState.LineWidth = 1.0f;
	PipelineDesc.RasterizationState.CullMode = BmRender_CullModeFlags::Back;
	PipelineDesc.RasterizationState.FrontFace = BmRender_FrontFace::CounterClockwise;
	PipelineDesc.RasterizationState.DepthBiasEnable = false;

	PipelineDesc.ColorBlendState = {};
	PipelineDesc.ColorBlendState.LogicOpEnable = false;
	PipelineDesc.ColorBlendState.AttachmentCount = 1;

	PipelineDesc.ColorBlendAttachment = {};
	PipelineDesc.ColorBlendAttachment.ColorWriteMask = BmRender_ColorComponentFlags::RGBA;
	PipelineDesc.ColorBlendAttachment.BlendEnable = true;
	PipelineDesc.ColorBlendAttachment.SrcColorBlendFactor = BmRender_BlendFactor::SrcAlpha;
	PipelineDesc.ColorBlendAttachment.DstColorBlendFactor = BmRender_BlendFactor::OneMinusSrcAlpha;
	PipelineDesc.ColorBlendAttachment.ColorBlendOp = BmRender_BlendOp::Add;
	PipelineDesc.ColorBlendAttachment.SrcAlphaBlendFactor = BmRender_BlendFactor::One;
	PipelineDesc.ColorBlendAttachment.DstAlphaBlendFactor = BmRender_BlendFactor::Zero;
	PipelineDesc.ColorBlendAttachment.AlphaBlendOp = BmRender_BlendOp::Add;

	PipelineDesc.DepthStencilState = {};
	PipelineDesc.DepthStencilState.DepthTestEnable = true;
	PipelineDesc.DepthStencilState.DepthWriteEnable = true;
	PipelineDesc.DepthStencilState.DepthCompareOp = BmRender_CompareOp::Less;
	PipelineDesc.DepthStencilState.DepthBoundsTestEnable = false;
	PipelineDesc.DepthStencilState.StencilTestEnable = false;

	PipelineDesc.MultisampleState = {};
	PipelineDesc.MultisampleState.SampleShadingEnable = false;

	PipelineDesc.InputAssemblyState = {};
	PipelineDesc.InputAssemblyState.Topology = BmRender_PrimitiveTopology::TriangleList;
	PipelineDesc.InputAssemblyState.PrimitiveRestartEnable = false;

	PipelineDesc.ViewportState = {};
	PipelineDesc.ViewportState.ViewportCount = 1;
	PipelineDesc.ViewportState.ScissorCount = 1;

	PipelineDesc.Viewport = {};
	PipelineDesc.Viewport.MinDepth = 0.0f;
	PipelineDesc.Viewport.MaxDepth = 1.0f;
	PipelineDesc.Viewport.X = 0.0f;
	PipelineDesc.Viewport.Y = 0.0f;
	PipelineDesc.Viewport.Width = static_cast<f32>(DepthViewportExtent.Width);
	PipelineDesc.Viewport.Height = static_cast<f32>(DepthViewportExtent.Height);

	PipelineDesc.Scissor = {};
	PipelineDesc.Scissor.Offset.X = 0;
	PipelineDesc.Scissor.Offset.Y = 0;
	PipelineDesc.Scissor.Extent.Width = DepthViewportExtent.Width;
	PipelineDesc.Scissor.Extent.Height = DepthViewportExtent.Height;

	return PipelineDesc;
}