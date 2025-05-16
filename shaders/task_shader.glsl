#version 450
#extension GL_EXT_mesh_shader : require

layout(local_size_x = 1) in;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Declare an empty payload structure
struct MeshTaskPayload {
    // Empty for this simple example
    float dummy; // Add a dummy value to avoid empty struct issues
};

// Then declare it as shared
taskPayloadSharedEXT MeshTaskPayload payload;

void main() {
    // Set dummy value (even though we don't need it)
    payload.dummy = 1.0;

    // Emit a single mesh shader workgroup for our cube
    EmitMeshTasksEXT(1, 1, 1);
}