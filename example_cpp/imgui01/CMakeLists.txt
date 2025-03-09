cmake_minimum_required(VERSION 3.14)
project(ImGuiSDL3Vulkan CXX)

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
    NAME ImGui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.91.8  # Use a specific stable version
    DOWNLOAD_ONLY YES  # We'll manually compile ImGui
)

CPMAddPackage(
    NAME VulkanHeaders
    GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
    GIT_TAG vulkan-sdk-1.4.304.1
)

# Find Vulkan from the system
find_package(Vulkan REQUIRED)

# Define ImGui source files
set(IMGUI_DIR ${ImGui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Define the executable with all source files
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${IMGUI_SOURCES}
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
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${VulkanHeaders_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

# Add compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
)

# Copy SDL3 DLLs if needed (Windows-specific)
if(WIN32 AND TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3-shared>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
        COMMENT "Copying SDL3.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
    )
endif()

# Installation rules (optional)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)