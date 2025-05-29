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

// Push constants removed - using UBO directly for better performance

// Chunk data for GPU culling and processing
struct ChunkDrawData {
    vec3 minBounds;
    float padding1;
    vec3 maxBounds;
    float padding2;
    uint firstFaceIndex;
    uint faceCount;
    uint padding3[2];
};

// Storage buffer for chunk data
layout(binding = 3, std430) restrict readonly buffer ChunkDataBuffer {
    ChunkDrawData chunks[];
} chunkDataBuffer;

// Payload to pass chunk info to mesh shader
struct MeshTaskPayload {
    uint chunkIndex;
    uint firstFaceIndex;
    uint faceCount;
};

// Then declare it as shared
taskPayloadSharedEXT MeshTaskPayload payload;

void main() {
    // For indirect drawing with multiple draws, we need a different approach
    // Since each draw launches 1 task workgroup, we can use a counter or push constant
    // For now, let's use a simpler approach - single draw with multiple workgroups
    uint chunkIndex = gl_WorkGroupID.x;
    
    // For now, we'll assume chunk data buffer has been properly sized
    // In production, you'd pass the chunk count via UBO or push constant
    
    // Get chunk data
    ChunkDrawData chunk = chunkDataBuffer.chunks[chunkIndex];
    
    // TODO: Add frustum culling here
    // For now, just process all chunks
    
    // Pass chunk info to mesh shader
    payload.chunkIndex = chunkIndex;
    payload.firstFaceIndex = chunk.firstFaceIndex;
    payload.faceCount = chunk.faceCount;
    
    // Calculate how many mesh workgroups we need for this chunk's faces
    uint facesPerWorkgroup = 32;
    uint numWorkgroups = (chunk.faceCount + facesPerWorkgroup - 1) / facesPerWorkgroup;
    
    // Emit mesh tasks for this chunk
    EmitMeshTasksEXT(numWorkgroups, 1, 1);
}