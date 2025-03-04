cmake_minimum_required(VERSION 3.16)
project(HelloSDL3Text C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(CONFIG_TYPE IN ITEMS Debug Release MinSizeRel RelWithDebInfo)
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/bin/${CONFIG_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG_TYPE})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG_TYPE})
endforeach()

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.6
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(cglm)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
)
set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable zlib in FreeType" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable bzip2 in FreeType" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "Disable PNG in FreeType" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz in FreeType" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli in FreeType" FORCE)
FetchContent_MakeAvailable(freetype)

add_executable(app src/main.c)

# Link SDL3 explicitly using the fetched target
target_link_libraries(app PRIVATE SDL3::SDL3 cglm freetype)

# Optional: Uncomment if you remove the #define from main.c
# target_compile_definitions(app PRIVATE SDL_MAIN_USE_CALLBACKS)

if(WIN32)
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:app>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:freetype>
            $<TARGET_FILE_DIR:app>
        COMMENT "Copying SDL3 and FreeType DLLs to output directory"
    )
endif()

# Add diagnostic message to verify SDL3 location
message(STATUS "SDL3 source dir: ${SDL3_SOURCE_DIR}")
message(STATUS "SDL3 binary dir: ${SDL3_BINARY_DIR}")