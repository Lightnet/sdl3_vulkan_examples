cmake_minimum_required(VERSION 3.20)
project(VulkanSDL3Project C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch SDL3
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main
)
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build static SDL library" FORCE)
FetchContent_MakeAvailable(SDL3)

# Fetch cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(cglm)

# Fetch vk-bootstrap
FetchContent_Declare(
    vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG v1.3.299
)
FetchContent_MakeAvailable(vk_bootstrap)

# Fetch Vulkan Memory Allocator (VMA)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.0.1
)
FetchContent_MakeAvailable(vma)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Executable
add_executable(${PROJECT_NAME} src/main.c src/vulkan_init.cpp)
set_source_files_properties(src/main.c PROPERTIES LANGUAGE C)
set_source_files_properties(src/vulkan_init.cpp PROPERTIES LANGUAGE CXX)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${Vulkan_INCLUDE_DIRS}
    ${vk_bootstrap_SOURCE_DIR}/include
    ${vma_SOURCE_DIR}/include  # Include VMA headers
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    cglm
    vk-bootstrap::vk-bootstrap
    Vulkan::Vulkan
    # No need to link VMA explicitly; it's header-only but requires Vulkan
)

# Shader compilation
find_program(GLSLC glslc REQUIRED HINTS "$ENV{VULKAN_SDK}/Bin")
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Ensure Vulkan SDK is installed and VULKAN_SDK is set.")
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

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} Shaders)

# Post-build steps for Windows
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "C:/Windows/System32/vulkan-1.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_OUT_DIR}/vert.spv" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_OUT_DIR}/frag.spv" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying DLLs and shaders to output directory"
    )
endif()