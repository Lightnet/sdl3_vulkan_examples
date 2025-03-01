#version 450
// vert.glsl: Vertex shader for rendering 3D objects with position and color
// Purpose: Transforms vertex positions and passes colors to the fragment shader

// Input attributes from vertex buffer
layout(location = 0) in vec3 inPosition; // Vertex position (x, y, z)
layout(location = 1) in vec3 inColor;    // Vertex color (r, g, b)

// Uniform buffer for transformation matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model; // Model transformation (e.g., rotation)
    mat4 view;  // Camera view transformation
    mat4 proj;  // Perspective projection
} ubo;

// Output to fragment shader
layout(location = 0) out vec3 fragColor; // Color passed to fragment stage

void main() {
    // Transform position from model space to clip space
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor; // Pass color directly to fragment shader
}

// Notes for Users:
// - Matches vertex data in main.c: 3 floats for position, 3 for color (6 floats per vertex).
// - Location 0 and 1 must match reflect_vertex_inputs() in main.c.
// - UBO binding 0 matches descriptor set in main.c.
// - Compile with glslc: `glslc -fshader-stage=vert vert.glsl -o vert.spv`.