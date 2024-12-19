@echo off


if "%VULKAN_SDK%" == "" (
    echo VULKAN_SDK environment variable is not set.
    pause
    exit /b 1
)

set PROJECT_DIR=%1

if "%PROJECT_DIR%" == "" (
    echo PROJECT_DIR environment variable is not set.
    pause
    exit /b 2
)

set SHADER_PATH=%PROJECT_DIR%Source/BMR/Shaders/
set SHADER_OUTPUT=%PROJECT_DIR%/Resources/Shaders/

if not exist "%SHADER_OUTPUT%" (
    mkdir "%SHADER_OUTPUT%"
    if errorlevel 1 (
        echo Failed to create directory "%SHADER_OUTPUT%".
        pause
        exit /b 3
    )
)

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%vert.spv -V %SHADER_PATH%Entity.vert.glsl
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%frag.spv -V %SHADER_PATH%Entity.frag.glsl

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%second_vert.spv -V %SHADER_PATH%Deffered.vert.glsl
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%second_frag.spv -V %SHADER_PATH%Deffered.frag.glsl

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%TerrainGenerator_vert.spv -V %SHADER_PATH%TerrainGenerator.vert.glsl
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%TerrainGenerator_frag.spv -V %SHADER_PATH%TerrainGenerator.frag.glsl

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%SkyBox_vert.spv -V %SHADER_PATH%SkyBox.vert.glsl
%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%SkyBox_frag.spv -V %SHADER_PATH%SkyBox.frag.glsl

%VULKAN_SDK%/Bin/glslangValidator.exe -o %SHADER_OUTPUT%Depth_vert.spv -V %SHADER_PATH%Depth.vert.glsl
pause