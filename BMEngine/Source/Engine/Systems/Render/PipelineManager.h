#pragma once

#include <Engine/Generated/ShaderRegistry.generated.h>
#include <ShortTypes.h>

#include "PipelineMetadata.h"
#include <RenderInterface.h>

void PipelineManger_Init(bool EnableLiveShaders);
void PipelineManager_DeInit();
void PipelineManager_Update();

void PipelineManager_CreatePipelineLayout(PipelineNames Name, const BmRender_DescriptorSetLayout* SetLayouts, u32 SetLayoutsCount,
	const BmRender_PushConstant* PushConstants, u32 PushConstantsCount, BmRender_PipelineType PipelineType);

void PipelineManager_CreatePipeline(PipelineNames Name, const BmRender_PipelineSettings* Settings, const AttachmentData* ResourceInfo);

BmRender_Pipeline  PipelineManager_GetPipeline(PipelineNames Name);
