#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

#include <slang.h>
#include <slang-com-ptr.h>

#include <Engine/Systems/Render/PipelineMetadata.h>
#include <Util/LiveShaders.h>

namespace fs = std::filesystem;

const std::string SPV_EXT = ".spv";
const std::string GNERATED_EXT = ".generated.h";
const std::string SLANG_EXTENSION = ".slang";

struct DescriptorSetGenerationHelper
{
	std::string Text;
	u32 Count;
};

bool IsSlangFile(const std::string& fileName)
{
	if (fileName.size() >= SLANG_EXTENSION.size() && fileName.compare(fileName.size() - SLANG_EXTENSION.size(), SLANG_EXTENSION.size(), SLANG_EXTENSION) == 0)
	{
		return true;
	}

	return false;
}

int main(int argc, const char* argv[])
{
	//argc = 4;
	//argv[1] = "E:/code/BMEngine/BMEngine/Source/Engine/Systems/Render/Shaders";
	//argv[2] = "E:/code/BMEngine/BMEngine/Resources/Shaders";
	//argv[3] = "E:/code/BMEngine/BMEngine/Source/Generated";
	//argv[4] = "-fr";

	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <SOURCE_DIR> <OUTPUT_DIR> <GEN_OUTPUT_DIR>" << std::endl;
		return 1;
	}

	std::vector<std::string> Names;

	const fs::path sourceDir(argv[1]);
	const fs::path outputDir(argv[2]);
	const fs::path GenOutputDir(argv[3]);

	const std::string IncludePath = sourceDir.string();
	const bool ForceRecompile = argc > 3 && std::string(argv[4]) == "-fr";

	if (!fs::exists(sourceDir))
	{
		std::cerr << "Source directory does not exist: " << sourceDir << std::endl;
		return 1;
	}

	fs::create_directories(outputDir);
	fs::create_directories(GenOutputDir);

	slang::IGlobalSession* globalSession = nullptr;
	slang::createGlobalSession(&globalSession);

	const char* searchPaths[] = { IncludePath.c_str() };

	slang::ISession* session = InitalizeSlangSession(globalSession, searchPaths, sizeof(searchPaths) / sizeof(searchPaths[0]));
	

	std::string StageVariableName = "Metadata_Stages";
	std::string StageArrayGeneration = "inline constexpr Metadata_Stage " + StageVariableName + "[] = { ";

	std::string PipelineVariableName = "Metadata_Pipelines";
	std::string PipelineGeneration = "inline constexpr Metadata_Pipeline " + PipelineVariableName + "[] = { ";

	u32 TotalStages = 0;

	for (const auto& entry : fs::directory_iterator(sourceDir))
	{
		if (!entry.is_regular_file()) continue;

		const fs::path filePath = entry.path();
		const std::string FullPathName = filePath.filename().string();

		if (!IsSlangFile(FullPathName))
		{
			continue;
		}

		std::string baseName = filePath.stem().string();
		std::replace(baseName.begin(), baseName.end(), '.', '_');

		fs::path outPath = outputDir / (baseName + SPV_EXT);

		std::ifstream ShaderSourceFile(filePath, std::ios::binary);

		ShaderSourceFile.seekg(0, std::ios::end);
		size_t fileSize = static_cast<size_t>(ShaderSourceFile.tellg());
		ShaderSourceFile.seekg(0, std::ios::beg);

		std::string ShaderSource(fileSize, '/0');
		ShaderSourceFile.read(ShaderSource.data(), fileSize);

		bool DiagnosticsCheck = true;

		Slang::ComPtr<slang::IBlob> diagnostics;
		Slang::ComPtr<slang::IModule> module;
		std::vector<Slang::ComPtr<slang::IComponentType>> componentsToLink;
		Slang::ComPtr<slang::IComponentType> program;
		Slang::ComPtr<slang::IComponentType> linkedProgram;
		Slang::ComPtr<ISlangBlob> spirv;

		module = session->loadModuleFromSourceString(baseName.c_str(), FullPathName.c_str(), ShaderSource.data(), diagnostics.writeRef());
		DiagnosticsCheck = CheckDiagnostics(diagnostics);
		assert(DiagnosticsCheck);

		const int definedEntryPointCount = module->getDefinedEntryPointCount();
		if (definedEntryPointCount == 0)
		{
			continue;
		}

		for (int i = 0; i < definedEntryPointCount; i++)
		{
			Slang::ComPtr<slang::IEntryPoint> entryPoint;
			module->getDefinedEntryPoint(i, entryPoint.writeRef());
			componentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(entryPoint.get()));
		}

		session->createCompositeComponentType((slang::IComponentType**)componentsToLink.data(), componentsToLink.size(), program.writeRef(), diagnostics.writeRef());
		CheckDiagnostics(diagnostics);
		assert(DiagnosticsCheck);

		program->link(linkedProgram.writeRef(), diagnostics.writeRef());
		CheckDiagnostics(diagnostics);
		assert(DiagnosticsCheck);

		slang::ProgramLayout* ProgramLayout = program->getLayout();
		const u32 EntryPointsCount = ProgramLayout->getEntryPointCount();
		for (int i = 0; i < EntryPointsCount; ++i)
		{
			slang::EntryPointReflection* EntryPoint = ProgramLayout->getEntryPointByIndex(i);
			SlangStage Stage = EntryPoint->getStage();

			StageArrayGeneration += "\n\t{ ";

			switch (Stage)
			{
			case SLANG_STAGE_VERTEX:
				StageArrayGeneration += "BmRender_PipelineShaderStage::Vertex";
				break;

			case SLANG_STAGE_FRAGMENT:
				StageArrayGeneration += "BmRender_PipelineShaderStage::Fragment";
				break;

			case SLANG_STAGE_COMPUTE:
				StageArrayGeneration += "BmRender_PipelineShaderStage::Compute";
				break;
			default:
				assert(false);
				break;
			}

			StageArrayGeneration += ", \"" + std::string(EntryPoint->getName()) + "\" },";
		}

		PipelineGeneration += "\n\t{ \"" + filePath.generic_string() + "\", \"" + outPath.generic_string() + "\", \"" + baseName + "\", " + StageVariableName + " + " + std::to_string(TotalStages) + ", " + std::to_string(EntryPointsCount) + " },";

		TotalStages += EntryPointsCount;

		Names.push_back(baseName);

		if (!ForceRecompile && fs::exists(outPath) &&
			fs::last_write_time(filePath) <= fs::last_write_time(outPath))
		{
			std::cout << "Up to date: " << FullPathName << std::endl;
			continue;
		}

		fs::remove(outPath);

		linkedProgram->getTargetCode(0, spirv.writeRef(), diagnostics.writeRef());
		CheckDiagnostics(diagnostics);
		assert(DiagnosticsCheck);

		std::cout << baseName << " compiled\n";

		std::ofstream SpirVOutput(outPath, std::ios::binary);
		SpirVOutput.write((const char*)spirv->getBufferPointer(), spirv->getBufferSize());
	}

	StageArrayGeneration += "\n};";
	PipelineGeneration += "\n};";

	session->Release();
	globalSession->Release();

	struct Metadata_ShaderEntry
	{
		const char* ShaderName;
		const char* FilePath;
	};

	std::string ShaderRegistryGeneration = "#pragma once\n\n#include <Engine/Systems/Render/PipelineMetadata.h>\n\n#include <slang.h>\n\n";
	ShaderRegistryGeneration += "inline const char* Metadata_ShadersIcludePaths[] = {\n";
	ShaderRegistryGeneration += "\t{\"" + IncludePath + "\"},\n";
	ShaderRegistryGeneration += "};\n\n";

	ShaderRegistryGeneration += "enum class PipelineNames : u32\n{\n";

	for (u32 i = 0; i < Names.size(); ++i)
	{
		ShaderRegistryGeneration += "\t" + Names[i] + ",\n";
	}

	ShaderRegistryGeneration += "\tMAX_VALUE\n};\n\n" + StageArrayGeneration + "\n\n" + PipelineGeneration + "\n\n";

	fs::path outGenPath = GenOutputDir / ("ShaderRegistry" + GNERATED_EXT);
	fs::remove(outGenPath);

	std::ofstream GetOutput(outGenPath, std::ios::binary);
	GetOutput.write(ShaderRegistryGeneration.c_str(), ShaderRegistryGeneration.size());

	return 0;
}