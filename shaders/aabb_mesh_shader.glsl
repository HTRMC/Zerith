#version 450
#extension GL_EXT_mesh_shader : require

// AABB wireframe mesh shader
// Generates line primitives for AABB edges

layout(local_size_x = 1) in;
layout(lines, max_vertices = 8, max_primitives = 12) out;

// Output to fragment shader
layout(location = 0) out PerVertexData {
    vec3 color;
} v_out[];

// Uniform buffer with camera data
layout(binding = 0) uniform UniformBufferObject {
    float time;
    mat4 view;
    mat4 proj;
    uint faceCount; // Reused as aabbCount when in debug mode
} ubo;

// AABB instance data
struct AABBData {
    vec4 min; // xyz + padding
    vec4 max; // xyz + padding
    vec4 color; // rgba
};

// Storage buffer for AABB instances
layout(binding = 1, std430) restrict readonly buffer AABBBuffer {
    AABBData aabbs[];
} aabbBuffer;

void main() {
    uint aabbIndex = gl_GlobalInvocationID.x;
    
    if (aabbIndex >= ubo.faceCount) { // faceCount is reused as aabbCount in debug mode
        return;
    }
    
    // Get AABB data
    AABBData aabb = aabbBuffer.aabbs[aabbIndex];
    vec3 minPos = aabb.min.xyz;
    vec3 maxPos = aabb.max.xyz;
    vec3 color = aabb.color.rgb;
    
    // Generate 8 corner vertices
    vec3 corners[8] = {
        vec3(minPos.x, minPos.y, minPos.z),
        vec3(maxPos.x, minPos.y, minPos.z),
        vec3(maxPos.x, maxPos.y, minPos.z),
        vec3(minPos.x, maxPos.y, minPos.z),
        vec3(minPos.x, minPos.y, maxPos.z),
        vec3(maxPos.x, minPos.y, maxPos.z),
        vec3(maxPos.x, maxPos.y, maxPos.z),
        vec3(minPos.x, maxPos.y, maxPos.z)
    };
    
    // Get view and projection matrices from UBO
    mat4 view = ubo.view;
    mat4 proj = ubo.proj;
    mat4 mvp = proj * view;
    
    // Set mesh output counts - 8 vertices, 12 line primitives
    SetMeshOutputsEXT(8, 12);
    
    // Output the 8 corner vertices
    for (uint i = 0; i < 8; i++) {
        gl_MeshVerticesEXT[i].gl_Position = mvp * vec4(corners[i], 1.0);
        v_out[i].color = color;
    }
    
    // Define the 12 edges using vertex indices
    gl_PrimitiveLineIndicesEXT[0] = uvec2(0, 1);  // Bottom face
    gl_PrimitiveLineIndicesEXT[1] = uvec2(1, 2);
    gl_PrimitiveLineIndicesEXT[2] = uvec2(2, 3);
    gl_PrimitiveLineIndicesEXT[3] = uvec2(3, 0);
    
    gl_PrimitiveLineIndicesEXT[4] = uvec2(4, 5);  // Top face
    gl_PrimitiveLineIndicesEXT[5] = uvec2(5, 6);
    gl_PrimitiveLineIndicesEXT[6] = uvec2(6, 7);
    gl_PrimitiveLineIndicesEXT[7] = uvec2(7, 4);
    
    gl_PrimitiveLineIndicesEXT[8] = uvec2(0, 4);  // Vertical edges
    gl_PrimitiveLineIndicesEXT[9] = uvec2(1, 5);
    gl_PrimitiveLineIndicesEXT[10] = uvec2(2, 6);
    gl_PrimitiveLineIndicesEXT[11] = uvec2(3, 7);
}