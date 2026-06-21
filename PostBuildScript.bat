@echo off

set SOURCE_DIR=%~1
set BUILD_DIR=%~2

echo Source: %SOURCE_DIR%
echo Build: %BUILD_DIR%

xcopy /E /I /Y /D "%SOURCE_DIR%\BMEngine\Resources" "%BUILD_DIR%\Resources"