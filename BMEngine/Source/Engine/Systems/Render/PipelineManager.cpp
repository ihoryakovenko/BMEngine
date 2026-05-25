#include "PipelineManager.h"

#include <Util/Util.h>
#include <SharedLib.h>

#include <Util/LiveShaders.h>

#include <slang.h>
#include <slang-com-ptr.h>

#include <vector>
#include <filesystem>
#include <fstream>

#include <thread>

namespace fs = std::filesystem;

static BmRender_Shader Shaders[(u32)PipelineNames::MAX_VALUE];
static BmRender_PipelineLayout Layouts[(u32)PipelineNames::MAX_VALUE];
static BmRender_Pipeline Pipelines[(u32)PipelineNames::MAX_VALUE];
static bool LiveShaders;
static bool Initialized;

static slang::IGlobalSession* GlobalSession;
static slang::ISession* Session;

static fs::file_time_type LastCompilationTime[(u32)PipelineNames::MAX_VALUE];
static BmRender_PipelineSettings SavedSettings[(u32)PipelineNames::MAX_VALUE];
static AttachmentData SavedAttachmentData[(u32)PipelineNames::MAX_VALUE];

void PipelineManger_Init(bool EnableLiveShaders)
{
	LiveShaders = EnableLiveShaders;

	if (LiveShaders)
	{
		slang::createGlobalSession(&GlobalSession);
	}

	std::vector<char> ShaderCode;
	for (u32 i = 0; i < (u32)PipelineNames::MAX_VALUE; ++i)
	{
		if (Util::OpenAndReadFileFull(Metadata_Pipelines[i].FilePath, ShaderCode, "rb"))
		{
			BmRender_ShaderDescription ShaderDesc = {};
			ShaderDesc.Code = reinterpret_cast<const u32*>(ShaderCode.data());
			ShaderDesc.CodeSize = ShaderCode.size();
			Shaders[i] = BmRender_CreateShader(&ShaderDesc);

			if (LiveShaders)
			{
				LastCompilationTime[i] = fs::last_write_time(Metadata_Pipelines[i].FilePath);
			}
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
	if (LiveShaders)
	{
		GlobalSession->Release();
	}

	assert(Initialized);
	for (u32 i = 0; i < (u32)PipelineNames::MAX_VALUE; ++i)
	{
		BmRender_DestroyShader(Shaders[i]);
		BmRender_DestroyPipelineLayout(Layouts[i]);
		BmRender_DestroyPipeline(Pipelines[i]);
	}

	Initialized = false;
}

void PipelineManager_Update()
{
	if (LiveShaders)
	{
		for (u32 i = 0; i < (u32)PipelineNames::MAX_VALUE; ++i)
		{
			const fs::file_time_type WriteTime = fs::last_write_time(Metadata_Pipelines[i].SourcePath);
			if (WriteTime <= LastCompilationTime[i])
			{
				continue;
			}

			BmRender_DeviceWaitIdle();

			if (!Session)
			{
				Session = InitalizeSlangSession(GlobalSession, Metadata_ShadersIcludePaths, sizeof(Metadata_ShadersIcludePaths) / sizeof(Metadata_ShadersIcludePaths[0]));
			}

			LastCompilationTime[i] = WriteTime;

			Slang::ComPtr<slang::IBlob> Diagnostics;
			Slang::ComPtr<slang::IModule> Module;
			std::vector<Slang::ComPtr<slang::IComponentType>> ComponentsToLink;
			Slang::ComPtr<slang::IComponentType> Program;
			Slang::ComPtr<slang::IComponentType> LinkedProgram;
			Slang::ComPtr<ISlangBlob> Spirv;

			std::ifstream ShaderSourceFile(Metadata_Pipelines[i].SourcePath, std::ios::binary);

			ShaderSourceFile.seekg(0, std::ios::end);
			const u64 FileSize = static_cast<u64>(ShaderSourceFile.tellg());
			ShaderSourceFile.seekg(0, std::ios::beg);

			std::string ShaderSource(FileSize, '/0');
			ShaderSourceFile.read(ShaderSource.data(), FileSize);

			Module = Session->loadModuleFromSourceString(Metadata_Pipelines[i].ModuleName, Metadata_Pipelines[i].SourcePath, ShaderSource.data(), Diagnostics.writeRef());
			bool DiagnosticsCheck = CheckDiagnostics(Diagnostics);
			if (!DiagnosticsCheck)
			{
				continue;
			}

			const int definedEntryPointCount = Module->getDefinedEntryPointCount();
			if (definedEntryPointCount == 0)
			{
				continue;
			}

			for (int i = 0; i < definedEntryPointCount; i++)
			{
				Slang::ComPtr<slang::IEntryPoint> entryPoint;
				Module->getDefinedEntryPoint(i, entryPoint.writeRef());
				ComponentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(entryPoint.get()));
			}

			Session->createCompositeComponentType((slang::IComponentType**)ComponentsToLink.data(), ComponentsToLink.size(), Program.writeRef(), Diagnostics.writeRef());
			CheckDiagnostics(Diagnostics);
			if (!DiagnosticsCheck)
			{
				continue;
			}

			Program->link(LinkedProgram.writeRef(), Diagnostics.writeRef());
			CheckDiagnostics(Diagnostics);
			if (!DiagnosticsCheck)
			{
				continue;
			}

			LinkedProgram->getTargetCode(0, Spirv.writeRef(), Diagnostics.writeRef());
			CheckDiagnostics(Diagnostics);
			if (!DiagnosticsCheck)
			{
				continue;
			}

			BmRender_DestroyPipeline(Pipelines[i]);
			BmRender_DestroyShader(Shaders[i]);

			BmRender_ShaderDescription ShaderDesc = {};
			ShaderDesc.Code = reinterpret_cast<const u32*>(Spirv->getBufferPointer());
			ShaderDesc.CodeSize = Spirv->getBufferSize();
			Shaders[i] = BmRender_CreateShader(&ShaderDesc);

			const Metadata_Pipeline* Metadata = Metadata_Pipelines + i;
			BmRender_ShaderStageDescription* StageDescriptions = (BmRender_ShaderStageDescription*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), sizeof(BmRender_ShaderStageDescription) * Metadata->StageCount);

			for (u32 j = 0; j < Metadata->StageCount; ++j)
			{
				BmRender_ShaderStageDescription* Stage = StageDescriptions + j;
				Stage->Shader = Shaders[i];
				Stage->EntryPointFunction = Metadata->Stages[j].EntryPoint;
				Stage->Stage = Metadata->Stages[j].Stage;
			}

			Pipelines[i] = BmRender_CreatePipeline(Layouts[i], SavedSettings + i, StageDescriptions, Metadata->StageCount, SavedAttachmentData + i);
		}

		if (Session)
		{
			Session->Release();
			Session = nullptr;
		}
	}
}

void PipelineManager_CreatePipelineLayout(PipelineNames Name, const BmRender_DescriptorSetLayout* SetLayouts, u32 SetLayoutsCount,
	const BmRender_PushConstant* PushConstants, u32 PushConstantsCount, BmRender_PipelineType PipelineType)
{
	assert(Initialized);

	BmRender_PipelineLayoutDescription LayoutDesc = {};
	LayoutDesc.SetLayoutCount = SetLayoutsCount;
	LayoutDesc.SetLayouts = SetLayouts;
	LayoutDesc.PushConstantRangeCount = PushConstantsCount;
	LayoutDesc.PushConstantRanges = PushConstants;
	LayoutDesc.PipelineType = PipelineType;

	Layouts[u32(Name)] = BmRender_CreatePipelineLayout(&LayoutDesc);
}

void PipelineManager_CreatePipeline(PipelineNames Name, const BmRender_PipelineSettings* Settings, const AttachmentData* ResourceInfo)
{
	assert(Initialized);

	const Metadata_Pipeline* Metadata = Metadata_Pipelines + u32(Name);
	BmRender_ShaderStageDescription* StageDescriptions = (BmRender_ShaderStageDescription*)Memory_LinearAllocator_Alloc(Memory::GetGeneralFrameMemory(), sizeof(BmRender_ShaderStageDescription) * Metadata->StageCount);

	for (u32 i = 0; i < Metadata->StageCount; ++i)
	{
		BmRender_ShaderStageDescription* Stage = StageDescriptions + i;
		Stage->Shader = Shaders[(u32)Name];
		Stage->EntryPointFunction = Metadata->Stages[i].EntryPoint;
		Stage->Stage = Metadata->Stages[i].Stage;
	}

	Pipelines[u32(Name)] = BmRender_CreatePipeline(Layouts[u32(Name)], Settings, StageDescriptions, Metadata->StageCount, ResourceInfo);

	if (LiveShaders)
	{
		SavedSettings[u32(Name)] = *Settings;
		SavedAttachmentData[u32(Name)] = *ResourceInfo;
	}
}

BmRender_Pipeline PipelineManager_GetPipeline(PipelineNames Name)
{
	assert(Initialized);
	return Pipelines[u32(Name)];
}
