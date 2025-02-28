Visual Confirmation

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

Spin Check
   * Tab: Rotates around Y-axis:
   * Red (front) → Purple (left) → White (back) → Cyan (right) → Yellow (top) → Green (bottom).
   * All outward faces visible, matching Vulkan standard.

Why It Works
   * Y-Flip: ubo.proj[1][1] *= -1 adjusts for Vulkan’s Y-down NDC, flipping the cube’s Y-orientation.
   * Front Face: VK_FRONT_FACE_CLOCKWISE ensures CW triangles (outward faces after Y-flip) are front-facing, not culled.
   * World Space: Remains Y-up (intuitive), with camera up as (0, 1, 0).

Standard Vulkan Build Locked In
   * Projection: Always flip Y with ubo.proj[1][1] *= -1 for Vulkan.
   * Pipeline: Use VK_FRONT_FACE_CLOCKWISE with VK_CULL_MODE_BACK_BIT when Y is flipped.
   * Cube: Define vertices CCW from outside in world space; the Y-flip handles screen-space adjustment.

