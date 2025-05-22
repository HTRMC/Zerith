#version 450
#extension GL_EXT_mesh_shader : require

// Configure task shader to dispatch a single mesh shader workgroup
layout(local_size_x = 1) in;

// Updated uniform buffer with packed data
layout(binding = 0) uniform CompressedUBO {
    float time;           // Time for animation
    uvec2 packedCamera;   // packed camera position and orientation
    uvec2 packedProj;     // packed projection parameters
    vec3 elementFrom;     // Model element from coordinates (Vulkan space)
    vec3 elementTo;       // Model element to coordinates (Vulkan space)
} ubo;

// Declare an empty payload structure
struct MeshTaskPayload {
    float dummy;
};

// Then declare it as shared
taskPayloadSharedEXT MeshTaskPayload payload;

void main() {
    // Set dummy value
    payload.dummy = 1.0;

    // Emit a single mesh shader workgroup
    // The mesh shader will handle all 6 faces with 6 local invocations
    EmitMeshTasksEXT(1, 1, 1);
}