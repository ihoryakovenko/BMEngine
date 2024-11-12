#pragma once

#include "BMR/BMRInterfaceTypes.h"

void LoadSettings(u32 WindowWidth, u32 WindowHeight);

extern BMR::BMRExtent2D MainScreenExtent;
extern BMR::BMRExtent2D DepthViewportExtent;

extern BMR::BMRPipelineSettings EntityPipelineSettings;
extern BMR::BMRPipelineSettings TerrainPipelineSettings;
extern BMR::BMRPipelineSettings DeferredPipelineSettings;
extern BMR::BMRPipelineSettings SkyBoxPipelineSettings;
extern BMR::BMRPipelineSettings DepthPipelineSettings;
