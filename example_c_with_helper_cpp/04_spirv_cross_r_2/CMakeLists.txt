# CMakeLists.txt: Build configuration for VulkanSDL3Project
# Purpose: Configures the build system to compile and link a Vulkan application with SDL3, cglm, vk-bootstrap, VMA, and SPIRV-Cross dependencies.

# Minimum CMake version required for modern FetchContent features and C++17 support
cmake_minimum_required(VERSION 3.20)

# Project name and languages (C for main.c, C++ for vulkan_init.cpp)
project(VulkanSDL3Project C CXX)

# Set C and C++ standards for consistency across platforms
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define Vulkan SDK path; adjust this if your SDK is installed elsewhere
set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.304.1" CACHE PATH "Path to Vulkan SDK")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${VULKAN_SDK_PATH}")
find_package(Vulkan REQUIRED) # Locate Vulkan SDK (vulkan-1.lib and headers)
message(STATUS "Vulkan SDK found at: ${Vulkan_INCLUDE_DIRS}")

# Include FetchContent module to download and build dependencies from GitHub
include(FetchContent)

# Fetch SDL3: Windowing and input handling library
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.4 # Stable release for reliability
)
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE) # Required for SDL_Vulkan_* functions
set(SDL_STATIC OFF CACHE BOOL "Build static SDL library" FORCE) # Use dynamic linking for simplicity
FetchContent_MakeAvailable(SDL3)

# Fetch cglm: Math library for vectors and matrices
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6 # Known stable version
)
FetchContent_MakeAvailable(cglm)

# Fetch vk-bootstrap: Simplifies Vulkan initialization (instance, device, swapchain)
FetchContent_Declare(
    vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG v1.3.299 # Latest stable version at project start
)
FetchContent_MakeAvailable(vk_bootstrap)

# Fetch Vulkan Memory Allocator (VMA): Manages Vulkan memory allocations
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.0.1 # Stable version
)
FetchContent_MakeAvailable(vma)

# Fetch SPIRV-Cross: Reflects SPIR-V shaders for vertex input setup
FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG vulkan-sdk-1.4.304.1 # Matches Vulkan SDK version
)
set(SPIRV_CROSS_STATIC ON CACHE BOOL "Build SPIRV-Cross as static libraries" FORCE) # Static linking avoids DLL issues
set(SPIRV_CROSS_CLI OFF CACHE BOOL "Disable SPIRV-Cross CLI" FORCE) # No CLI needed
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Disable SPIRV-Cross tests" FORCE) # Skip tests for faster build
set(SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "Enable GLSL support" FORCE) # Required for GLSL shader reflection
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "Disable CPP support" FORCE) # Not needed
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "Disable MSL support" FORCE) # Not needed
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "Disable HLSL support" FORCE) # Not needed
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "Disable JSON reflection" FORCE) # Not needed
FetchContent_MakeAvailable(spirv_cross)

# Define the executable target with source files
add_executable(${PROJECT_NAME} 
    src/main.c          # Main C file with application logic
    src/vulkan_init.cpp # C++ file for Vulkan initialization
)
set_source_files_properties(src/main.c PROPERTIES LANGUAGE C)          # Explicitly set C language
set_source_files_properties(src/vulkan_init.cpp PROPERTIES LANGUAGE CXX) # Explicitly set C++ language

# Include directories for header files
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${VULKAN_SDK_PATH}/Include"        # Vulkan SDK headers (vulkan.h, etc.)
    "${vk_bootstrap_SOURCE_DIR}/include" # vk-bootstrap headers (VkBootstrap.h)
    "${vma_SOURCE_DIR}/include"         # VMA headers (vk_mem_alloc.h)
    "${spirv_cross_SOURCE_DIR}"         # SPIRV-Cross headers (spirv_cross_c.h)
)

# Link libraries to the executable
# Note: Explicit paths ensure Debug/Release builds link correctly; adjust paths if build output changes
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3                         # SDL3 dynamic library
    cglm                               # cglm dynamic library
    vk-bootstrap::vk-bootstrap         # vk-bootstrap static library
    "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib" # Vulkan core library
    "$<IF:$<CONFIG:Debug>,${spirv_cross_BINARY_DIR}/Debug/spirv-cross-cd.lib,${spirv_cross_BINARY_DIR}/Release/spirv-cross-c.lib>" # SPIRV-Cross C API
    "$<IF:$<CONFIG:Debug>,${spirv_cross_BINARY_DIR}/Debug/spirv-cross-cored.lib,${spirv_cross_BINARY_DIR}/Release/spirv-cross-core.lib>" # SPIRV-Cross core
    "$<IF:$<CONFIG:Debug>,${spirv_cross_BINARY_DIR}/Debug/spirv-cross-glsld.lib,${spirv_cross_BINARY_DIR}/Release/spirv-cross-glsl.lib>" # SPIRV-Cross GLSL
    "$<IF:$<CONFIG:Debug>,${vma_BINARY_DIR}/src/Debug/VulkanMemoryAllocatord.lib,${vma_BINARY_DIR}/src/Release/VulkanMemoryAllocator.lib>" # VMA library
)

# Ensure SPIRV-Cross builds before the executable (avoids LNK1104 for missing libs)
add_dependencies(${PROJECT_NAME} spirv-cross-c spirv-cross-core spirv-cross-glsl)

# Locate glslc shader compiler from Vulkan SDK
find_program(GLSLC glslc REQUIRED HINTS "${VULKAN_SDK_PATH}/Bin")
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found at ${VULKAN_SDK_PATH}/Bin") # Fatal if missing; required for shader compilation
endif()

# Shader compilation setup
# Note: Assumes vert.glsl and frag.glsl are in assets/; outputs SPIR-V to build/shaders/
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR}) # Create output directory if it doesn’t exist

foreach(SHADER vert frag)
    set(SHADER_SRC ${SHADER_SRC_DIR}/${SHADER}.glsl)
    set(SHADER_OUT ${SHADER_OUT_DIR}/${SHADER}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUT}
        COMMAND ${GLSLC} -fshader-stage=${SHADER} ${SHADER_SRC} -o ${SHADER_OUT} # Compile GLSL to SPIR-V
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling ${SHADER}.glsl to ${SHADER}.spv"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS}) # Build shaders as part of default target
add_dependencies(${PROJECT_NAME} Shaders) # Ensure shaders build before executable

# Windows-specific post-build steps
# Copies required DLLs and shaders to the output directory (Debug/) for runtime
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:${PROJECT_NAME}> # SDL3.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "C:/Windows/System32/vulkan-1.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}> # vulkan-1.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_OUT_DIR}/vert.spv" $<TARGET_FILE_DIR:${PROJECT_NAME}> # Vertex shader
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_OUT_DIR}/frag.spv" $<TARGET_FILE_DIR:${PROJECT_NAME}> # Fragment shader
        COMMENT "Copying DLLs and shaders to output directory"
    )
endif()

# Notes for Users:
# - Dependencies are fetched from GitHub; ensure internet access during first configure.
# - Vulkan SDK path must match your installation; update VULKAN_SDK_PATH if different.
# - Debug builds link -d suffixed libs (e.g., spirv-cross-cd.lib); Release uses non-suffixed.
# - Shader files (vert.glsl, frag.glsl) must be in assets/ directory relative to CMakeLists.txt.
# - Add new source files to add_executable() if expanding the project.