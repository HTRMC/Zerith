#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragColor;

// Base cube vertices for a 1x1x1 cube
vec3 basePositions[36] = vec3[](
    // Front face
    vec3(-0.5, -0.5,  0.5),
    vec3(-0.5,  0.5,  0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3(-0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3( 0.5, -0.5,  0.5),
    // Back face
    vec3(-0.5, -0.5, -0.5),
    vec3( 0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5),
    vec3(-0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5),
    vec3(-0.5,  0.5, -0.5),
    // Top face
    vec3(-0.5,  0.5, -0.5),
    vec3( 0.5,  0.5, -0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3(-0.5,  0.5, -0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3(-0.5,  0.5,  0.5),
    // Bottom face
    vec3(-0.5, -0.5, -0.5),
    vec3(-0.5, -0.5,  0.5),
    vec3( 0.5, -0.5,  0.5),
    vec3(-0.5, -0.5, -0.5),
    vec3( 0.5, -0.5,  0.5),
    vec3( 0.5, -0.5, -0.5),
    // Right face
    vec3( 0.5, -0.5, -0.5),
    vec3( 0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3( 0.5, -0.5, -0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3( 0.5,  0.5, -0.5),
    // Left face
    vec3(-0.5, -0.5, -0.5),
    vec3(-0.5,  0.5, -0.5),
    vec3(-0.5,  0.5,  0.5),
    vec3(-0.5, -0.5, -0.5),
    vec3(-0.5,  0.5,  0.5),
    vec3(-0.5, -0.5,  0.5)
);

// Colors for each face
vec3 colors[6] = vec3[](
    vec3(1.0, 0.0, 0.0), // Front - Red
    vec3(0.0, 1.0, 0.0), // Back - Green
    vec3(0.0, 0.0, 1.0), // Top - Blue
    vec3(1.0, 1.0, 0.0), // Bottom - Yellow
    vec3(1.0, 0.0, 1.0), // Right - Magenta
    vec3(0.0, 1.0, 1.0)  // Left - Cyan
);

void main() {
    // Calculate which block this vertex belongs to
    int verticesPerCube = 36;
    int cubeIndex = gl_VertexIndex / verticesPerCube;
    int vertexInCube = gl_VertexIndex % verticesPerCube;

    // Calculate x, y, z coordinates of the block in the 16x16x16 grid
    int x = cubeIndex % 16;
    int y = (cubeIndex / 16) % 16;
    int z = cubeIndex / (16 * 16);

    // Get the base position for this vertex
    vec3 basePos = basePositions[vertexInCube];

    // Offset the position based on the block's position in the grid
    vec3 offsetPos = basePos + vec3(x, y, z);

    // Calculate final position
    gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(offsetPos, 1.0);

    // Set color based on which face this vertex belongs to
    fragColor = colors[vertexInCube / 6];
}