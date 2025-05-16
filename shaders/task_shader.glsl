#version 450
#extension GL_EXT_mesh_shader : require

layout(local_size_x = 1) in;

// Updated uniform buffer with packed data
layout(binding = 0) uniform CompressedUBO {
    float time;           // Time for animation
    uvec2 packedCamera;   // packed camera position and orientation
    uvec2 packedProj;     // packed projection parameters
} ubo;

// Function to convert a quaternion to a 4x4 rotation matrix
mat4 quatToMat4(vec4 q) {
    float qxx = q.x * q.x;
    float qyy = q.y * q.y;
    float qzz = q.z * q.z;
    float qxz = q.x * q.z;
    float qxy = q.x * q.y;
    float qyz = q.y * q.z;
    float qwx = q.w * q.x;
    float qwy = q.w * q.y;
    float qwz = q.w * q.z;

    mat4 result;
    result[0][0] = 1.0 - 2.0 * (qyy + qzz);
    result[0][1] = 2.0 * (qxy + qwz);
    result[0][2] = 2.0 * (qxz - qwy);
    result[0][3] = 0.0;

    result[1][0] = 2.0 * (qxy - qwz);
    result[1][1] = 1.0 - 2.0 * (qxx + qzz);
    result[1][2] = 2.0 * (qyz + qwx);
    result[1][3] = 0.0;

    result[2][0] = 2.0 * (qxz + qwy);
    result[2][1] = 2.0 * (qyz - qwx);
    result[2][2] = 1.0 - 2.0 * (qxx + qyy);
    result[2][3] = 0.0;

    result[3] = vec4(0.0, 0.0, 0.0, 1.0);
    return result;
}

// Function to unpack 2 half-float values from a uint
vec2 unpackHalf2(uint packedValue) {
    return unpackHalf2x16(packedValue);
}

// Declare an empty payload structure
struct MeshTaskPayload {
    float dummy;
};

// Then declare it as shared
taskPayloadSharedEXT MeshTaskPayload payload;

void main() {
    // Set dummy value
    payload.dummy = 1.0;

    // Emit a single mesh shader workgroup for our cube
    EmitMeshTasksEXT(1, 1, 1);
}