#version 450
#extension GL_EXT_mesh_shader : require

// Configure task shader to dispatch a single mesh shader workgroup
layout(local_size_x = 1) in;

// Uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    float time;           // Time for animation
    mat4 view;            // View matrix
    mat4 proj;            // Projection matrix
    uint faceCount;       // Number of face instances to render
} ubo;

// Push constant for indirect drawing
layout(push_constant) uniform PushConstants {
    uint firstFaceIndex;  // Starting face index for this draw
    uint faceCount;       // Number of faces in this chunk
} pc;

// Declare an empty payload structure
struct MeshTaskPayload {
    float dummy;
};

// Then declare it as shared
taskPayloadSharedEXT MeshTaskPayload payload;

void main() {
    // Set dummy value
    payload.dummy = 1.0;

    // Calculate how many mesh workgroups we need for this chunk
    // Each mesh workgroup can handle 32 faces
    uint facesPerWorkgroup = 32;
    uint numWorkgroups = (pc.faceCount + facesPerWorkgroup - 1) / facesPerWorkgroup;
    
    // Emit mesh tasks for this chunk's workgroups
    EmitMeshTasksEXT(numWorkgroups, 1, 1);
}