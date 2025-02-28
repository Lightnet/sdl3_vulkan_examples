# sdl3_vulkan_examples

# License: MIT

# Informtion:
  This document implementation details of a Vulkan-based 3D application using SDL3 and cglm in pure C.
  
  This is for reference build to understand how it works. To compile and build application.
  
  Using the Grok Beta 3 AI Agent to help debug the error and improve code logic. Using the Grok and did ask to write up on each part how to Vulkan works.
 
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
 * python 3.13
 * nasm 2.16.03 (https://www.nasm.us/)
 * vulkan sdk for compile shader


 
# C Language:
 Testing the limited of AI Agent.
 
## Notes:
 * Using the pure C to make SDL3 and Vulkan build to create Triangle. Note there are some required tools that SDL3 use as well Vulkan libs.
 * 
 
# C++ Language:
 * N/A
 
# Refs:
 * https://vulkan-tutorial.com
   * Using the AI agent to reference vertex set up.

# Grok:
 Below is Grok wrote docs. This help refs how it work.   

# cglm

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

# Lesson: Building Vulkan Correctly in Pure C
  Here’s a structured lesson based on our journey, tailored for pure C:

## Vulkan Initialization Order
 * Steps: Instance → Surface → Physical Device → Logical Device → Queues → Swapchain → Render Pass → Pipeline → Buffers → Command Buffers → Sync Objects.
 * Why: Vulkan requires objects to be created in a dependency order. E.g., the swapchain needs a surface, and the pipeline needs a render pass.
 * C Challenge: Manage memory manually (e.g., malloc/free) for arrays like swapchainImages.

## Shaders and Pipeline Compatibility
 * Key: Shaders define what data (inputs, uniforms) the pipeline needs. The pipeline layout and descriptor sets must match these exactly.
 * What Went Wrong: Original shaders expected a UBO, but the pipeline didn’t provide it. Always align VkPipelineLayoutCreateInfo with shader requirements.
 * C Tip: Use simple file I/O (fopen, fread) to load SPIR-V, avoiding C++ dependencies.
## Resource Management
 * Buffers: Create (vkCreateBuffer), allocate memory (vkAllocateMemory), bind (vkBindBufferMemory), map (vkMapMemory) for data.
 * Descriptors: Define layouts (VkDescriptorSetLayout), allocate sets (vkAllocateDescriptorSets), update with resources (vkUpdateDescriptorSets).
 * C Approach: Use structs to group related Vulkan handles (e.g., VulkanContext), initialize to zero (= {0}), and clean up in reverse order.
## Render Loop
 * Steps: Acquire image → Reset/record command buffer → Submit → Present → Wait for fences.
 * Sync: Use semaphores for GPU-GPU sync (image acquisition, presentation), fences for CPU-GPU sync (frame completion).
 * C Pitfall: Avoid C++ exceptions—check every Vulkan call with != VK_SUCCESS and exit with printf

## Input Handling in SDL3
 * Events: Poll with SDL_PollEvent, handle SDL_EVENT_KEY_DOWN, SDL_EVENT_MOUSE_MOTION, etc.
 * Mouse Capture: SDL_SetRelativeMouseMode toggles relative mode for smooth rotation.
 * C Strategy: Pass state (e.g., mouseCaptured) as pointers to functions to modify it outside the event loop.

## Debugging
 * Validation Layers: Enable VK_LAYER_KHRONOS_validation in VkInstanceCreateInfo (optional for now) to catch issues early.
 * Printf: Sprinkle printf statements to trace execution flow (e.g., "Drawing triangle").
 
# Step 1: Understanding the Cube and Vertex Layout
  First, let’s define the cube’s 8 vertices and visualize their positions. The cube is centered at (0, 0, 0) with a size of 1 unit (from -0.5 to 0.5 on each axis).

## Vertex Diagram
```
       v3--------v2
      /|         /|      Y
     / |        / |      |  Z
    v7--------v6  |      | /
    |  v0-----|-v1      |/___X
    | /       | /
    |/        |/
    v4--------v5
```
 * Vertices (x, y, z, r, g, b)
    * v0: (-0.5, -0.5,  0.5, 1.0, 0.0, 0.0) // Front bottom-left, red
    * v1: ( 0.5, -0.5,  0.5, 0.0, 1.0, 0.0) // Front bottom-right, green
    * v2: ( 0.5,  0.5,  0.5, 0.0, 0.0, 1.0) // Front top-right, blue
    * v3: (-0.5,  0.5,  0.5, 1.0, 1.0, 0.0) // Front top-left, yellow
    * v4: (-0.5, -0.5, -0.5, 1.0, 0.0, 1.0) // Back bottom-left, purple
    * v5: ( 0.5, -0.5, -0.5, 0.0, 1.0, 1.0) // Back bottom-right, cyan
    * v6: ( 0.5,  0.5, -0.5, 1.0, 1.0, 1.0) // Back top-right, white
    * v7: (-0.5,  0.5, -0.5, 0.5, 0.5, 0.5) // Back top-left, gray

# Face Directions:
 * Front: Z = 0.5 (toward camera at Z=3)
 * Back: Z = -0.5
 * Left: X = -0.5
 * Right: X = 0.5
 * Top: Y = 0.5
 * Bottom: Y = -0.5


# Step 2: Defining Faces with Vertex Order
 Each face is 2 triangles (6 vertices total in the vertex buffer). Vulkan needs the order to be counterclockwise (CCW) when viewed from outside to match VK_FRONT_FACE_COUNTER_CLOCKWISE.

## Face Diagrams
 1. Front Face (Z=0.5):
```
v3-------v2
|     /    |
|  /       |
v0-------v1
```
 * Triangles: v0-v2-v1, v0-v3-v2 (CCW from Z>0.5)
 * Order: Bottom-left → Top-right → Bottom-right, then Bottom-left → Top-left → Top-right

2. Back Face (Z=-0.5):
```
v7-------v6
|     /    |
|  /       |
v4-------v5
```
 * Triangles: v4-v5-v6, v4-v6-v7 (CCW from Z<-0.5)
 * Order: Bottom-left → Bottom-right → Top-right, then Bottom-left → Top-right → Top-left


3. Left Face (X=-0.5):
```
v7-------v3
|     /    |
|  /       |
v4-------v0
```
 * Triangles: v4-v0-v3, v4-v3-v7 (CCW from X<-0.5)
 * Order: Bottom-back → Bottom-front → Top-front, then Bottom-back → Top-front → Top-back

4. Right Face (X=0.5)
```
v6-------v2
|     /    |
|  /       |
v5-------v1
```
 * Triangles: v5-v1-v2, v5-v2-v6 (CCW from X>0.5)
 * Order: Bottom-back → Bottom-front → Top-front, then Bottom-back → Top-front → Top-back

5. Top Face (Y=0.5):
```
v7-------v6
|     /    |
|  /       |
v3-------v2
```
 * Triangles: v3-v6-v2, v3-v7-v6 (CCW from Y>0.5)
 * Order: Front-left → Back-right → Front-right, then Front-left → Back-left → Back-right

6. Bottom Face (Y=-0.5):
```
v4-------v5
|     /    |
|  /       |
v0-------v1
```
 * Triangles: v0-v4-v5, v0-v5-v1 (CCW from Y<-0.5)
 * Order: Front-left → Back-left → Back-right, then Front-left → Back-right → Front-right

# Step 3: Vulkan Settings for Outward Facing
 Here’s how Vulkan renders these faces outward:

 1. Vertex Buffer
   * We list all 36 vertices (6 per face) in the order above. No index buffer—each vertex is repeated as needed.

 2. Pipeline Config (in create_pipeline):
   * rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE:
      * Defines "front" as CCW when viewed from outside.
   * rasterizer.cullMode = VK_CULL_MODE_BACK_BIT
      * Culls back faces (CW from outside), keeping front faces (CCW).
   * depthStencil.depthTestEnable = VK_TRUE
      * Ensures closer faces (e.g., front Z=0.5) obscure farther ones (e.g., back Z=-0.5).
   * depthStencil.depthCompareOp = VK_COMPARE_OP_LESS
      * Fragments with lower Z (closer to camera) pass.
 3. Camera
   * At (0, 0, 3) looking toward (0, 0, 0), the front face (Z=0.5) is closest and should render red.

# Step-by-Step Guide to Build a Cube Correctly
 Let’s build it visually, step by step:

 1. Define Vertices
   * Assign 8 corners (v0-v7) as shown in the first diagram.
   * Give each a unique color for tracking (e.g., v0 red, v1 green).
 2. Pick a Face (e.g., Front)
   * Vertices: v0 (bottom-left), v1 (bottom-right), v2 (top-right), v3 (top-left).
   * Direction: From camera (Z>0.5), it’s outward.
   * Triangles:
```
v3-------v2
|     /    |
|  /       |
v0-------v1
```
   * v0-v2-v1: Bottom-left → Top-right → Bottom-right (CCW from outside).
   * v0-v3-v2: Bottom-left → Top-left → Top-right (CCW from outside).

 3. Check Winding
   * Stand outside (Z>0.5), trace v0-v2-v1
     * v0 (-0.5, -0.5, 0.5) → v2 (0.5, 0.5, 0.5) → v1 (0.5, -0.5, 0.5).
     * Moves up-right, then down—CCW loop.

 4. Repeat for Each Face
     * Back (Z=-0.5): v4-v5-v6, v4-v6-v7 (CCW from Z<-0.5).
     * Left (X=-0.5): v0-v4-v7, v0-v7-v3 (CCW from X<-0.5).
     * Right (X=0.5): v1-v2-v6, v1-v6-v5 (CCW from X>0.5).
     * Top (Y=0.5): v3-v6-v2, v3-v7-v6 (CCW from Y>0.5).
     * Bottom (Y=-0.5): v0-v4-v5, v0-v5-v1 (CCW from Y<-0.5).
     * For each, stand outside (e.g., X<-0.5 for left) and ensure the path is CCW.

  5. Write the Vertex Buffer
     * List all 36 vertices in order (6 per face), as in the updated create_cube.

  6. Set Vulkan Pipeline
     * frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE
     * cullMode = VK_CULL_MODE_BACK_BIT
     * Enable depth testing (VK_COMPARE_OP_LESS).

  7. Test and Adjust
     * Run, check each face (spin with Tab).
     * If a face is missing, reverse its triangle order (e.g., v0-v1-v2 → v0-v2-v1).


# Progressing Face by Face

You don’t need to build faces in a specific order (e.g., front then back), but here’s a logical progression:
 * Start with Front: Closest to camera, easiest to test (Z=0.5).
 * Back: Opposite side (Z=-0.5), mirrors front.
 * Left/Right: Side faces (X=±0.5), connect front and back.
 * Top/Bottom: Cap it off (Y=±0.5).

For each:
 * Draw the quad (e.g., v0-v1-v2-v3 for front).
 * Split into triangles (v0-v2-v1, v0-v3-v2).
 * Check winding from outside (CCW).
 * Add to vertex array.

# Visual Example: Front Face
```
v3(yellow)---v2(blue)
|         /      |
|      /         |
v0(red)----v1(green)
```
 * Outside: Z>0.5 (camera side).
 * Path: v0 → v2 → v1 (red → blue → green).
 * Check: From Z=3, moves up-right, then down-left—CCW.



# Visual Confirmation
Front Face (Z=0.5)
```
v3(yellow)---v2(blue)    World Space: CCW from Z>0.5
|         /      |       Screen Space (post Y-flip): CW
|      /         |       Kept because VK_FRONT_FACE_CLOCKWISE
v0(red)----v1(green)
```
 * Camera View: From (0, 0, 3), red face faces you, oriented correctly:
    * Red (v0): Bottom-left
    * Green (v1): Bottom-right
    * Blue (v2): Top-right
    * Yellow (v3): Top-left

 * Spin Check
   * Tab: Rotates around Y-axis:
      * Red (front) → Purple (left) → White (back) → Cyan (right) → Yellow (top) → Green (bottom).
   * All outward faces visible, matching Vulkan standard.


# Why It Works
 * Y-Flip: ubo.proj[1][1] *= -1 adjusts for Vulkan’s Y-down NDC, flipping the cube’s Y-orientation.
 * Front Face: VK_FRONT_FACE_CLOCKWISE ensures CW triangles (outward faces after Y-flip) are front-facing, not culled.
 * World Space: Remains Y-up (intuitive), with camera up as (0, 1, 0).

