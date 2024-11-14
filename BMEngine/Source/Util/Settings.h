#pragma once

#include "BMR/BMRInterfaceTypes.h"

void LoadSettings(u32 WindowWidth, u32 WindowHeight);

struct EntityVertex
{
	glm::vec3 Position;
	glm::vec3 Color;
	glm::vec2 TextureCoords;
	glm::vec3 Normal;
};

struct TerrainVertex
{
	f32 Altitude;
};

struct SkyBoxVertex
{
	glm::vec3 Position;
};

extern BMR::BMRVertexInput EntityVertexInput;
extern BMR::BMRVertexInput TerrainVertexInput;
extern BMR::BMRVertexInput SkyBoxVertexInput;
extern BMR::BMRVertexInput DepthVertexInput;

extern VkExtent2D MainScreenExtent;
extern VkExtent2D DepthViewportExtent;

extern VkFormat ColorFormat;
extern VkFormat DepthFormat;

extern BMR::BMRPipelineSettings EntityPipelineSettings;
extern BMR::BMRPipelineSettings TerrainPipelineSettings;
extern BMR::BMRPipelineSettings DeferredPipelineSettings;
extern BMR::BMRPipelineSettings SkyBoxPipelineSettings;
extern BMR::BMRPipelineSettings DepthPipelineSettings;

extern BMR::BMRRenderPassSettings MainRenderPassSettings;
extern BMR::BMRRenderPassSettings DepthRenderPassSettings;
