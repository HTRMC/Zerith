#version 450

layout(binding = 1) uniform sampler2DArray texArray;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in float fragTextureIndex;

layout(location = 0) out vec4 outColor;

void main() {
    // Color faces based on orientation
    vec3 faceColors[6] = vec3[](
    vec3(1.0, 0.0, 0.0),  // Front (-Z) - Red
    vec3(0.0, 1.0, 0.0),  // Back (+Z) - Green
    vec3(0.0, 0.0, 1.0),  // Top (+Y) - Blue
    vec3(1.0, 1.0, 0.0),  // Bottom (-Y) - Yellow
    vec3(1.0, 0.0, 1.0),  // Right (+X) - Magenta
    vec3(0.0, 1.0, 1.0)   // Left (-X) - Cyan
    );

    // Get face index from color passed from vertex shader
    int faceIndex = int(fragColor.x);
    outColor = vec4(faceColors[faceIndex], 1.0);
}