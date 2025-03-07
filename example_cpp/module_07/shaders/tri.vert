#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 transform;
} ubo;

void main() {
    // Orthographic projection: map [-1, 1] to screen space
    mat4 ortho = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    gl_Position = ortho * ubo.transform * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}