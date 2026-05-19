#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

#include <slang.h>
#include <slang-com-ptr.h>

#include <PipelineMetadata.h>

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

void CheckDiagnostics(Slang::ComPtr<slang::IBlob> diagnostics)
{
	if (diagnostics)
	{
		std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
		assert(false);
	}
}

std::string GenerateMetadata(slang::IComponentType* LinkedProgram, const std::string& ModuleName)
{
	slang::ProgramLayout* ProgramLayout = LinkedProgram->getLayout();
	const u32 ParametersCount = ProgramLayout->getParameterCount();

	std::unordered_map<u32, DescriptorSetGenerationHelper> DescriptorSetGenerationMap;

	std::string DescriptorsVariableName = "Private_Metadata_" + ModuleName + "Descriptors";
	std::string DescriptorArrayGeneration = "inline constexpr Metadata_Descriptor " + DescriptorsVariableName + "[] = { ";
	
	u32 DescriptorCount = 0;

	for (u32 i = 0; i < ParametersCount; ++i)
	{
		slang::VariableLayoutReflection* varLayout = ProgramLayout->getParameterByIndex(i);
		slang::VariableReflection* var = varLayout->getVariable();

		slang::TypeReflection* type = var->getType();
		SlangResourceShape Shape = type->getResourceShape();

		slang::TypeReflection::Kind SlangKind = type->getKind();
		const char* DescriptorName = var->getName();

		const u32 bindingIndex = varLayout->getBindingIndex();
		const u32 SetIndex = varLayout->getBindingSpace();

		auto DescriptorGenerationIt = DescriptorSetGenerationMap.find(SetIndex);
		if (DescriptorGenerationIt == DescriptorSetGenerationMap.end())
		{
			DescriptorSetGenerationMap[SetIndex] = { std::string(), 0 };
			DescriptorGenerationIt = DescriptorSetGenerationMap.find(SetIndex);
		}

		DescriptorGenerationIt->second.Text += "\n\t{ " + std::to_string(bindingIndex) + ", BmRender_DescriptorShaderStage::Vertex | BmRender_DescriptorShaderStage::Fragment, ";
		++DescriptorGenerationIt->second.Count;

		if (SlangKind == slang::TypeReflection::Kind::ConstantBuffer)
		{
			DescriptorGenerationIt->second.Text += "BmRender_DescriptorType::UniformBuffer, false";
			++DescriptorCount;
		}
		else if (SlangKind == slang::TypeReflection::Kind::Resource)
		{
			if (Shape == SLANG_STRUCTURED_BUFFER)
			{
				DescriptorGenerationIt->second.Text += "BmRender_DescriptorType::StorageBuffer, false";
				++DescriptorCount;
			}
			else if (Shape & SLANG_TEXTURE_COMBINED_FLAG)
			{
				if (Shape & SLANG_TEXTURE_2D)
				{
					DescriptorGenerationIt->second.Text += "BmRender_DescriptorType::CombinedImageSampler, false";
					++DescriptorCount;
				}
				else
				{
					assert(false);
				}
			}
			else
			{
				assert(false);
			}
		}
		else if (SlangKind == slang::TypeReflection::Kind::Array)
		{
			slang::TypeLayoutReflection* arrayLayout = varLayout->getTypeLayout();
			slang::TypeReflection* arrayType = arrayLayout->getType();
			u64 elementCount = arrayType->getElementCount();
			bool isBindless = (elementCount == 0);
			assert(isBindless);

			if (Shape & SLANG_TEXTURE_COMBINED_FLAG)
			{
				if (Shape & SLANG_TEXTURE_2D)
				{
					DescriptorGenerationIt->second.Text += "BmRender_DescriptorType::CombinedImageSampler, true";
					++DescriptorCount;
				}
			}
			else
			{
				assert(false);
			}
		}
		else
		{
			assert(false);
		}
	   
		DescriptorGenerationIt->second.Text += " },";
	}

	std::string DescriptorsSetVariableName = "Private_Metadata_" + ModuleName + "DescriptorSets";
	std::string DescriptorSetArrayGeneration = "inline constexpr Metadata_DescriptorSet " + DescriptorsSetVariableName + "[] = { ";

	if (DescriptorCount > 0)
	{
		u32 SetIndex = 0;
		u32 TotalDescriptors = 0;
		for (const auto& [DescriptorSet, DescriptorGeneration] : DescriptorSetGenerationMap)
		{
			DescriptorArrayGeneration += DescriptorGeneration.Text;
			DescriptorSetArrayGeneration += "\n\t{ " + DescriptorsVariableName + " + " + std::to_string(TotalDescriptors) + ", "
				+ std::to_string(DescriptorGeneration.Count) + ", " + std::to_string(SetIndex) + " },";
			TotalDescriptors += DescriptorGeneration.Count;
			++SetIndex;
		}

		DescriptorArrayGeneration += "\n};";
		DescriptorSetArrayGeneration += "\n};";
	}

	std::string StageVariableName = "Private_Metadata_" + ModuleName + "Stages";
	std::string StageArrayGeneration = "inline constexpr Metadata_Stage " + StageVariableName + "[] = { ";

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

	StageArrayGeneration += "\n};";

	std::string PipelineVariableName = "Metadata_" + ModuleName + "Pipeline";
	std::string PipelineGeneration = "inline constexpr Metadata_Pipeline " + PipelineVariableName + " = { \"" + ModuleName + "\", " + StageVariableName + ", " +
		DescriptorsSetVariableName + ", " + std::to_string(EntryPointsCount) + ", " + std::to_string(DescriptorSetGenerationMap.size()) + " };";

	const std::string MetadataFile = "#pragma once\n\n#include \"RenderInterface.h\"\n#include <PipelineMetadata.h>\n\n" + DescriptorArrayGeneration + "\n\n" +
		DescriptorSetArrayGeneration + "\n\n" + StageArrayGeneration + "\n\n" + PipelineGeneration + "\n";

	return MetadataFile;
}

int main(int argc, const char* argv[])
{
	//argc = 3;
	//argv[1] = "E:/code/BMEngine/BMEngine/Source/Engine/Systems/Render/Shaders";
	//argv[2] = "E:/code/BMEngine/BMEngine/Resources/Shaders";
	//argv[3] = "E:/code/BMEngine/BMEngine/Source/Generated";
	//argv[4] = "-fr";

	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <SOURCE_DIR> <OUTPUT_DIR> <GEN_OUTPUT_DIR>" << std::endl;
		return 1;
	}

	const fs::path sourceDir(argv[1]);
	const fs::path outputDir(argv[2]);
	const fs::path GenOutputDir(argv[3]);

	const std::string IncludePath = sourceDir.string();
	const bool ForceRecompile = std::string(argv[4]) == "-fr";

	if (!fs::exists(sourceDir))
	{
		std::cerr << "Source directory does not exist: " << sourceDir << std::endl;
		return 1;
	}

	fs::create_directories(outputDir);
	fs::create_directories(GenOutputDir);

	slang::IGlobalSession* globalSession = nullptr;
	slang::createGlobalSession(&globalSession);

	slang::TargetDesc targetDesc = {};
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = globalSession->findProfile("spirv_1_5");

	const char* searchPaths[] = { IncludePath.c_str() };

	slang::CompilerOptionEntry options[] =
	{
		slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr},
		slang::CompilerOptionName::MatrixLayoutColumn, {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr},
	};

	slang::SessionDesc sessionDesc = {};
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	sessionDesc.searchPaths = searchPaths;
	sessionDesc.searchPathCount = sizeof(searchPaths) / sizeof(searchPaths[0]);
	sessionDesc.compilerOptionEntries = options;
	sessionDesc.compilerOptionEntryCount = sizeof(options) / sizeof(options[0]);

	slang::ISession* session = nullptr;
	globalSession->createSession(sessionDesc, &session);

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
		fs::path outGenPath = GenOutputDir / (baseName + GNERATED_EXT);

		if (!ForceRecompile && fs::exists(outPath) && fs::exists(outGenPath) &&
			fs::last_write_time(filePath) <= fs::last_write_time(outPath))
		{
			std::cout << "Up to date: " << FullPathName << std::endl;
			continue;
		}

		fs::remove(outPath);
		fs::remove(outGenPath);

		std::ifstream ShaderSourceFile(filePath, std::ios::binary);

		ShaderSourceFile.seekg(0, std::ios::end);
		size_t fileSize = static_cast<size_t>(ShaderSourceFile.tellg());
		ShaderSourceFile.seekg(0, std::ios::beg);

		std::string ShaderSource(fileSize, '/0');
		ShaderSourceFile.read(ShaderSource.data(), fileSize);

		Slang::ComPtr<slang::IBlob> diagnostics;
		Slang::ComPtr<slang::IModule> module;
		std::vector<Slang::ComPtr<slang::IComponentType>> componentsToLink;
		Slang::ComPtr<slang::IComponentType> program;
		Slang::ComPtr<slang::IComponentType> linkedProgram;
		Slang::ComPtr<ISlangBlob> spirv;

		module = session->loadModuleFromSourceString(baseName.c_str(), FullPathName.c_str(), ShaderSource.data(), diagnostics.writeRef());
		CheckDiagnostics(diagnostics);

		const int definedEntryPointCount = module->getDefinedEntryPointCount();
		for (int i = 0; i < definedEntryPointCount; i++)
		{
			Slang::ComPtr<slang::IEntryPoint> entryPoint;
			module->getDefinedEntryPoint(i, entryPoint.writeRef());
			componentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(entryPoint.get()));
		}

		session->createCompositeComponentType((slang::IComponentType**)componentsToLink.data(), componentsToLink.size(), program.writeRef(), diagnostics.writeRef());
		CheckDiagnostics(diagnostics);

		program->link(linkedProgram.writeRef(), diagnostics.writeRef());
		CheckDiagnostics(diagnostics);

		linkedProgram->getTargetCode(0, spirv.writeRef(), diagnostics.writeRef());
		CheckDiagnostics(diagnostics);

		const std::string& Metadata = GenerateMetadata(linkedProgram, baseName);

		std::cout << baseName << " compiled\n";

		std::ofstream SpirVOutput(outPath, std::ios::binary);
		SpirVOutput.write((const char*)spirv->getBufferPointer(), spirv->getBufferSize());

		std::ofstream GetOutput(outGenPath, std::ios::binary);
		GetOutput.write(Metadata.c_str(), Metadata.size());
	}

	session->Release();
	globalSession->Release();

	return 0;
}