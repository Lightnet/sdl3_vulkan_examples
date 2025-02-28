# Design Document for Vulkan SDL3 Project (`grok_sdl3_vulkan02`)

This document outlines the design, setup, controls, and implementation details of a Vulkan-based 3D application using SDL3 and cglm in pure C. The goal is to render a simple triangle, allow camera movement and rotation, and enable object manipulation, mimicking a basic 3D model editor.

---

## Project Overview
- **Purpose**: Demonstrate a minimal Vulkan application with a camera-controlled view and an interactive 3D object (triangle) in pure C.
- **Features**:
  - Render a colored triangle at the world origin.
  - Camera movement (WASD) and rotation (mouse with left-click capture, Escape to release).
  - Triangle rotation toggle (Tab) and reset (1 key).
  - Camera reset (2 key).
  - Logging of camera state for debugging.

---

## Setup and Build Steps

### Required Tools and Packages
1. **Vulkan SDK**:
   - **Download**: https://vulkan.lunarg.com/sdk/home
   - **Purpose**: Provides Vulkan headers, libraries (`vulkan-1.dll`), and tools like `glslc` for shader compilation.
   - **Install**: Set `VULKAN_SDK` environment variable (e.g., `C:\VulkanSDK\1.3.290.0`).

2. **CMake**:
   - **Download**: https://cmake.org/download/
   - **Purpose**: Manages build configuration and dependencies.
   - **Version**: 3.20 or higher.

3. **MSVC (Visual Studio 2022)**:
   - **Purpose**: C compiler for Windows.
   - **Install**: Community edition with C/C++ workload.

4. **SDL3**:
   - **Source**: Fetched via CMake from `https://github.com/libsdl-org/SDL.git` (branch: `main`).
   - **Purpose**: Windowing, input handling, and Vulkan surface creation.

5. **cglm**:
   - **Source**: Fetched via CMake from `https://github.com/recp/cglm.git` (tag: `v0.9.6`).
   - **Purpose**: Math library for vectors, matrices, and transformations in C.

6. **Vulkan-Headers and Vulkan-Loader**:
   - **Source**: Fetched via CMake from `https://github.com/KhronosGroup/Vulkan-Headers.git` and `Vulkan-Loader.git` (tag: `vulkan-sdk-1.4.304.1`).
   - **Purpose**: Vulkan API definitions and runtime loader.

### Directory Structure
```
grok_sdl3_vulkan02/
├── assets/
│   ├── vert.glsl
│   └── frag.glsl
├── src/
│   └── main.c
├── CMakeLists.txt
└── build/
    └── (generated build files)
```


### Build Steps
1. **Clone/Setup Repository**:
   - Create the directory structure and place `main.c` and `CMakeLists.txt`.

2. **Configure CMake**:
   ```bash
   cd grok_sdl3_vulkan02/build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

3. Compile Shaders:
```
cd ../assets
glslc -fshader-stage=vert vert.glsl -o ../build/shaders/vert.spv
glslc -fshader-stage=frag frag.glsl -o ../build/shaders/frag.spv
```

4. Build Project:
```
cd ../build
cmake --build . --config Debug
```

5. Run:
```
Debug\VulkanSDL3Project.exe
```
### Controls:
 * Camera Movement
   * W: Move forward (smoothly along front).
   * S: Move backward.
   * A: Move left (perpendicular to front).
   * D: Move right.
   * Logged: Position and front vector.
 * Camera Rotation:
   * Left-click: Capture mouse for rotation.
   * Mouse movement: Rotate yaw (left/right) and pitch (up/down).
   * Escape: Release mouse.
   * Logged: Yaw, pitch, and front vector.
 * Triangle Controls
   * Tab: Toggle auto-rotation (90°/s around Y-axis).
   * 1: Reset triangle rotation to 0°.
   * Logged: Rotation state changes.
 * Camera Reset:
   * 2: Reset camera to initial position (0, 0, 3) and rotation (yaw -90°, pitch 0°).
   * Logged: Reset confirmation.

### Coordinate System and World Design
 * World Space
    * X: Positive right, negative left.
    * Y: Positive up, negative down.
    * Z: Positive into screen (away), negative toward viewer (near).
    * Origin: (0, 0, 0) is the center of the world, where the triangle is positioned.
 * Camera
    * Initial Position: (0, 0, 3) – 3 units along +Z (looking toward origin).
    * Front: (0, 0, -1) – points toward negative Z (into scene).
    * Up: (0, 1, 0) – points toward positive Y.
    * Yaw: -90° (initially looks along -Z).
    * Pitch: 0° (no tilt).
 * Triangle
    * Fixed at (0, 0, 0) in world space, rotates around Y-axis when toggled.

### Required Files and Libraries:
 * main.c
   * Purpose: Main application logic, Vulkan setup, rendering loop, input handling.
   * Dependencies: <SDL3/SDL.h>, <SDL3/SDL_vulkan.h>, <vulkan/vulkan.h>, <cglm/cglm.h>.

 * CMakeLists.txt
   * Purpose: Configures build, fetches dependencies, compiles shaders, copies DLLs/shaders to output.
   * Key Sections
     * FetchContent_Declare: Downloads SDL3, cglm, Vulkan-Headers, Vulkan-Loader.
     * add_custom_command: Compiles vert.glsl and frag.glsl to SPIR-V.
     * target_link_libraries: Links SDL3, cglm, Vulkan.

 * Shaders (vert.glsl, frag.glsl)
   * vert.glsl: Processes vertex positions and colors with UBO transformations.
   * frag.glsl: Outputs vertex colors to the framebuffer.
   * Compiled: To vert.spv and frag.spv using glslc.

 * cglm
   * Role: Provides vector/matrix math (e.g., glm_vec3_add, glm_rotate_y, glm_lookat).
   * How It Works: C library with inline functions for 3D transformations, avoiding C++ dependencies.

### How cglm Works
  cglm is a C library for OpenGL-style math, offering functions for vectors, matrices, and quaternions. Here’s a primer:

#### Basic Operations
 * Add: glm_vec3_add(vec3 a, vec3 b, vec3 dest) – Adds vectors a and b, stores in dest
```c
vec3 a = {1.0f, 2.0f, 3.0f}, b = {4.0f, 5.0f, 6.0f}, result;
glm_vec3_add(a, b, result); // result = {5.0f, 7.0f, 9.0f}
```
 * Multiply (Scale): glm_vec3_scale(vec3 v, float s, vec3 dest) – Scales v by scalar s
```c
glm_vec3_scale(a, 2.0f, result); // result = {2.0f, 4.0f, 6.0f}
```
 * Divide: glm_vec3_div(vec3 a, vec3 b, vec3 dest) – Element-wise division (not commonly used for movement).
```c
glm_vec3_div(a, (vec3){2.0f, 2.0f, 2.0f}, result); // result = {0.5f, 1.0f, 1.5f}
```
* Inverse: glm_mat4_inv(mat4 mat, mat4 dest) – Inverts a 4x4 matrix
```c
mat4 view, invView;
glm_lookat((vec3){0,0,3}, (vec3){0,0,0}, (vec3){0,1,0}, view);
glm_mat4_inv(view, invView);
```
#### Movement Examples
 * Move Forward: Scale front by speed, add to position.
```c
vec3 move; glm_vec3_scale(cam->front, speed, move); glm_vec3_add(cam->pos, move, cam->pos);
```
 * Move Left: Cross front with up, normalize, scale, subtract
```c
vec3 left; glm_vec3_cross(cam->front, cam->up, left); glm_vec3_normalize(left); glm_vec3_scale(left, -speed, left); glm_vec3_add(cam->pos, left, cam->pos);
```
 * Move Back: Scale front negatively, add
```c
vec3 move; glm_vec3_scale(cam->front, -speed, move); glm_vec3_add(cam->pos, move, cam->pos);
```
#### Rotation
 * Object Rotation: Use glm_rotate_y(mat4 mat, float angle, mat4 dest) (angle in radians)
```c
glm_rotate_y(ubo.model, glm_rad(45.0f), ubo.model); // Rotate 45° around Y
```
 * Radian Conversion: glm_rad(degrees) converts degrees to radians.
```c
float rad = glm_rad(90.0f); // 1.5708 radians
```
 * Camera Rotation: Update front based on yaw/pitch (degrees to radians).
```c
front[0] = cosf(glm_rad(yaw)) * cosf(glm_rad(pitch));
front[1] = sinf(glm_rad(pitch));
front[2] = sinf(glm_rad(yaw)) * cosf(glm_rad(pitch));
```

### Vulkan Setup in C: Step-by-Step

#### Steps and Parameters

  1. Instance Creation (vkCreateInstance):
      * Parameters: VkInstanceCreateInfo with app info, extensions (e.g., VK_KHR_SURFACE_EXTENSION_NAME).
      * Purpose: Initializes Vulkan.

  2. Surface Creation (SDL_Vulkan_CreateSurface):
      * Parameters: Window, instance, surface pointer.
      * Purpose: Links SDL window to Vulkan for rendering.

  3. Physical Device (vkEnumeratePhysicalDevices):
      * Parameters: Instance, device count, device array
      * Purpose: Selects GPU (simplified to first device).

  4. Logical Device(vkCreateDevice):
      * Parameters: VkDeviceCreateInfo with queue info, extensions (e.g., VK_KHR_SWAPCHAIN_EXTENSION_NAME).
      * Purpose: Creates a device with graphics queue.

  5. Swapchain (vkCreateSwapchainKHR):
      * Parameters: VkSwapchainCreateInfoKHR with surface, format, extent.
      * Purpose: Manages framebuffers for rendering.

  6. Render Pass (vkCreateRenderPass):
      * Parameters: VkRenderPassCreateInfo with attachments (color).
      * Purpose: Defines rendering pipeline stages.

  7. Pipeline (vkCreateGraphicsPipelines):
      * Parameters: VkGraphicsPipelineCreateInfo with shaders, vertex input, layout.
      * Purpose: Sets up shading and rendering.

  8. Buffers (vkCreateBuffer, vkAllocateMemory):
      * Parameters: Size, usage (e.g., VK_BUFFER_USAGE_VERTEX_BUFFER_BIT).
      * Purpose: Stores vertex and uniform data.

  9. Command Buffers (vkAllocateCommandBuffers):
      * Parameters: VkCommandBufferAllocateInfo with pool.
      * Purpose: Records rendering commands.

  10. Sync Objects (vkCreateSemaphore, vkCreateFence):
      * Parameters: Basic create infos.
      * Purpose: Synchronizes GPU operations.

##### Debugging
 * Validation Layers: Add VK_LAYER_KHRONOS_validation to createInfo.enabledLayerNames.
 * Print Statements: Use printf to trace execution (e.g., after Vulkan calls).
 * Check Results: Every Vulkan call returns VkResult—check != VK_SUCCESS.


### Creating a Triangle in Vulkan

#### Steps
 1. Define Vertices
```c
float vertices[] = {x, y, z, r, g, b, ...};
```
 * 6 floats per vertex: position (x, y, z), color (r, g, b).

2. Create Buffer:
 * vkCreateBuffer with VK_BUFFER_USAGE_VERTEX_BUFFER_BIT.

3. Allocate Memory:
 * vkAllocateMemory with host-visible/coherent properties

4. Bind and Fill:
 * vkBindBufferMemory, vkMapMemory, memcpy, vkUnmapMemory.

5. Bind in Command Buffer
 * vkCmdBindVertexBuffers before vkCmdDraw.

#### Adding More Objects (e.g., Box)
 * Vertices: Define a box’s 8 vertices, use indices for triangles (36 indices for 12 triangles).
 * Index Buffer: Create a separate buffer with VK_BUFFER_USAGE_INDEX_BUFFER_BIT, bind with vkCmdBindIndexBuffer.
 * Draw: Use vkCmdDrawIndexed instead of vkCmdDraw.

#### Shader Management

##### Adding/Removing Shaders
 * Add: Load new SPIR-V files, create new VkShaderModule, update pipelineInfo.pStages.
 * Remove: Destroy old modules (vkDestroyShaderModule), recreate pipeline.

##### AI Agent Design
 * Purpose: Assist in debugging, generating code, and explaining Vulkan setup.
 * Features
   * Parse main.c, CMakeLists.txt, and shaders.
   * Suggest fixes (e.g., API updates, Vulkan errors).
   * Generate examples (e.g., box vertices, rotations).
 * Implementation: Use natural language processing to interpret requests, maintain state of project files, and output C code or Markdown explanations.

##### Reading the Triangle Code

```c
void create_triangle() {
    float vertices[] = {
        0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // Top, red
       -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // Bottom-left, green
        0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f  // Bottom-right, blue
    };
```
 * Line-by-Line:
    * float vertices[]: Array of 18 floats (3 vertices × 6 floats each).
    * 0.0f, -0.5f, 0.0f: Top vertex position (x=0, y=-0.5, z=0).
    * 1.0f, 0.0f, 0.0f: Top vertex color (red=1, green=0, blue=0).
    * -0.5f, 0.5f, 0.0f: Bottom-left position (x=-0.5, y=0.5, z=0).
    * 0.0f, 1.0f, 0.0f: Bottom-left color (green).
    * 0.5f, 0.5f, 0.0f: Bottom-right position (x=0.5, y=0.5, z=0).
    * 0.0f, 0.0f, 1.0f: Bottom-right color (blue).
