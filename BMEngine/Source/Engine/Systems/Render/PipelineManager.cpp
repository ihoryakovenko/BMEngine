#include "PipelineManager.h"

#include <Util/Util.h>

#include <vector>

static BmRender_Shader Shaders[(u32)PipelineNames::MAX_VALUE];
static bool Initialized;

void PipelineManger_Init()
{
	std::vector<char> ShaderCode;

	for (u32 i = 0; i < (u32)PipelineNames::MAX_VALUE; ++i)
	{
		if (Util::OpenAndReadFileFull(Metadata_Pipelines[i].FilePath, ShaderCode, "rb"))
		{
			BmRender_ShaderDescription ShaderDesc = {};
			ShaderDesc.Code = reinterpret_cast<const u32*>(ShaderCode.data());
			ShaderDesc.CodeSize = ShaderCode.size();
			Shaders[i] = BmRender_CreateShader(&ShaderDesc);
		}
		else
		{
			assert(false);
		}
	}

	Initialized = true;
}

void PipelineManager_DeInit()
{
	assert(Initialized);
	for (u32 i = 0; i < (u32)PipelineNames::MAX_VALUE; ++i)
	{
		BmRender_DestroyShader(Shaders[i]);
	}

	Initialized = false;
}

const BmRender_Shader PipelineManager_GetShader(PipelineNames Name)
{
	assert(Initialized);
	return Shaders[(u32)Name];
}

const Metadata_Pipeline* PipelineManager_GetPipelineMetadata(PipelineNames Name)
{
	assert(Initialized);
	return Metadata_Pipelines + u32(Name);
}

u32 PipelineManager_GetStageCout(PipelineNames Name)
{
	assert(Initialized);
	return Metadata_Pipelines[u32(Name)].StageCount;
}
