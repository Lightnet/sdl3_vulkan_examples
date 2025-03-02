
# Information:
 Work in progres. VS2022

build.bat
```
@echo off
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Debug
```

# Notes:
 * use this command for shader output.
```
cmake --build . --config Debug
```
 Reason check for build types.