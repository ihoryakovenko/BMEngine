@echo off

if "%VULKAN_SDK%" == "" (
    echo VULKAN_SDK environment variable is not set.
    exit /b 1
)

set SHADER_PATH=%~dp0

%VULKAN_SDK%/Bin/glslangValidator.exe -V %SHADER_PATH%Shader.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -V %SHADER_PATH%Shader.frag

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%second_vert.spv -V %SHADER_PATH%Second.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%second_frag.spv -V %SHADER_PATH%Second.frag

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%TerrainGenerator_vert.spv -V %SHADER_PATH%TerrainGenerator.vert
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%TerrainGenerator_frag.spv -V %SHADER_PATH%TerrainGenerator.frag
pause