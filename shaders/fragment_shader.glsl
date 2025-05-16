#version 450

// Input from mesh shader
layout(location = 0) in PerVertexData {
    vec3 color;
} v_in;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(v_in.color, 1.0);
}
