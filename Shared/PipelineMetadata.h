#pragma once

#include "ShortTypes.h"
#include "RenderInterface.h"

struct Metadata_Descriptor
{
	u32 Binding;
	BmRender_DescriptorShaderStage Stage;
	BmRender_DescriptorType Type;
	bool IsBindless;
};

struct Metadata_DescriptorSet
{
	const Metadata_Descriptor* Descriptors;
	u32 DescriptorCount;
	u32 Set;
};

struct Metadata_Stage
{
	BmRender_PipelineShaderStage Stage;
	const char* EntryPoint;
};

struct Metadata_Pipeline
{
	const char* ModuleName;
	const Metadata_Stage* Stages;
	const Metadata_DescriptorSet* Sets;
	u32 StageCount;
	u32 SetCount;
};
