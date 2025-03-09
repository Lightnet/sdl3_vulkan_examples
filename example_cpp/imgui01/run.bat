@echo off
setlocal

set APPPATH=build\Debug\ImGuiSDL3Vulkan.exe
set EXECUTABLE=ImGuiSDL3Vulkan.exe

if not exist %APPPATH% (
    echo Executable not found! Please build the project first.
    exit /b 1
)

echo Running ImGuiSDL3Vulkan...

cd build\Debug\

%EXECUTABLE%

if %ERRORLEVEL% NEQ 0 (
    echo Program exited with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo Program ran successfully!
endlocal