#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragTexFlag;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D textSampler;

void main() {
    if (fragTexFlag > 0.5) {
        float alpha = texture(textSampler, fragTexCoord).r;
        outColor = vec4(alpha, 0.0, 0.0, 1.0); // Red where text should be, black background
    } else {
        outColor = vec4(fragColor, 1.0);
    }
}