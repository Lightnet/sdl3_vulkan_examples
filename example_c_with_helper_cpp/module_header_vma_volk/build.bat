@echo off
setlocal

echo Setting up build directory...
if not exist build mkdir build
cd build

echo Configuring with CMake...
cmake -DCMAKE_BUILD_TYPE=Debug ..

echo Building the project...
@REM cmake --build . --config Debug --verbose  > build_log.txt 2>&1
cmake --build . --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo Build completed successfully!
cd ..
endlocal