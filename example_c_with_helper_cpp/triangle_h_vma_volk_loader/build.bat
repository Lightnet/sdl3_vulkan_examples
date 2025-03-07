@echo off
echo 'init build'
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug --verbose  > build_log.txt 2>&1