@echo off
ECHO Building VulkanTriangle...

mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug

IF %ERRORLEVEL% NEQ 0 (
    ECHO Build failed!
    exit /b %ERRORLEVEL%
)

ECHO Build completed successfully!
cd ..