cmake_minimum_required(VERSION 3.20)
project(VulkanSDL3Project C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

# Cache dependencies
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps_cache")

# SDL3
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main
)
set(SDL_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
FetchContent_MakeAvailable(SDL3)

# cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(cglm)

# Vulkan (prefer installed SDK)
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
    OUTPUT ${SHADER_OUT_DIR}/vert.spv
    COMMAND ${GLSLC} -fshader-stage=vert ${SHADER_SRC_DIR}/vert.glsl -o ${SHADER_OUT_DIR}/vert.spv
    DEPENDS ${SHADER_SRC_DIR}/vert.glsl
    COMMENT "Compiling vert.glsl"
)
add_custom_command(
    OUTPUT ${SHADER_OUT_DIR}/frag.spv
    COMMAND ${GLSLC} -fshader-stage=frag ${SHADER_SRC_DIR}/frag.glsl -o ${SHADER_OUT_DIR}/frag.spv
    DEPENDS ${SHADER_SRC_DIR}/frag.glsl
    COMMENT "Compiling frag.glsl"
)

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUT_DIR}/vert.spv ${SHADER_OUT_DIR}/frag.spv)
add_dependencies(${PROJECT_NAME} Shaders)

# Copy DLLs and shaders with error checking
if(WIN32)
    # SDL3 DLL
    set(SDL3_DLL "$<TARGET_FILE:SDL3::SDL3>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying SDL3.dll from ${SDL3_DLL}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL3_DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E echo "SDL3.dll copied to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying SDL3.dll"
    )

    # Vulkan DLL
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_DLL "$ENV{VULKAN_SDK}/Bin/vulkan-1.dll")
        if(EXISTS ${VULKAN_DLL})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Copying vulkan-1.dll from ${VULKAN_DLL}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VULKAN_DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>
                COMMAND ${CMAKE_COMMAND} -E echo "vulkan-1.dll copied to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying vulkan-1.dll"
            )
        else()
            message(WARNING "vulkan-1.dll not found at ${VULKAN_DLL}")
        endif()
    else()
        message(WARNING "VULKAN_SDK not defined, skipping vulkan-1.dll copy")
    endif()

    # Shaders
    foreach(SHADER vert.spv frag.spv)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying ${SHADER} from ${SHADER_OUT_DIR}/${SHADER}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_OUT_DIR}/${SHADER} $<TARGET_FILE_DIR:${PROJECT_NAME}>/${SHADER}
            COMMAND ${CMAKE_COMMAND} -E echo "${SHADER} copied to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying ${SHADER}"
        )
    endforeach()
endif()