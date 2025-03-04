

Since everything is working as expected, here’s a quick summary of what we achieved:

- Used SDL3 (release-3.2.6) with the callback system (SDL_MAIN_USE_CALLBACKS) for a clean, modern event loop.
    
- Integrated FreeType (VER-2-13-3) to render "Hello World" with proper glyph spacing and alignment, respecting advance, bearing, and baseline.
    
- Centered the text dynamically in a resizable 640x480 window using SDL3’s renderer and texture functions.
    
- Built the project with CMake, fetching SDL3, cglm (v0.9.6), and FreeType from GitHub.