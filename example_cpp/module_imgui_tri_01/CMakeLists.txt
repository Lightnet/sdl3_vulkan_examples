cmake_minimum_required(VERSION 3.14)
project(VulkanTriangle CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set all outputs to the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Option to enable/disable validation layers
option(ENABLE_VULKAN_VALIDATION_LAYERS "Enable Vulkan validation layers" ON)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_VULKAN_VALIDATION_LAYERS ON CACHE BOOL "Force validation layers in Debug" FORCE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(ENABLE_VULKAN_VALIDATION_LAYERS OFF CACHE BOOL "Disable validation layers in Release" FORCE)
endif()

# Define VSDL_ENABLE_VALIDATION_LAYERS based on the option
if(ENABLE_VULKAN_VALIDATION_LAYERS)
    add_definitions(-DVSDL_ENABLE_VALIDATION_LAYERS=1)
else()
    add_definitions(-DVSDL_ENABLE_VALIDATION_LAYERS=0)
endif()

# Download CPM.cmake v0.40.7
file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.7/CPM.cmake"
    "${CMAKE_BINARY_DIR}/CPM_0.40.7.cmake"
)
include(${CMAKE_BINARY_DIR}/CPM_0.40.7.cmake)

# Add dependencies with CPM
CPMAddPackage(
    NAME SDL3
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-3.2.8
)

CPMAddPackage(
    NAME VMA
    GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG v3.2.1
)

CPMAddPackage(
    NAME IMGUI
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.91.8
)

CPMAddPackage(
    NAME GLM
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 1.0.1
)

CPMAddPackage(
    NAME VulkanHeaders
    GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
    GIT_TAG vulkan-sdk-1.4.304.1
)

# Find Vulkan from the system
find_package(Vulkan REQUIRED)

# Define ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

# Define the executable with all source files
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/vsdl_init.cpp
    src/vsdl_renderer.cpp
    src/vsdl_mesh.cpp
    src/vsdl_pipeline.cpp
    src/vsdl_cleanup.cpp
    src/vsdl_imgui.cpp
    ${VMA_SOURCE_DIR}/src/VmaUsage.cpp
    ${IMGUI_SOURCES}  # Add ImGui sources
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    Vulkan::Vulkan
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL3_SOURCE_DIR}/include
    ${VMA_SOURCE_DIR}/include
    ${GLM_SOURCE_DIR}
    ${VulkanHeaders_SOURCE_DIR}/include
    ${VMA_SOURCE_DIR}/src/
    ${Vulkan_INCLUDE_DIRS}
    ${IMGUI_SOURCE_DIR}  # ImGui include directory
    ${IMGUI_SOURCE_DIR}/backends  # For backend implementations
)

# Shader handling
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/shaders)

if(NOT EXISTS ${SHADER_SRC_DIR})
    message(FATAL_ERROR "Shaders directory '${SHADER_SRC_DIR}' does not exist. Please create it and add 'tri.vert' and 'tri.frag' files.")
endif()

find_program(GLSLC_EXECUTABLE glslc HINTS "C:/VulkanSDK/1.4.304.1/Bin" "$ENV{VULKAN_SDK}/Bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Ensure Vulkan SDK is installed at C:/VulkanSDK/1.4.304.1/")
endif()

set(SHADER_FILES
    ${SHADER_SRC_DIR}/tri.vert
    ${SHADER_SRC_DIR}/tri.frag
)

foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${SHADER_DEST_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DEST_DIR}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME} to SPIR-V in ${SHADER_DEST_DIR}"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} Shaders)

if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3-shared>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
        COMMENT "Copying SDL3.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
    )
endif()