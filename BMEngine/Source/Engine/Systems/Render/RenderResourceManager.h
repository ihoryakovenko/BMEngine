#pragma once

#include <Engine/Generated/ShaderRegistry.generated.h>
#include <ShortTypes.h>

#include "PipelineMetadata.h"
#include <RenderInterface.h>

void PipelineManger_Init(bool EnableLiveShaders);
void RenderResourceManager_DeInit();
void RenderResourceManager_Update();

void RenderResourceManager_CreatePipelineLayout(PipelineNames Name, const BmRender_DescriptorSetLayout* SetLayouts, u32 SetLayoutsCount,
	const BmRender_PushConstant* PushConstants, u32 PushConstantsCount);

void RenderResourceManager_CreateGraphicsPipeline(PipelineNames Name, const BmRender_PipelineSettings* Settings, const AttachmentData* ResourceInfo);
void RenderResourceManager_CreateComputePipeline(PipelineNames Name);

void RenderResourceManager_BindPipeline(BmRender_CommandBuffer CmdBuffer, PipelineNames Name);
void RenderResourceManager_RecordBindDescriptorSets(BmRender_CommandBuffer CommandBuffer, PipelineNames Name, u32 FirstSet, u32 DescriptorSetCount, const BmRender_DescriptorSet* pDescriptorSets, u32 DynamicOffsetCount, const u32* pDynamicOffsets);

void RenderResourceManager_RecordPushConstants(BmRender_CommandBuffer CommandBuffer, PipelineNames Name, BmRender_DescriptorShaderStage StageFlags, u32 Offset, u32 Size, const void* pValues);