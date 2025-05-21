#version 450

// Input from mesh shader
layout(location = 0) in PerVertexData {
    vec3 color;
    vec2 texCoord;
} v_in;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Simple color output with slight shading based on texture coordinates
    // This creates a subtle gradient across each face
    float shade = 0.8 + 0.2 * dot(v_in.texCoord, vec2(0.5, 0.5));
    outColor = vec4(v_in.color * shade, 1.0);
}