@echo off
ECHO Running VulkanTriangle...

cd build/Debug
VulkanTriangle.exe

IF %ERRORLEVEL% NEQ 0 (
    ECHO Application failed with error code %ERRORLEVEL%!
    exit /b %ERRORLEVEL%
)

ECHO Application ran successfully!
cd ..