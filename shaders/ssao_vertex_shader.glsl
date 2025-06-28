#version 450

// Output to fragment shader
layout(location = 0) out vec2 texCoord;

void main() {
    // Generate a full-screen triangle
    // Vertex 0: (-1, -1)
    // Vertex 1: ( 3, -1)
    // Vertex 2: (-1,  3)
    texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoord * 2.0 - 1.0, 0.0, 1.0);
    
    // Flip Y coordinate for Vulkan's coordinate system
    texCoord.y = 1.0 - texCoord.y;
}