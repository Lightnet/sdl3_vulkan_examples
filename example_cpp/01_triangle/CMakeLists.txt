cmake_minimum_required(VERSION 3.15)
project(VulkanTriangle CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Display build directory (won't resolve config here)
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "Build configuration will be determined at build time (e.g., Debug or Release via --config)")

# Try your test (won't work as expected at config time)
set(CUSTOM_BUILD_TYPE $<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)
message(STATUS "CUSTOM_BUILD_TYPE: ${CUSTOM_BUILD_TYPE}") # Will print unresolved expression

# Specify Vulkan SDK path (for glslc shader compilation)
set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.304.1")
set(GLSLC "${VULKAN_SDK_PATH}/Bin/glslc.exe")

# Verify glslc exists
if(NOT EXISTS ${GLSLC})
    message(FATAL_ERROR "glslc not found at ${GLSLC}. Please check Vulkan SDK installation.")
endif()

# Fetch SDL3
include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.6
)
FetchContent_MakeAvailable(SDL3)

# Fetch Vulkan Headers
FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG v1.4.309
)
FetchContent_MakeAvailable(VulkanHeaders)

# Fetch GLM
FetchContent_Declare(
    GLM
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(GLM)

# Fetch Volk
FetchContent_Declare(
    Volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG 1.4.304
)
FetchContent_MakeAvailable(Volk)

# Create executable
add_executable(${PROJECT_NAME} main.cpp)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    volk
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${vulkanheaders_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
    ${volk_SOURCE_DIR}
)

# Define Volk static linking
target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_NO_PROTOTYPES
)

# Shader compilation with glslc
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_FILES
    ${SHADER_DIR}/triangle.vert
    ${SHADER_DIR}/triangle.frag
)
set(SPIRV_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)

# Ensure shader directory and files exist
if(NOT EXISTS ${SHADER_DIR})
    message(FATAL_ERROR "Shader directory ${SHADER_DIR} does not exist. Please create it and add shader files.")
endif()

foreach(SHADER ${SHADER_FILES})
    if(NOT EXISTS ${SHADER})
        message(FATAL_ERROR "Shader file ${SHADER} not found. Please ensure it exists in ${SHADER_DIR}.")
    endif()
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV_OUTPUT ${SPIRV_OUTPUT_DIR}/${SHADER_NAME}.spv)
    
    # Determine shader stage
    if(SHADER_NAME MATCHES ".vert$")
        set(SHADER_STAGE "vert")
    elseif(SHADER_NAME MATCHES ".frag$")
        set(SHADER_STAGE "frag")
    else()
        message(FATAL_ERROR "Unknown shader type for ${SHADER_NAME}")
    endif()

    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT}
        COMMAND ${GLSLC} -fshader-stage=${SHADER_STAGE} ${SHADER} -o ${SPIRV_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME} to SPIR-V with glslc"
        VERBATIM
    )
    list(APPEND SPIRV_FILES ${SPIRV_OUTPUT})
endforeach()

add_custom_target(Shaders DEPENDS ${SPIRV_FILES})
add_dependencies(${PROJECT_NAME} Shaders)

# Copy SPIR-V files to runtime directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPIRV_FILES} "${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/shaders"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying SPIR-V shaders to ${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/shaders"
    COMMENT "Copying SPIR-V shaders (Build Type? $<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)"
)

# Copy SDL3 DLL to output directory
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            "${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
        COMMAND ${CMAKE_COMMAND} -E echo "Copying SDL3.dll to ${CMAKE_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
        COMMENT "Copying SDL3.dll (Build Type? $<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)"
    )
endif()