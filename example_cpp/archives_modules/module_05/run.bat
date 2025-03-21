@echo off
setlocal

set APPPATH=build\Debug\VulkanTriangle.exe
set EXECUTABLE=VulkanTriangle.exe

if not exist %APPPATH% (
    echo Executable not found! Please build the project first.
    exit /b 1
)

echo Running VulkanTriangle...

cd build\Debug\

%EXECUTABLE%

if %ERRORLEVEL% NEQ 0 (
    echo Program exited with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo Program ran successfully!
endlocal