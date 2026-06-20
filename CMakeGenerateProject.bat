@echo off
setlocal

REM Root directory (where this .bat is located)
set ROOT=%~dp0

REM Paths
set SOURCE_DIR=%ROOT%.
set BUILD_DIR=%ROOT%Temp

REM Clean previous build (optional but recommended for consistency)
if exist "%BUILD_DIR%" (
    echo Cleaning old Temp folder...
    rmdir /s /q "%BUILD_DIR%"
)

echo Generating Visual Studio project...

cmake ^
    -S "%SOURCE_DIR%" ^
    -B "%BUILD_DIR%" ^
    -G "Visual Studio 18 2026" ^
    -A x64

if %ERRORLEVEL% neq 0 (
    echo CMake generation failed!
    exit /b %ERRORLEVEL%
)

echo.
echo Done.
echo Solution generated in: %BUILD_DIR%
pause