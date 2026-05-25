#pragma once

#include "ShortTypes.h"
#include "RenderInterface.h"

struct Metadata_Stage
{
	BmRender_PipelineShaderStage Stage;
	const char* EntryPoint;
};

struct Metadata_Pipeline
{
	const char* SourcePath;
	const char* FilePath;
	const char* ModuleName;
	const Metadata_Stage* Stages;
	u32 StageCount;
};
