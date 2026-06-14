#pragma once

#include <RenderInterface.h>
#include <Util/Settings.h>

inline BmRender_PipelineSettings GetStaticPipelineDescription()
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

inline BmRender_PipelineSettings GetDeferredPipelineDescription()
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

inline BmRender_PipelineSettings GetDepthPipelineDescription()
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

inline BmRHI_SamplerDescription GetShadowMapSamplerDescription()
{
	BmRHI_SamplerDescription ShadowMapSamplerDescription;
	ShadowMapSamplerDescription.MagFilter = BmRender_Filter::Linear;
	ShadowMapSamplerDescription.MinFilter = BmRender_Filter::Linear;
	ShadowMapSamplerDescription.MipmapMode = BmRender_SamplerMipmapMode::Linear;
	ShadowMapSamplerDescription.AddressModeU = BmRender_SamplerAddressMode::ClampToBorder;
	ShadowMapSamplerDescription.AddressModeV = BmRender_SamplerAddressMode::ClampToBorder;
	ShadowMapSamplerDescription.AddressModeW = BmRender_SamplerAddressMode::ClampToBorder;
	ShadowMapSamplerDescription.MipLodBias = 0.0f;
	ShadowMapSamplerDescription.AnisotropyEnable = true;
	ShadowMapSamplerDescription.MaxAnisotropy = 1.0f;
	ShadowMapSamplerDescription.CompareEnable = false;
	ShadowMapSamplerDescription.CompareOp = BmRender_CompareOp::Never;
	ShadowMapSamplerDescription.MinLod = 0.0f;
	ShadowMapSamplerDescription.MaxLod = 0.0f;
	ShadowMapSamplerDescription.BorderColor = BmRender_BorderColor::IntOpaqueBlack;
	ShadowMapSamplerDescription.UnnormalizedCoordinates = false;
	return ShadowMapSamplerDescription;
}

inline BmRHI_SamplerDescription GetDiffuseTextureSamplerDescription()
{
	BmRHI_SamplerDescription DiffuseTextureSamplerDescription;
	DiffuseTextureSamplerDescription.MagFilter = BmRender_Filter::Linear;
	DiffuseTextureSamplerDescription.MinFilter = BmRender_Filter::Linear;
	DiffuseTextureSamplerDescription.MipmapMode = BmRender_SamplerMipmapMode::Linear;
	DiffuseTextureSamplerDescription.AddressModeU = BmRender_SamplerAddressMode::ClampToEdge;
	DiffuseTextureSamplerDescription.AddressModeV = BmRender_SamplerAddressMode::ClampToEdge;
	DiffuseTextureSamplerDescription.AddressModeW = BmRender_SamplerAddressMode::ClampToEdge;
	DiffuseTextureSamplerDescription.MipLodBias = 0.0f;
	DiffuseTextureSamplerDescription.AnisotropyEnable = true;
	DiffuseTextureSamplerDescription.MaxAnisotropy = 16.0f;
	DiffuseTextureSamplerDescription.CompareEnable = false;
	DiffuseTextureSamplerDescription.CompareOp = BmRender_CompareOp::Never;
	DiffuseTextureSamplerDescription.MinLod = 0.0f;
	DiffuseTextureSamplerDescription.MaxLod = 0.0f;
	DiffuseTextureSamplerDescription.BorderColor = BmRender_BorderColor::IntOpaqueBlack;
	DiffuseTextureSamplerDescription.UnnormalizedCoordinates = false;
	return DiffuseTextureSamplerDescription;
}

inline BmRHI_SamplerDescription GetColorAttachmentSamplerDescription()
{
	BmRHI_SamplerDescription ColorAttachmentSamplerDescription;
	ColorAttachmentSamplerDescription.MagFilter = BmRender_Filter::Linear;
	ColorAttachmentSamplerDescription.MinFilter = BmRender_Filter::Linear;
	ColorAttachmentSamplerDescription.MipmapMode = BmRender_SamplerMipmapMode::Linear;
	ColorAttachmentSamplerDescription.AddressModeU = BmRender_SamplerAddressMode::ClampToEdge;
	ColorAttachmentSamplerDescription.AddressModeV = BmRender_SamplerAddressMode::ClampToEdge;
	ColorAttachmentSamplerDescription.AddressModeW = BmRender_SamplerAddressMode::ClampToEdge;
	ColorAttachmentSamplerDescription.MipLodBias = 0.0f;
	ColorAttachmentSamplerDescription.AnisotropyEnable = true;
	ColorAttachmentSamplerDescription.MaxAnisotropy = 16.0f;
	ColorAttachmentSamplerDescription.CompareEnable = false;
	ColorAttachmentSamplerDescription.CompareOp = BmRender_CompareOp::Never;
	ColorAttachmentSamplerDescription.MinLod = 0.0f;
	ColorAttachmentSamplerDescription.MaxLod = 0.0f;
	ColorAttachmentSamplerDescription.BorderColor = BmRender_BorderColor::IntOpaqueBlack;
	ColorAttachmentSamplerDescription.UnnormalizedCoordinates = false;
	return ColorAttachmentSamplerDescription;
}

inline BmRHI_SamplerDescription GetDepthAttachmentSamplerDescription()
{
	BmRHI_SamplerDescription DepthAttachmentSamplerDescription;
	DepthAttachmentSamplerDescription.MagFilter = BmRender_Filter::Nearest;
	DepthAttachmentSamplerDescription.MinFilter = BmRender_Filter::Nearest;
	DepthAttachmentSamplerDescription.MipmapMode = BmRender_SamplerMipmapMode::Nearest;
	DepthAttachmentSamplerDescription.AddressModeU = BmRender_SamplerAddressMode::ClampToEdge;
	DepthAttachmentSamplerDescription.AddressModeV = BmRender_SamplerAddressMode::ClampToEdge;
	DepthAttachmentSamplerDescription.AddressModeW = BmRender_SamplerAddressMode::ClampToEdge;
	DepthAttachmentSamplerDescription.MipLodBias = 0.0f;
	DepthAttachmentSamplerDescription.AnisotropyEnable = false;
	DepthAttachmentSamplerDescription.MaxAnisotropy = 16.0f;
	DepthAttachmentSamplerDescription.CompareEnable = false;
	DepthAttachmentSamplerDescription.CompareOp = BmRender_CompareOp::Never;
	DepthAttachmentSamplerDescription.MinLod = 0.0f;
	DepthAttachmentSamplerDescription.MaxLod = 0.0f;
	DepthAttachmentSamplerDescription.BorderColor = BmRender_BorderColor::IntOpaqueBlack;
	DepthAttachmentSamplerDescription.UnnormalizedCoordinates = false;
	return DepthAttachmentSamplerDescription;
}