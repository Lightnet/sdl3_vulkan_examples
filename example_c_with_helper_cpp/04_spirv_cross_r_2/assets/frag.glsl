#version 450
// frag.glsl: Fragment shader for coloring fragments
// Purpose: Outputs the final color for each pixel based on vertex input

// Input from vertex shader
layout(location = 0) in vec3 fragColor; // Interpolated color from vertex stage

// Output to color attachment
layout(location = 0) out vec4 outColor; // Final fragment color (r, g, b, a)

void main() {
    outColor = vec4(fragColor, 1.0); // Set color with full opacity
}

// Notes for Users:
// - Takes fragColor from vert.glsl; location 0 matches vertex output.
// - Outputs to attachment 0 (swapchain image).
// - Simple pass-through; add lighting or effects here if needed.
// - Compile with glslc: `glslc -fshader-stage=frag frag.glsl -o frag.spv`.