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
    vec4 cameraPos;       // Camera position for LOD
    vec4 frustumPlanes[6]; // Frustum planes for culling
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

// Test if an AABB is inside the frustum
bool isAABBInFrustum(vec3 minBounds, vec3 maxBounds) {
    // Test against all 6 frustum planes
    for (int i = 0; i < 6; i++) {
        vec4 plane = ubo.frustumPlanes[i];
        
        // Find the vertex most positive along the plane normal
        vec3 p;
        p.x = (plane.x > 0.0) ? maxBounds.x : minBounds.x;
        p.y = (plane.y > 0.0) ? maxBounds.y : minBounds.y;
        p.z = (plane.z > 0.0) ? maxBounds.z : minBounds.z;
        
        // If this vertex is outside the plane, the whole AABB is outside
        if (dot(vec4(p, 1.0), plane) < 0.0) {
            return false;
        }
    }
    return true;
}

void main() {
    // For indirect drawing with multiple draws, we need a different approach
    // Since each draw launches 1 task workgroup, we can use a counter or push constant
    // For now, let's use a simpler approach - single draw with multiple workgroups
    uint chunkIndex = gl_WorkGroupID.x;
    
    // For now, we'll assume chunk data buffer has been properly sized
    // In production, you'd pass the chunk count via UBO or push constant
    
    // Get chunk data
    ChunkDrawData chunk = chunkDataBuffer.chunks[chunkIndex];
    
    // Perform frustum culling
    if (!isAABBInFrustum(chunk.minBounds, chunk.maxBounds)) {
        // Chunk is outside frustum - don't emit any mesh workgroups
        return;
    }
    
    // Pass chunk info to mesh shader
    payload.chunkIndex = chunkIndex;
    payload.firstFaceIndex = chunk.firstFaceIndex;
    payload.faceCount = chunk.faceCount;
    
    // Calculate how many mesh workgroups we need for this chunk's faces
    uint facesPerWorkgroup = 32;
    uint numWorkgroups = (chunk.faceCount + facesPerWorkgroup - 1) / facesPerWorkgroup;
    
    // Emit mesh tasks for visible chunk
    EmitMeshTasksEXT(numWorkgroups, 1, 1);
}