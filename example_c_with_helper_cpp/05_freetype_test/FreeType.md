Required Dependencies to Build FreeType
For a minimal build (which should suffice for your needs—rendering TTF fonts into bitmaps for Vulkan textures), FreeType has no mandatory external library dependencies. Here’s what you need at its core:
C Compiler:
FreeType is written in C, so you need a C compiler (e.g., gcc, clang, or MSVC). Since your project already builds with MSVC (Visual Studio 16 2019), this is covered.

Required: Already satisfied by your CMake setup (cmake -G "Visual Studio 16 2019").

Standard C Library:
FreeType relies on standard C functions (malloc, free, memcpy, etc.), provided by your platform’s C runtime (CRT).

Required: Included with MSVC and Windows (e.g., msvcrt.dll).

Build System:
FreeType supports multiple build systems: CMake, GNU Make, or its own custom build scripts. Since your project uses CMake, we’ll use FreeType’s CMake support.

Required: CMake 3.20 (already in use).

Minimal Build
With just these, you can build FreeType’s core functionality (TTF rendering without extras like PNG or zlib support). This is sufficient for generating glyph bitmaps, which you can then upload to Vulkan textures using VMA.
Optional Dependencies
FreeType has optional features that require external libraries. You can disable these if not needed (recommended for simplicity in your case). Here’s the list:
zlib (Optional):
Purpose: Enables support for compressed font files (e.g., .ttf.gz or WOFF fonts with zlib compression).

Need: Not required unless you plan to load compressed fonts (most TTF files are uncompressed).

Build Impact: Adds FT_CONFIG_OPTION_USE_ZLIB to the config; requires linking zlib.lib.

Recommendation: Disable (-DFT_CONFIG_OPTION_USE_ZLIB=OFF in CMake) unless needed.

libpng (Optional):
Purpose: Enables PNG glyph support (e.g., for color emoji fonts like OpenMoji).

Need: Only if you want to render PNG-based glyphs (rare for basic text).

Build Impact: Adds FT_CONFIG_OPTION_USE_PNG; requires linking libpng.lib.

Recommendation: Disable (-DFT_CONFIG_OPTION_USE_PNG=OFF) for simplicity.

HarfBuzz (Optional):
Purpose: Enables advanced text shaping (e.g., kerning, ligatures, right-to-left scripts).

Need: Useful for complex typography, but overkill for basic text rendering.

Build Impact: Adds FT_CONFIG_OPTION_USE_HARFBUZZ; requires linking harfbuzz.lib.

Recommendation: Disable (-DFT_CONFIG_OPTION_USE_HARFBUZZ=OFF) unless you need text layout.

Brotli (Optional):
Purpose: Supports Brotli-compressed WOFF2 fonts.

Need: Rare for standard TTF files; only needed for web font formats.

Build Impact: Adds FT_CONFIG_OPTION_USE_BROTLI; requires linking brotli.lib.

Recommendation: Disable (-DFT_CONFIG_OPTION_USE_BROTLI=OFF).

BZip2 (Optional):
Purpose: Supports BZip2-compressed fonts (uncommon).

Need: Not typically needed for TTF files.

Build Impact: Adds FT_CONFIG_OPTION_USE_BZIP2; requires linking bzip2.lib.

Recommendation: Disable (-DFT_CONFIG_OPTION_USE_BZIP2=OFF).

Platform-Specific Notes (Windows)
Windows CRT: FreeType uses standard C functions provided by MSVC’s CRT (msvcrt.dll), so no additional setup is needed.

No POSIX: FreeType avoids POSIX-specific dependencies, making it fully compatible with Windows without extra libs.

Building FreeType for Your Project
Since your project uses CMake, I’ll provide a minimal setup to build FreeType with no optional dependencies, ensuring it integrates seamlessly with VulkanSDL3Project.
Updated CMakeLists.txt Snippet
Add this to your existing CMakeLists.txt:
cmake

# Fetch and configure FreeType for font rendering
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-2 # Latest stable version as of Feb 2025
)
# Disable optional features to keep it minimal
set(FT_DISABLE_ZLIB ON CACHE BOOL "Disable zlib" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "Disable libpng" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "Disable HarfBuzz" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "Disable Brotli" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "Disable BZip2" FORCE)
FetchContent_MakeAvailable(freetype)

# Link FreeType to the project
target_link_libraries(${PROJECT_NAME} PRIVATE freetype)
target_include_directories(${PROJECT_NAME} PRIVATE "${freetype_SOURCE_DIR}/include")

Dependencies Satisfied
CMake: Already using 3.20.

MSVC: Your Visual Studio 16 2019 setup provides the C compiler and CRT.

No External Libs: With optional features disabled, no additional downloads or linking beyond FreeType’s source are needed.

