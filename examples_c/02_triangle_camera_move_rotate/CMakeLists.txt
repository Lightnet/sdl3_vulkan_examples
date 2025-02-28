# Minimum CMake version required for modern features (e.g., FetchContent)
cmake_minimum_required(VERSION 3.20)

# Project name and language (C only, no C++)
project(VulkanSDL3Project C)

# Enforce C11 standard for modern C features (e.g., bool from stdbool.h)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include FetchContent module for downloading dependencies
include(FetchContent)

# Set a cache directory to avoid redownloading dependencies on every build
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps_cache")

# Fetch SDL3 from GitHub
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main  # Use latest main branch for SDL3
)
# Enable Vulkan support, disable unnecessary features to reduce build time
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
FetchContent_MakeAvailable(SDL3) # Download and build SDL3

# Fetch cglm for math operations
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6  # Stable version for consistency
)
FetchContent_MakeAvailable(cglm) # Download and build cglm

# Find Vulkan SDK (assumes installed locally via VULKAN_SDK env var)
find_package(Vulkan REQUIRED) # Links to vulkan-1.lib and headers

# Define the executable target using main.c
add_executable(${PROJECT_NAME} src/main.c)
set_source_files_properties(src/main.c PROPERTIES LANGUAGE C) # Ensure C compilation

# Include directories for Vulkan headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)

# Link libraries: SDL3, cglm, Vulkan
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3      # SDL3 library for windowing/input
    cglm            # cglm for math operations
    Vulkan::Vulkan  # Vulkan library for graphics
)

# Shader compilation setup
find_program(GLSLC glslc REQUIRED HINTS ENV VULKAN_SDK PATH_SUFFIXES bin) # Find glslc from Vulkan SDK
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets) # Source directory for .glsl files
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders) # Output directory for .spv files
file(MAKE_DIRECTORY ${SHADER_OUT_DIR}) # Create shaders directory if it doesn’t exist

# Compile vertex shader
add_custom_command(
    OUTPUT ${SHADER_OUT_DIR}/vert.spv
    COMMAND ${GLSLC} -fshader-stage=vert ${SHADER_SRC_DIR}/vert.glsl -o ${SHADER_OUT_DIR}/vert.spv
    DEPENDS ${SHADER_SRC_DIR}/vert.glsl # Rebuild only if vert.glsl changes
    COMMENT "Compiling vert.glsl to SPIR-V"
)

# Compile fragment shader
add_custom_command(
    OUTPUT ${SHADER_OUT_DIR}/frag.spv
    COMMAND ${GLSLC} -fshader-stage=frag ${SHADER_SRC_DIR}/frag.glsl -o ${SHADER_OUT_DIR}/frag.spv
    DEPENDS ${SHADER_SRC_DIR}/frag.glsl # Rebuild only if frag.glsl changes
    COMMENT "Compiling frag.glsl to SPIR-V"
)

# Custom target to ensure shaders are built before the executable
add_custom_target(Shaders ALL DEPENDS ${SHADER_OUT_DIR}/vert.spv ${SHADER_OUT_DIR}/frag.spv)
add_dependencies(${PROJECT_NAME} Shaders) # Link shaders to main target

# Post-build steps for Windows: Copy DLLs and shaders to output directory
if(WIN32)
    # Copy SDL3.dll dynamically from its build location
    set(SDL3_DLL "$<TARGET_FILE:SDL3::SDL3>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying SDL3.dll from ${SDL3_DLL}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL3_DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E echo "SDL3.dll copied to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying SDL3.dll to output directory"
    )

    # Copy vulkan-1.dll if VULKAN_SDK is set and file exists
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_DLL "$ENV{VULKAN_SDK}/Bin/vulkan-1.dll")
        if(EXISTS ${VULKAN_DLL})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Copying vulkan-1.dll from ${VULKAN_DLL}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VULKAN_DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>
                COMMAND ${CMAKE_COMMAND} -E echo "vulkan-1.dll copied to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying vulkan-1.dll from Vulkan SDK"
            )
        else()
            message(WARNING "vulkan-1.dll not found at ${VULKAN_DLL}. Ensure VULKAN_SDK points to a valid installation.")
        endif()
    else()
        message(WARNING "VULKAN_SDK environment variable not set. Skipping vulkan-1.dll copy.")
    endif()

    # Copy compiled shaders to output directory
    foreach(SHADER vert.spv frag.spv)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${SHADER} from ${SHADER_OUT_DIR}/${SHADER}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_OUT_DIR}/${SHADER} $<TARGET_FILE_DIR:${PROJECT_NAME}>/${SHADER}
            COMMAND ${CMAKE_COMMAND} -E echo "${SHADER} copied to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying ${SHADER} to output directory"
        )
    endforeach()
endif()