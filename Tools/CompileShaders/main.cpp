#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdlib>

#include <Windows.h>

namespace fs = std::filesystem;

std::string SLANG_COMPILER;
const std::string SPV_EXT = ".spv";

const std::vector<std::string> SLANG_EXTENSIONS =
{
    ".vert.slang",
    ".frag.slang",
};

bool is_slang_file(const std::string& fileName)
{
    for (const auto& ext : SLANG_EXTENSIONS)
    {
        if (fileName.size() >= ext.size() && fileName.compare(fileName.size() - ext.size(), ext.size(), ext) == 0)
        {
            return true;
        }
    }

    return false;
}

void compile_slang(const fs::path& source, const fs::path& output)
{
    std::string command = SLANG_COMPILER + " \"" + source.string() + "\" -o \"" + output.string() + "\"";
    std::cout << "Compiling Slang: " << source.filename().string() << std::endl;

    if (std::system(command.c_str()) != 0)
    {
        std::cerr << "Error: Failed to compile " << source.filename().string() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <SOURCE_DIR> <OUTPUT_DIR> <SLANG_COMPILER>" << std::endl;
        return 1;
    }

    const fs::path sourceDir(argv[1]);
    const fs::path outputDir(argv[2]);
    SLANG_COMPILER = argv[3];

    try
    {
        if (!fs::exists(sourceDir))
        {
            std::cerr << "Source directory does not exist: " << sourceDir << std::endl;
            return 1;
        }

        fs::create_directories(outputDir);

        for (const auto& entry : fs::directory_iterator(sourceDir))
        {
            if (!entry.is_regular_file()) continue;

            const fs::path filePath = entry.path();
            const std::string fileName = filePath.filename().string();

            if (is_slang_file(fileName))
            {
                std::string baseName = filePath.stem().string();
                std::replace(baseName.begin(), baseName.end(), '.', '_');

                fs::path outPath = outputDir / (baseName + SPV_EXT);

                bool needsCompile = true;
                if (fs::exists(outPath))
                {
                    if (fs::last_write_time(filePath) <= fs::last_write_time(outPath))
                    {
                        needsCompile = false;
                    }
                }

                if (needsCompile)
                {
                    compile_slang(filePath, outPath);
                }
                else
                {
                    std::cout << "Up to date: " << fileName << std::endl;
                }
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}