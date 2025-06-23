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

set SHADER_PATH=%PROJECT_DIR%Source\\Engine\\Systems\\Render\\Shaders\\
set SHADER_OUTPUT=%PROJECT_DIR%\\Resources\\Shaders\\
set META_INFO_FILE=%PROJECT_DIR%.tmp\\ShadersMetaInfo.txt
set TEMP_META_FILE=%PROJECT_DIR%.tmp\\ShadersMetaInfo_temp.txt

if not exist "%SHADER_OUTPUT%" (
    mkdir "%SHADER_OUTPUT%"
    if errorlevel 1 (
        echo Failed to create directory "%SHADER_OUTPUT%".
        pause
        exit /b 3
    )
)

if not exist "%PROJECT_DIR%.tmp" (
    mkdir "%PROJECT_DIR%.tmp"
    if errorlevel 1 (
        echo Failed to create directory "%PROJECT_DIR%.tmp".
        pause
        exit /b 4
    )
)

echo Checking for ShadersMetaInfo.txt...

if not exist "%META_INFO_FILE%" (
    echo Creating ShadersMetaInfo.txt...
    echo # Shader Meta Information > "%META_INFO_FILE%"
    echo # Format: ShaderName^|LastModifiedTime >> "%META_INFO_FILE%"
    echo. >> "%META_INFO_FILE%"
)

echo # Shader Meta Information > "%TEMP_META_FILE%"
echo # Format: ShaderName^|LastModifiedTime >> "%TEMP_META_FILE%"
echo. >> "%TEMP_META_FILE%"

echo Compiling shaders in "%SHADER_PATH%"...

for %%f in ("%SHADER_PATH%"*.glsl) do (
    set "FULLFILE=%%~nxf"
    setlocal enabledelayedexpansion
    set "FILENAME=%%~nf"
    set "EXT=%%~xf"
    set "SHADER_FILE=!FILENAME!!EXT!"
    set "NEED_COMPILE=1"

    for /f "tokens=1,2 delims=." %%a in ("!FILENAME!") do (
        set "BASENAME=%%a"
        set "TYPE=%%b"

        set "OUTPUT_FILE=%SHADER_OUTPUT%!BASENAME!_!TYPE!.spv"
        
        REM Check if shader exists in meta file and compare modification times
        if exist "%META_INFO_FILE%" (
            findstr /c:"!SHADER_FILE!^|" "%META_INFO_FILE%" >nul
            if not errorlevel 1 (
                REM Get current file modification time
                for %%t in ("%SHADER_PATH%!SHADER_FILE!") do set "CURRENT_TIME=%%~tt"
                
                REM Get stored modification time from meta file
                for /f "tokens=2 delims=^|" %%m in ('findstr /c:"!SHADER_FILE!^|" "%META_INFO_FILE%"') do set "STORED_TIME=%%m"
                
                REM Compare times (remove leading/trailing spaces and normalize format)
                set "STORED_TIME=!STORED_TIME: =!"
                set "CURRENT_TIME=!CURRENT_TIME: =!"
                
                REM Debug output to see what we're comparing
                echo Comparing: "!CURRENT_TIME!" vs "!STORED_TIME!"
                
                REM Alternative: Check if output file is newer than source file
                if exist "!OUTPUT_FILE!" (
                    for %%s in ("%SHADER_PATH%!SHADER_FILE!") do for %%o in ("!OUTPUT_FILE!") do (
                        if "%%~ts" leq "%%~to" (
                            set "NEED_COMPILE=0"
                            echo Skipping !SHADER_FILE! - output file is newer
                        )
                    )
                )
                
                if !NEED_COMPILE!==1 (
                    if "!CURRENT_TIME!"=="!STORED_TIME!" (
                        set "NEED_COMPILE=0"
                        echo Skipping !SHADER_FILE! - no changes detected
                    ) else (
                        echo Changes detected in !SHADER_FILE! - recompiling
                    )
                )
            )
        )
        
        if !NEED_COMPILE!==1 (
            echo Compiling !SHADER_FILE! to !OUTPUT_FILE! ...
            "%VULKAN_SDK%\Bin\glslangValidator.exe" -V "%SHADER_PATH%!SHADER_FILE!" -o "!OUTPUT_FILE!"
            if errorlevel 1 (
                echo Failed to compile !SHADER_FILE!
            ) else (
                echo Updating meta info for !SHADER_FILE!...
                for %%t in ("%SHADER_PATH%!SHADER_FILE!") do set "MOD_TIME=%%~tt"
                echo !SHADER_FILE!^|!MOD_TIME! >> "%TEMP_META_FILE%"
            )
        ) else (
            REM Add existing record to temp file to preserve it
            for /f "tokens=1,2 delims=^|" %%m in ('findstr /c:"!SHADER_FILE!^|" "%META_INFO_FILE%"') do echo %%m^|%%n >> "%TEMP_META_FILE%"
        )
    )
    endlocal
)

move /y "%TEMP_META_FILE%" "%META_INFO_FILE%" >nul
echo Shader compilation complete.
pause