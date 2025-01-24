@echo off

set CURRENT_PATH=%~dp0

xcopy /E /I /Y /D %CURRENT_PATH%BMEngine\Resources %CURRENT_PATH%x64\Debug\Resources
xcopy /E /I /Y /D %CURRENT_PATH%BMEngine\Resources %CURRENT_PATH%x64\Release\Resources