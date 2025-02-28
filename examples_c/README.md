


# SCRIPT BUILDS:


# quick build:
```

```


# shader script:

shader_build.bat
```
cd assets
glslc -fshader-stage=vertex vert.glsl -o vert.spv
glslc -fshader-stage=fragment frag.glsl -o frag.spv
```

## build SCRIPT:


```

```


folder > build
```
cmake ..
```
setup build files.




build.bat
```
@echo off
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build .
cd ..
```

