#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <fstream>

#include <slang.h>
#include <slang-com-ptr.h>

namespace fs = std::filesystem;

const std::string SPV_EXT = ".spv";
const std::string SLANG_EXTENSION = ".slang";

bool IsSlangFile(const std::string& fileName)
{
    if (fileName.size() >= SLANG_EXTENSION.size() && fileName.compare(fileName.size() - SLANG_EXTENSION.size(), SLANG_EXTENSION.size(), SLANG_EXTENSION) == 0)
    {
        return true;
    }

    return false;
}

std::vector<Slang::ComPtr<slang::IComponentType>> GetEntryPoints(Slang::ComPtr<slang::IModule> module)
{
    std::vector<Slang::ComPtr<slang::IComponentType>> componentsToLink;
    const int definedEntryPointCount = module->getDefinedEntryPointCount();

    for (int i = 0; i < definedEntryPointCount; i++)
    {
        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        module->getDefinedEntryPoint(i, entryPoint.writeRef());
        printf("Entry point: %s \n", entryPoint->getFunctionReflection()->getName());
        componentsToLink.push_back(Slang::ComPtr<slang::IComponentType>(entryPoint.get()));
    }

    return componentsToLink;
}

void reflectProgram(slang::IComponentType* compiledProgram)
{
    slang::ProgramLayout* programLayout = compiledProgram->getLayout();
    uint32_t parameterCount = programLayout->getParameterCount();

    for (uint32_t i = 0; i < parameterCount; ++i)
    {
        slang::VariableLayoutReflection* varLayout = programLayout->getParameterByIndex(i);
        slang::VariableReflection* var = varLayout->getVariable();
        slang::TypeReflection* type = var->getType();
        SlangResourceShape Shape = type->getResourceShape();

        SlangParameterCategory category;
        const char* KindString = "";

        switch (type->getKind())
        {
        case slang::TypeReflection::Kind::ConstantBuffer:
            category = SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER;
            KindString = "UniformBuffer";
            break;

        case slang::TypeReflection::Kind::Resource:
            category = SLANG_PARAMETER_CATEGORY_SHADER_RESOURCE;
            if (Shape == SLANG_STRUCTURED_BUFFER)
            {
                KindString = "StorageBuffer";
            }
            else if (Shape & SLANG_TEXTURE_COMBINED_FLAG)
            {
                KindString = "ImageSampler";
            }
        }

        uint32_t bindingIndex = varLayout->getBindingIndex();
        uint32_t spaceIndex = varLayout->getBindingSpace(category);

        printf("%s: %s\n", KindString, var->getName());
        printf("Binding: %u, Set:%u\n", bindingIndex, spaceIndex);
    }
}

int main(int argc, const char* argv[])
{
    argc = 3;
    argv[1] = "E:/code/BMEngine/BMEngine/Source/Engine/Systems/Render/Shaders";
    argv[2] = "E:/code/BMEngine/BMEngine/Resources/Shaders";
    argv[3] = "-fr";


    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <SOURCE_DIR> <OUTPUT_DIR>" << std::endl;
        return 1;
    }

    const fs::path sourceDir(argv[1]);
    const fs::path outputDir(argv[2]);

    const std::string IncludePath = sourceDir.string();
    const bool ForceRecompile = std::string(argv[3]) == "-fr";

    if (!fs::exists(sourceDir))
    {
        std::cerr << "Source directory does not exist: " << sourceDir << std::endl;
        return 1;
    }

    fs::create_directories(outputDir);

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
        const std::string fileName = filePath.filename().string();

        if (!IsSlangFile(fileName))
        {
            continue;
        }

        std::string baseName = filePath.stem().string();
        std::replace(baseName.begin(), baseName.end(), '.', '_');

        fs::path outPath = outputDir / (baseName + SPV_EXT);

        if (!ForceRecompile && fs::exists(outPath) && fs::last_write_time(filePath) <= fs::last_write_time(outPath))
        {
            std::cout << "Up to date: " << fileName << std::endl;
            continue;
        }

        fs::remove(outPath);

        std::ifstream ShaderSourceFile(filePath, std::ios::binary);

        ShaderSourceFile.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(ShaderSourceFile.tellg());
        ShaderSourceFile.seekg(0, std::ios::beg);

        std::string ShaderSource(fileSize, '/0');
        ShaderSourceFile.read(ShaderSource.data(), fileSize);

        Slang::ComPtr<slang::IBlob> diagnostics = nullptr;
        Slang::ComPtr<slang::IModule> module;
        module = session->loadModuleFromSourceString(baseName.c_str(), nullptr, ShaderSource.data(), diagnostics.writeRef());

        if (diagnostics)
        {
            std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
        }

        SlangResult SlResult;

        std::vector<Slang::ComPtr<slang::IComponentType>> componentsToLink = GetEntryPoints(module);
        Slang::ComPtr<slang::IComponentType> program;
        SlResult = session->createCompositeComponentType((slang::IComponentType**)componentsToLink.data(), componentsToLink.size(), program.writeRef(), diagnostics.writeRef());

        if (diagnostics)
        {
            std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
            SLANG_ASSERT_ON_FAIL(SlResult);
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        SlResult = program->link(linkedProgram.writeRef(), diagnostics.writeRef());

        if (diagnostics)
        {
            std::cout << (const char*)diagnostics->getBufferPointer() << std::endl;
            SLANG_ASSERT_ON_FAIL(SlResult);
        }

        reflectProgram(linkedProgram);

        Slang::ComPtr<ISlangBlob> spirv = nullptr;
        //linkedProgram->getTargetCode()
        linkedProgram->getEntryPointCode(0, 0, spirv.writeRef(), nullptr);

        if (!spirv)
        {
            std::cout << "Failed to compile shader\n";
            continue;
        }

        std::cout << baseName << " compiled\n";

        std::ofstream SpirVOutput(outPath, std::ios::binary);
        SpirVOutput.write((const char*)spirv->getBufferPointer(), spirv->getBufferSize());
    }

    session->Release();
    globalSession->Release();

    return 0;
}