@echo off
setlocal

echo Setting up build directory...
if not exist build mkdir build
cd build

echo Configuring with CMake...
cmake -DCMAKE_BUILD_TYPE=Debug ..

echo Building the project...
cmake --build . --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo Build completed successfully!
cd ..
endlocal