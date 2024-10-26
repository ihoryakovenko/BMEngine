@echo off


if "%VULKAN_SDK%" == "" (
    echo VULKAN_SDK environment variable is not set.
    exit /b 1
)

set SHADER_PATH=%~dp0

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%vert.spv -V %SHADER_PATH%Entity.vert.glsl
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%frag.spv -V %SHADER_PATH%Entity.frag.glsl

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%second_vert.spv -V %SHADER_PATH%Deffered.vert.glsl
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%second_frag.spv -V %SHADER_PATH%Deffered.frag.glsl

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%TerrainGenerator_vert.spv -V %SHADER_PATH%TerrainGenerator.vert.glsl
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%TerrainGenerator_frag.spv -V %SHADER_PATH%TerrainGenerator.frag.glsl

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%SkyBox_vert.spv -V %SHADER_PATH%SkyBox.vert.glsl
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_PATH%SkyBox_frag.spv -V %SHADER_PATH%SkyBox.frag.glsl
pause