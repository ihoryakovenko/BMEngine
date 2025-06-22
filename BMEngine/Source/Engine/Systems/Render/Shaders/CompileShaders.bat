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

set SHADER_PATH=%PROJECT_DIR%Source/Engine/Systems/Render/Shaders/
set SHADER_OUTPUT=%PROJECT_DIR%/Resources/Shaders/

if not exist "%SHADER_OUTPUT%" (
    mkdir "%SHADER_OUTPUT%"
    if errorlevel 1 (
        echo Failed to create directory "%SHADER_OUTPUT%".
        pause
        exit /b 3
    )
)

echo Compiling shaders in "%SHADER_PATH%"...

for %%f in ("%SHADER_PATH%"*.glsl) do (
    set "FULLFILE=%%~nxf"
    setlocal enabledelayedexpansion
    set "FILENAME=%%~nf"
    set "EXT=%%~xf"

    for /f "tokens=1,2 delims=." %%a in ("!FILENAME!") do (
        set "BASENAME=%%a"
        set "TYPE=%%b"

        set "OUTPUT_FILE=%SHADER_OUTPUT%!BASENAME!_!TYPE!.spv"
        echo Compiling !FILENAME!!EXT! to !OUTPUT_FILE! ...
        "%VULKAN_SDK%\Bin\glslangValidator.exe" -V "%SHADER_PATH%!FILENAME!!EXT!" -o "!OUTPUT_FILE!"
        if errorlevel 1 (
            echo Failed to compile !FILENAME!!EXT!
        )
    )
    endlocal
)

echo Shader compilation complete.
pause