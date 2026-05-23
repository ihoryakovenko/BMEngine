#pragma once

#include <Engine/Generated/ShaderRegistry.generated.h>

#include "PipelineMetadata.h"

void PipelineManger_Init();
void PipelineManager_DeInit();

const BmRender_Shader PipelineManager_GetShader(PipelineNames Name);
const Metadata_Pipeline* PipelineManager_GetPipelineMetadata(PipelineNames Name);
u32  PipelineManager_GetStageCout(PipelineNames Name);
