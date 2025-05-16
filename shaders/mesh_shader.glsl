#version 450
#extension GL_EXT_mesh_shader : require

// Define cube vertices
const vec3 positions[8] = {
    // Front face (z = 0.5)
    vec3(-0.5, -0.5,  0.5),
    vec3( 0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3(-0.5,  0.5,  0.5),
    // Back face (z = -0.5)
    vec3(-0.5, -0.5, -0.5),
    vec3( 0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5),
    vec3(-0.5,  0.5, -0.5)
};

// Define cube colors (one per vertex for nice interpolation)
const vec3 colors[8] = {
    vec3(1.0, 0.0, 0.0), // red
    vec3(0.0, 1.0, 0.0), // green
    vec3(0.0, 0.0, 1.0), // blue
    vec3(1.0, 1.0, 0.0), // yellow
    vec3(1.0, 0.0, 1.0), // magenta
    vec3(0.0, 1.0, 1.0), // cyan
    vec3(1.0, 0.5, 0.0), // orange
    vec3(0.5, 0.0, 1.0)  // purple
};

// Define cube indices (12 triangles = 36 vertices with duplicates)
const uint indices[36] = {
    // Front face
    0, 1, 2, 2, 3, 0,
    // Right face
    1, 5, 6, 6, 2, 1,
    // Back face
    5, 4, 7, 7, 6, 5,
    // Left face
    4, 0, 3, 3, 7, 4,
    // Top face
    3, 2, 6, 6, 7, 3,
    // Bottom face
    4, 5, 1, 1, 0, 4
};

// Mesh shader configuration
layout(local_size_x = 1) in;
layout(triangles, max_vertices = 8, max_primitives = 12) out;

// Must exactly match the task shader's structure
struct MeshTaskPayload {
    // Empty for this simple example
    float dummy; // Add a dummy value to avoid empty struct issues
};

// Then declare it as shared
taskPayloadSharedEXT MeshTaskPayload payload;

// Output to fragment shader
layout(location = 0) out PerVertexData {
    vec3 color;
} v_out[];

// Uniform buffer for transformations
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    // We don't actually use the payload in this simple example
    // float dummy = payload.dummy;

    // Set the number of vertices and primitives to output
    SetMeshOutputsEXT(8, 12);

    // Output all vertices
    for (int i = 0; i < 8; i++) {
        // Transform vertex position
        gl_MeshVerticesEXT[i].gl_Position = ubo.proj * ubo.view * ubo.model * vec4(positions[i], 1.0);
        // Set vertex color
        v_out[i].color = colors[i];
    }

    // Output all primitives (triangles)
    for (int i = 0; i < 12; i++) {
        // Each triangle uses 3 indices
        uint baseIndex = i * 3;
        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(
            indices[baseIndex],
            indices[baseIndex + 1],
            indices[baseIndex + 2]
        );
    }
}