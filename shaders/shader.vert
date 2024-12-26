// shader.vert
 #version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragColor;

// Cube vertices
vec3 positions[36] = vec3[](
    // Front face (clockwise)
    vec3(-0.5, -0.5,  0.5),
    vec3(-0.5,  0.5,  0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3(-0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3( 0.5, -0.5,  0.5),

    // Back face (clockwise when looking from back)
    vec3(-0.5, -0.5, -0.5),
    vec3( 0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5),
    vec3(-0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5),
    vec3(-0.5,  0.5, -0.5),

    // Top face (clockwise from top)
    vec3(-0.5,  0.5, -0.5),
    vec3( 0.5,  0.5, -0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3(-0.5,  0.5, -0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3(-0.5,  0.5,  0.5),

    // Bottom face (clockwise from bottom)
    vec3(-0.5, -0.5, -0.5),
    vec3(-0.5, -0.5,  0.5),
    vec3( 0.5, -0.5,  0.5),
    vec3(-0.5, -0.5, -0.5),
    vec3( 0.5, -0.5,  0.5),
    vec3( 0.5, -0.5, -0.5),

    // Right face (clockwise)
    vec3( 0.5, -0.5, -0.5),
    vec3( 0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3( 0.5, -0.5, -0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3( 0.5,  0.5, -0.5),

    // Left face (clockwise)
    vec3(-0.5, -0.5, -0.5),
    vec3(-0.5,  0.5, -0.5),
    vec3(-0.5,  0.5,  0.5),
    vec3(-0.5, -0.5, -0.5),
    vec3(-0.5,  0.5,  0.5),
    vec3(-0.5, -0.5,  0.5)
);

// Colors for each vertex
vec3 colors[36] = vec3[](
    // Front face (red)
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    // Back face (green)
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    // Top face (blue)
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
    // Bottom face (yellow)
    vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
    vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
    // Right face (magenta)
    vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0),
    vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0),
    // Left face (cyan)
    vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0),
    vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 1.0)
);

 void main() {
     gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(positions[gl_VertexIndex], 1.0);
     fragColor = colors[gl_VertexIndex];
 }