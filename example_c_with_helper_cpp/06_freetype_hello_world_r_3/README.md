# Information:
 This correct once the Grok beta 3 feed the correct Freetype font build.

 * Note not clean up yet.
 * Notes are added partly.
 * https://freetype.org/freetype2/docs/tutorial/step1.html#section-7


build.bat
```
@echo off
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Debug
cd ..
```
Note if default there might be error on build example.
```
cmake ..
```
Since those libs need to check which tool I think need more test.