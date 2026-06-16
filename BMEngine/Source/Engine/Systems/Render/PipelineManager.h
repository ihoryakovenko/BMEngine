#pragma once

#include <Engine/Generated/ShaderRegistry.generated.h>
#include <ShortTypes.h>

#include "PipelineMetadata.h"
#include <RenderInterface.h>

void PipelineManger_Init(bool EnableLiveShaders);
void PipelineManager_DeInit();
void PipelineManager_Update();

void PipelineManager_CreatePipelineLayout(PipelineNames Name, const BmRender_DescriptorSetLayout* SetLayouts, u32 SetLayoutsCount,
	const BmRender_PushConstant* PushConstants, u32 PushConstantsCount);

void PipelineManager_CreateGraphicsPipeline(PipelineNames Name, const BmRender_PipelineSettings* Settings, const AttachmentData* ResourceInfo);
void PipelineManager_CreateComputePipeline(PipelineNames Name);

void PipelineManager_BindPipeline(BmRender_CommandBuffer CmdBuffer, PipelineNames Name);
void PipelineManager_RecordBindDescriptorSets(BmRender_CommandBuffer CommandBuffer, PipelineNames Name, u32 FirstSet, u32 DescriptorSetCount, const BmRender_DescriptorSet* pDescriptorSets, u32 DynamicOffsetCount, const u32* pDynamicOffsets);
