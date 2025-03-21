# CMakeLists.txt: Build configuration for VulkanSDL3Project
cmake_minimum_required(VERSION 3.20)
project(VulkanSDL3Project C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.304.1" CACHE PATH "Path to Vulkan SDK")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${VULKAN_SDK_PATH}")
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan SDK found at: ${Vulkan_INCLUDE_DIRS}")

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.4
)
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build static SDL library" FORCE)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(cglm)

FetchContent_Declare(
    vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG v1.3.299
)
FetchContent_MakeAvailable(vk_bootstrap)

FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.0.1
)
FetchContent_MakeAvailable(vma)

FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG vulkan-sdk-1.4.304.1
)
set(SPIRV_CROSS_STATIC ON CACHE BOOL "Build SPIRV-Cross as static libraries" FORCE)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "Disable SPIRV-Cross CLI" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Disable SPIRV-Cross tests" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "Enable GLSL support" FORCE)
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "Disable CPP support" FORCE)
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "Disable MSL support" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "Disable HLSL support" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "Disable JSON reflection" FORCE)
FetchContent_MakeAvailable(spirv_cross)

# Fetch FreeType for font rendering
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-2
)
set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable zlib" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "Disable libpng" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable BZip2" FORCE)
FetchContent_MakeAvailable(freetype)

# Fetch stb_image as a single header file
FetchContent_Declare(
    stb_image
    URL https://raw.githubusercontent.com/nothings/stb/master/stb_image.h # Direct download of stb_image.h
    DOWNLOAD_NO_EXTRACT TRUE # Don’t treat it as an archive
)
FetchContent_GetProperties(stb_image)
if(NOT stb_image_POPULATED)
    FetchContent_Populate(stb_image)
    # Copy stb_image.h to a known location (e.g., src/)
    file(COPY "${stb_image_SOURCE_DIR}/stb_image.h" DESTINATION "${CMAKE_SOURCE_DIR}/src/")
endif()

add_executable(${PROJECT_NAME} 
    src/main.c 
    src/vulkan_init.cpp
)
set_source_files_properties(src/main.c PROPERTIES LANGUAGE C)
set_source_files_properties(src/vulkan_init.cpp PROPERTIES LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${VULKAN_SDK_PATH}/Include"
    "${vk_bootstrap_SOURCE_DIR}/include"
    "${vma_SOURCE_DIR}/include"
    "${spirv_cross_SOURCE_DIR}"
    "${freetype_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src" # Include stb_image.h from src/
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    cglm
    vk-bootstrap::vk-bootstrap
    "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib"
    "$<IF:$<CONFIG:Debug>,${spirv_cross_BINARY_DIR}/Debug/spirv-cross-cd.lib,${spirv_cross_BINARY_DIR}/Release/spirv-cross-c.lib>"
    "$<IF:$<CONFIG:Debug>,${spirv_cross_BINARY_DIR}/Debug/spirv-cross-cored.lib,${spirv_cross_BINARY_DIR}/Release/spirv-cross-core.lib>"
    "$<IF:$<CONFIG:Debug>,${spirv_cross_BINARY_DIR}/Debug/spirv-cross-glsld.lib,${spirv_cross_BINARY_DIR}/Release/spirv-cross-glsl.lib>"
    "$<IF:$<CONFIG:Debug>,${vma_BINARY_DIR}/src/Debug/VulkanMemoryAllocatord.lib,${vma_BINARY_DIR}/src/Release/VulkanMemoryAllocator.lib>"
    freetype
)

add_dependencies(${PROJECT_NAME} spirv-cross-c spirv-cross-core spirv-cross-glsl)

find_program(GLSLC glslc REQUIRED HINTS "${VULKAN_SDK_PATH}/Bin")
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found at ${VULKAN_SDK_PATH}/Bin")
endif()

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

foreach(SHADER vert frag)
    set(SHADER_SRC ${SHADER_SRC_DIR}/${SHADER}.glsl)
    set(SHADER_OUT ${SHADER_OUT_DIR}/${SHADER}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUT}
        COMMAND ${GLSLC} -fshader-stage=${SHADER} ${SHADER_SRC} -o ${SHADER_OUT}
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling ${SHADER}.glsl to ${SHADER}.spv"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUT})
endforeach()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_SRC_DIR}/FiraSans-Bold.ttf" $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying FiraSans-Bold.ttf to output directory"
)

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} Shaders)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "C:/Windows/System32/vulkan-1.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_OUT_DIR}/vert.spv" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_OUT_DIR}/frag.spv" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying DLLs and shaders to output directory"
    )
endif()