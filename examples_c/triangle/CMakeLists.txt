cmake_minimum_required(VERSION 3.20)
project(VulkanSDL3Project C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_COMPILER "")
set(CMAKE_CXX_FOUND FALSE)

include(FetchContent)

# SDL3
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main
)
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
set(SDL_DIRECTX OFF CACHE BOOL "Disable DirectX" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "Disable OpenGL" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "Disable OpenGLES" FORCE)
FetchContent_MakeAvailable(SDL3)

# cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(cglm)

# Vulkan Headers
FetchContent_Declare(
    Vulkan-Headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG vulkan-sdk-1.4.304.1
)
FetchContent_MakeAvailable(Vulkan-Headers)

# Vulkan Loader
FetchContent_Declare(
    Vulkan-Loader
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG vulkan-sdk-1.4.304.1
)
FetchContent_MakeAvailable(Vulkan-Loader)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} src/main.c)
set_source_files_properties(src/main.c PROPERTIES LANGUAGE C)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    cglm
    Vulkan::Vulkan
)

# Shader compilation
find_program(GLSLC glslc REQUIRED HINTS ENV VULKAN_SDK PATH_SUFFIXES bin)
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets)
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

add_custom_command(
    OUTPUT ${SHADER_OUT_DIR}/frag.spv
    COMMAND ${GLSLC} -fshader-stage=frag ${SHADER_SRC_DIR}/frag.glsl -o ${SHADER_OUT_DIR}/frag.spv
    DEPENDS ${SHADER_SRC_DIR}/frag.glsl
    COMMENT "Compiling frag.glsl to SPIR-V (fragment shader)"
)
add_custom_command(
    OUTPUT ${SHADER_OUT_DIR}/vert.spv
    COMMAND ${GLSLC} -fshader-stage=vert ${SHADER_SRC_DIR}/vert.glsl -o ${SHADER_OUT_DIR}/vert.spv
    DEPENDS ${SHADER_SRC_DIR}/vert.glsl
    COMMENT "Compiling vert.glsl to SPIR-V (vertex shader)"
)

add_custom_target(
    Shaders ALL
    DEPENDS ${SHADER_OUT_DIR}/frag.spv ${SHADER_OUT_DIR}/vert.spv
)
add_dependencies(${PROJECT_NAME} Shaders)

# Copy DLLs to output directory
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying SDL3.dll to output directory"
    )

    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_DLL "$ENV{VULKAN_SDK}/Bin/vulkan-1.dll")
        if(EXISTS ${VULKAN_DLL})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${VULKAN_DLL}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
                COMMENT "Copying vulkan-1.dll from Vulkan SDK to output directory"
            )
        endif()
    endif()
endif()

# Copy shaders to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${SHADER_OUT_DIR}/vert.spv
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/vert.spv
    COMMENT "Copying vert.spv to output directory"
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${SHADER_OUT_DIR}/frag.spv
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/frag.spv
    COMMENT "Copying frag.spv to output directory"
)