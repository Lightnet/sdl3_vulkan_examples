cmake_minimum_required(VERSION 3.26)
project(vk_sdl C CXX)

# Vulkan SDK setup (for glslc)
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

# SDL3 and Vulkan dependencies
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.8")
CPMAddPackage("gh:KhronosGroup/Vulkan-Headers#vulkan-sdk-1.4.304.1")
CPMAddPackage(
    NAME Vulkan-Loader
    GITHUB_REPOSITORY "KhronosGroup/Vulkan-Loader"
    GIT_TAG "vulkan-sdk-1.4.304.1"
    OPTIONS
        "BUILD_TESTS OFF"  # Disable tests to speed up build
        #"CMAKE_BUILD_TYPE Debug"  # Match your config for initial build
)
CPMAddPackage("gh:zeux/volk#vulkan-sdk-1.4.304.1")
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator#v3.2.1")

# Find Vulkan SDK glslc compiler
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS "${VULKAN_SDK_PATH}/Bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Please install the Vulkan SDK at ${VULKAN_SDK_PATH}")
endif()

# Shader compilation
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/shaders")

# Define shaders to compile
set(SHADERS
    ${SHADER_SRC_DIR}/triangle.vert
    ${SHADER_SRC_DIR}/triangle.frag
)

# Compile shaders to SPIR-V
foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${SHADER_OUT_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(Shaders DEPENDS ${SHADER_OUTPUTS})

# Vulkan-Loader library path, adjusted for Debug/Release
#set(VULKAN_LOADER_LIB "${CMAKE_BINARY_DIR}/$<CONFIG>/vulkan-1$<IF:$<CONFIG:Debug>,d,>.lib")
set(VULKAN_LOADER_LIB "${CMAKE_BINARY_DIR}/$<CONFIG>/vulkan-1.lib")

add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE
    Vulkan::Headers
    "${VULKAN_LOADER_LIB}"  # Links vulkan-1d.lib for Debug, vulkan-1.lib for Release
    volk::volk_headers
    GPUOpen::VulkanMemoryAllocator
    SDL3::SDL3
)
if (${BUILD_SHARED_LIBS})
  message(STATUS "SHARED_LIBS ON!")
else()
  message(STATUS "SHARED_LIBS OFF!")
endif()


if (PROJECT_IS_TOP_LEVEL)
    add_executable(${PROJECT_NAME}_test src/main.c src/deps.cpp)
    add_dependencies(${PROJECT_NAME}_test Shaders vulkan)  # Use 'vulkan' target from Vulkan-Loader
    target_link_libraries(${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME})
    
    # Ensure shader output directory exists
    add_custom_command(TARGET ${PROJECT_NAME}_test PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}"
        COMMENT "Creating shader output directory: ${SHADER_OUT_DIR}"
    )
    # Copy compiled shaders to the binary directory
    add_custom_command(TARGET ${PROJECT_NAME}_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SHADER_OUT_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}_test>/shaders"
        COMMENT "Copying shaders to binary directory"
    )
endif ()