#version 450
#extension GL_EXT_mesh_shader : require

// Configure task shader to dispatch a single mesh shader workgroup
layout(local_size_x = 1) in;

// Updated uniform buffer with packed data
layout(binding = 0) uniform CompressedUBO {
    float time;           // Time for animation
    uvec2 packedCamera;   // packed camera position and orientation
    uvec2 packedProj;     // packed projection parameters
    uint faceCount;       // Number of face instances to render
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

    // Calculate how many mesh workgroups we need
    // Each mesh workgroup can handle 32 faces
    uint facesPerWorkgroup = 32;
    uint numWorkgroups = (ubo.faceCount + facesPerWorkgroup - 1) / facesPerWorkgroup;
    
    // Emit mesh tasks for all workgroups needed
    EmitMeshTasksEXT(numWorkgroups, 1, 1);
}