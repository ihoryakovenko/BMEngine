#pragma once

#include <vulkan/vulkan.h>

#include "BMR/BMRInterfaceTypes.h"

void LoadSettings(u32 WindowWidth, u32 WindowHeight);

extern VkExtent2D MainScreenExtent;
extern VkViewport MainViewport;

extern VkExtent2D DepthViewportExtent;
extern VkViewport DepthViewport;

extern BMR::BMRPipelineSettings EntityPipelineSettings;
extern BMR::BMRPipelineSettings TerrainPipelineSettings;
extern BMR::BMRPipelineSettings DeferredPipelineSettings;
extern BMR::BMRPipelineSettings SkyBoxPipelineSettings;
extern BMR::BMRPipelineSettings DepthPipelineSettings;
