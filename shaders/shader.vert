#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragColor;

// Base cube vertices for a 1x1x1 cube, with face indices
vec3 basePositions[36] = vec3[](
// Front face (0-5)
vec3(-0.5, -0.5,  0.5),
vec3(-0.5,  0.5,  0.5),
vec3( 0.5,  0.5,  0.5),
vec3(-0.5, -0.5,  0.5),
vec3( 0.5,  0.5,  0.5),
vec3( 0.5, -0.5,  0.5),
// Back face (6-11)
vec3(-0.5, -0.5, -0.5),
vec3( 0.5, -0.5, -0.5),
vec3( 0.5,  0.5, -0.5),
vec3(-0.5, -0.5, -0.5),
vec3( 0.5,  0.5, -0.5),
vec3(-0.5,  0.5, -0.5),
// Top face (12-17)
vec3(-0.5,  0.5, -0.5),
vec3( 0.5,  0.5, -0.5),
vec3( 0.5,  0.5,  0.5),
vec3(-0.5,  0.5, -0.5),
vec3( 0.5,  0.5,  0.5),
vec3(-0.5,  0.5,  0.5),
// Bottom face (18-23)
vec3(-0.5, -0.5, -0.5),
vec3(-0.5, -0.5,  0.5),
vec3( 0.5, -0.5,  0.5),
vec3(-0.5, -0.5, -0.5),
vec3( 0.5, -0.5,  0.5),
vec3( 0.5, -0.5, -0.5),
// Right face (24-29)
vec3( 0.5, -0.5, -0.5),
vec3( 0.5, -0.5,  0.5),
vec3( 0.5,  0.5,  0.5),
vec3( 0.5, -0.5, -0.5),
vec3( 0.5,  0.5,  0.5),
vec3( 0.5,  0.5, -0.5),
// Left face (30-35)
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

// Function to check if a block exists at the given coordinates
bool blockExists(ivec3 pos) {
    // Check if the position is within the 16x16x16 grid
    if (pos.x < 0 || pos.x >= 16 ||
    pos.y < 0 || pos.y >= 16 ||
    pos.z < 0 || pos.z >= 16) {
        return false;
    }
    return true;
}

// Function to check if a face should be rendered
bool shouldRenderFace(int faceIndex, ivec3 blockPos) {
    ivec3 neighbor;

    // Check different neighbors based on face index
    if (faceIndex >= 0 && faceIndex <= 5) {           // Front face
                                                      neighbor = blockPos + ivec3(0, 0, 1);
    } else if (faceIndex >= 6 && faceIndex <= 11) {   // Back face
                                                      neighbor = blockPos + ivec3(0, 0, -1);
    } else if (faceIndex >= 12 && faceIndex <= 17) {  // Top face
                                                      neighbor = blockPos + ivec3(0, 1, 0);
    } else if (faceIndex >= 18 && faceIndex <= 23) {  // Bottom face
                                                      neighbor = blockPos + ivec3(0, -1, 0);
    } else if (faceIndex >= 24 && faceIndex <= 29) {  // Right face
                                                      neighbor = blockPos + ivec3(1, 0, 0);
    } else {                                          // Left face
                                                      neighbor = blockPos + ivec3(-1, 0, 0);
    }

    return !blockExists(neighbor);
}

void main() {
    // Calculate which block this vertex belongs to
    int verticesPerCube = 36;
    int cubeIndex = gl_VertexIndex / verticesPerCube;
    int vertexInCube = gl_VertexIndex % verticesPerCube;

    // Calculate block position in the 16x16x16 grid
    ivec3 blockPos = ivec3(
    cubeIndex % 16,                  // x
    (cubeIndex / 16) % 16,          // y
    cubeIndex / (16 * 16)           // z
    );

    // Determine which face this vertex belongs to (0-5)
    int faceIndex = vertexInCube / 6;

    // Check if this face should be rendered
    if (!shouldRenderFace(vertexInCube, blockPos)) {
        // If the face shouldn't be rendered, move the vertex behind the camera
        gl_Position = vec4(0, 0, 10, 1);
        fragColor = vec3(0);
        return;
    }

    // Get the base position for this vertex
    vec3 basePos = basePositions[vertexInCube];

    // Offset the position based on the block's position in the grid
    vec3 offsetPos = basePos + vec3(blockPos);

    // Calculate final position
    gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(offsetPos, 1.0);

    // Set color based on which face this vertex belongs to
    fragColor = colors[faceIndex];
}