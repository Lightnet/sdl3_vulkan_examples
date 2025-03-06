cmake_minimum_required(VERSION 3.26)
project(vk_sdl C CXX)

# Vulkan SDK setup
set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.304.1" CACHE PATH "Path to Vulkan SDK")

set(CMAKE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

LINK_DIRECTORIES(${CMAKE_BINARY_DIR})

file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.7/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
    EXPECTED_HASH SHA256=C0FC82149E00C43A21FEBE7B2CA57B2FFEA2B8E88AB867022C21D6B81937EB50
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# SDL3
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.8")
CPMAddPackage("gh:KhronosGroup/Vulkan-Headers#vulkan-sdk-1.4.304.1")
CPMAddPackage("gh:zeux/volk#vulkan-sdk-1.4.304.1")
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator#v3.2.1")

# Find Vulkan SDK glslc compiler
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS "C:/VulkanSDK/1.4.304.1/Bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Please install the Vulkan SDK at C:/VulkanSDK/1.4.304.1/")
endif()


# Shader compilation
find_program(GLSLC glslc REQUIRED HINTS "${VULKAN_SDK_PATH}/Bin")
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found at ${VULKAN_SDK_PATH}/Bin")
endif()

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/shaders")

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE
    Vulkan::Headers
    volk::volk_headers
    GPUOpen::VulkanMemoryAllocator
    SDL3::SDL3
)

if (PROJECT_IS_TOP_LEVEL)
    add_executable(${PROJECT_NAME}_test src/main.c src/deps.cpp)
    add_dependencies(${PROJECT_NAME}_test shaders)
    target_link_libraries(${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME})
    
    add_dependencies(${PROJECT_NAME}_test Shaders)
    # Ensure shader output directory exists
    add_custom_command(TARGET ${PROJECT_NAME}_test PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}"
      COMMENT "Creating shader output directory: ${SHADER_OUT_DIR}"
    )
    # Copy shaders
    # windows vulkan shaders
    if(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME}_test POST_BUILD
            COMMAND ${GLSLC} -fshader-stage=vert "${SHADER_SRC_DIR}/vert.glsl" -o "${SHADER_OUT_DIR}/vert.spv"
            COMMAND ${GLSLC} -fshader-stage=frag "${SHADER_SRC_DIR}/frag.glsl" -o "${SHADER_OUT_DIR}/frag.spv"
            DEPENDS "${SHADER_SRC_DIR}/vert.glsl" "${SHADER_SRC_DIR}/frag.glsl"
            COMMENT "Compiling shaders to ${SHADER_OUT_DIR}"
        )
    endif()
endif ()