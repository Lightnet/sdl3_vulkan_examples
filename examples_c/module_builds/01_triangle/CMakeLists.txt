cmake_minimum_required(VERSION 3.16)  # Bump to a more recent version for better features
project(VulkanTriangle
    VERSION 1.0
    DESCRIPTION "Vulkan Triangle with SDL3"
    LANGUAGES C
)

# Set C standard globally
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Define output directories for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

# Set Vulkan SDK path
set(VULKAN_SDK "C:/VulkanSDK/1.4.304.1")
set(ENV{VULKAN_SDK} "${VULKAN_SDK}")
find_package(Vulkan REQUIRED)

# Fetch SDL3 from GitHub
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
)
set(SDL_STATIC ON CACHE BOOL "Build SDL3 as a static library" FORCE)
set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as a shared library" FORCE)
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support in SDL3" FORCE)
FetchContent_MakeAvailable(SDL3)

# Source and include directories
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/shaders")

# Define source files
set(SOURCES
    "${SRC_DIR}/main.c"
    "${SRC_DIR}/vsdl_init.c"
    "${SRC_DIR}/vsdl_mesh.c"
    "${SRC_DIR}/vsdl_pipeline.c"
    "${SRC_DIR}/vsdl_types.c"
    "${SRC_DIR}/vsdl_loop.c"
    "${SRC_DIR}/vsdl_clean_up.c"
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3-static Vulkan::Vulkan)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIR}                   # Project headers
    ${SDL3_SOURCE_DIR}/include/SDL3  # SDL3 headers
    ${Vulkan_INCLUDE_DIRS}/vulkan    # Vulkan headers
)

# Debug messages
message(STATUS "SDL3_SOURCE_DIR: ${SDL3_SOURCE_DIR}")
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
file(GLOB SDL_VULKAN_HEADER "${SDL3_SOURCE_DIR}/include/SDL3/SDL_vulkan.h")
message(STATUS "Using SDL_vulkan.h: ${SDL_VULKAN_HEADER}")

# Shader compilation setup
set(GLSLC "${VULKAN_SDK}/Bin/glslc.exe")

# Ensure shader output directory exists
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}"
    COMMENT "Creating shader output directory: ${SHADER_OUT_DIR}"
)

# Compile shaders
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${GLSLC} "${SHADER_SRC_DIR}/triangle.vert" -o "${SHADER_OUT_DIR}/triangle.vert.spv"
    COMMAND ${GLSLC} "${SHADER_SRC_DIR}/triangle.frag" -o "${SHADER_OUT_DIR}/triangle.frag.spv"
    DEPENDS "${SHADER_SRC_DIR}/triangle.vert" "${SHADER_SRC_DIR}/triangle.frag"
    COMMENT "Compiling shaders to ${SHADER_OUT_DIR}"
)