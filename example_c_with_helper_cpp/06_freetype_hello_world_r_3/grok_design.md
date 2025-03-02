# Design Documentation Outline for VulkanSDL3Project

 * Introduction
   * Purpose of the project
   * Overview of technologies used

 * System Architecture
   * High-level component breakdown (SDL3, Vulkan, CMake, etc.)
   * Interaction between components

 * Build Configuration
   * CMake setup and dependency management
   * Library versions and fetch logic

 * Rendering Pipeline
   * Vulkan initialization
   * Object rendering (triangle, cube, text)
   * Shader details and vertex input reflection
   * Camera and uniform buffer management

 * Code Structure
   * File organization and responsibilities
   * Key data structures (e.g., VulkanContext, RenderObject)

 * Implementation Details
   * Text rendering with FreeType
   * Input handling and camera controls
   * Resource cleanup

 * Usage Instructions
   * Building and running the project
   * Interacting with the application

 * Future Improvements
   * Potential enhancements or optimizations

# Detailed Sections

## 1. Introduction
Purpose: This project, VulkanSDL3Project, demonstrates a Vulkan-based rendering application using SDL3 for windowing and input. It renders a colored triangle, a cube, and the text "Hello World" with FreeType, all within a configurable 3D scene controlled by a camera. The project serves as a foundation for learning Vulkan or building more complex graphics applications.
### Technologies:

 * SDL 3.2.4: Window creation and input handling.
 * Vulkan 1.3 (via SDK 1.4.304.1): Graphics API for rendering.
 * CMake 3.20+: Build system with dependency fetching.
 * vk-bootstrap: Simplifies Vulkan instance, device, and swapchain setup.
 * VMA 3.0.1: Memory allocation for Vulkan buffers and images.
 * cglm 0.9.6: Math library for matrices and vectors.
 * SPIRV-Cross: Reflects shader inputs for pipeline creation.
 * FreeType 2.13.2: Font rendering for text.
 * stb_image: Single-header image loading (currently unused but included).

## 2. System Architecture
The application follows a modular design:

 * SDL3: Initializes a window and Vulkan surface, handles events (e.g., WASD camera movement, mouse look).
 * Vulkan: Core rendering engine, managed via VulkanContext struct in main.c. Uses vk-bootstrap for setup and VMA for memory.
 * Rendering Loop: Main loop in main.c updates camera, uniform buffers, and submits draw commands.
 * Dependencies: Fetched via CMake’s FetchContent, integrated into the build.

### Component Interaction:
 * SDL creates a window and surface, passed to init_vulkan (in vulkan_init.cpp).
 * Vulkan initializes via vk-bootstrap, setting up instance, device, swapchain, and queues.
 * VMA allocates buffers (e.g., vertex, uniform) and images (e.g., depth, text texture).
 * Shaders (vert.glsl, frag.glsl) process vertex data and textures, with SPIRV-Cross reflecting inputs.
 * FreeType generates a texture atlas for "Hello World," rendered as a textured plane.

## 3. Build Configuration

CMake Setup (CMakeLists.txt):

 * Requirements: CMake 3.20+, C11, C++17.

 * Vulkan: Uses Vulkan SDK 1.4.304.1 (configurable via VULKAN_SDK_PATH).

 * Dependencies:
   * SDL3 3.2.4: Fetched from GitHub, built with Vulkan support (SDL_VULKAN=ON).
   * cglm 0.9.6: Math library for transformations.
   * vk-bootstrap 1.3.299: Simplifies Vulkan initialization.
   * VMA 3.0.1: Memory management for Vulkan resources.
   * SPIRV-Cross (SDK 1.4.304.1): Static build for shader reflection, GLSL-only.
   * FreeType 2.13.2: Font rendering, minimal configuration (no zlib/png/etc.).
   * stb_image: Single-header fetched from GitHub, copied to src/.

 * Target: VulkanSDL3Project links src/main.c (C) and src/vulkan_init.cpp (C++).
 * Shaders: Compiled from assets/vert.glsl and frag.glsl to SPIR-V using glslc.
 * Post-Build: Copies FiraSans-Bold.ttf, shaders, and DLLs (Windows-specific) to the output directory.

## 4. Rendering Pipeline
Vulkan Initialization (vulkan_init.cpp):
 * Uses vk-bootstrap to create:
   * Instance (Vulkan 1.3, validation layers in debug).
   * Surface via SDL.
   * Physical device (discrete GPU preferred).
   * Logical device and graphics queue.
   * Swapchain (800x600, B8G8R8A8_UNORM, FIFO present mode).

 * Initializes VMA allocator for memory management.

Object Rendering (main.c):
 * Triangle: 3 vertices with position, color, and dummy UV/flag data.
 * Cube: 36 vertices (6 faces, 2 triangles each) with Vulkan-standard winding (clockwise front-facing).
 * Text: "Hello World" rendered via FreeType as a grayscale texture atlas, displayed on a plane with UVs aligned to the baseline.
 * Vertex format: vec3 pos, vec3 color, vec2 uv, float texFlag (reflected via SPIRV-Cross).

Shaders:
 * Vertex (vert.glsl): Transforms positions with model-view-projection matrices, passes color/UV/flag to fragment.
 * Fragment (frag.glsl): Outputs color directly if texFlag < 0.5, else uses texture alpha for white text.

 * Camera and Uniforms:
   * Camera (Camera struct): Position, front, up vectors, yaw/pitch for FPS-style control.

   * Uniform Buffer Object (UBO): mat4 model, view, proj, updated per frame with cglm.

## 5. Code Structure
 * CMakeLists.txt: Build configuration and dependency management.
 * src/main.c: Core logic, Vulkan setup/cleanup, rendering loop, object creation.
 * src/vulkan_init.cpp: Vulkan initialization with vk-bootstrap and VMA.
 * src/vulkan_init.h: Declarations for init_vulkan and allocator.
 * assets/vert.glsl, frag.glsl: Shaders compiled to vert.spv, frag.spv.
 * assets/FiraSans-Bold.ttf: Font file for text rendering.

## 6. Implementation Details
### Text Rendering:
 * FreeType generates a texture atlas for "Hello World" (48px), aligned to a baseline.
 * Atlas copied to a Vulkan texture via staging buffer, rendered on a plane with UVs.
 * Debugged to ensure proper glyph positioning and alpha blending.

### Input Handling:
 * WASD moves camera, mouse (left-click captured) adjusts yaw/pitch.
 * Keys: Tab (toggle rotation), 1 (reset rotation), 2 (reset camera), 4/5/6 (toggle triangle/cube/text).

### Cleanup:
 * All Vulkan resources (swapchain, buffers, images, etc.) destroyed in reverse order, with VMA handling memory.

