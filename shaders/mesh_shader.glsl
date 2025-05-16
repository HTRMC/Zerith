#version 450
#extension GL_EXT_mesh_shader : require

// Define cube vertices (unchanged)
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

// Define cube colors (unchanged)
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

// Define cube indices (unchanged)
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

// Mesh shader configuration (unchanged)
layout(local_size_x = 1) in;
layout(triangles, max_vertices = 8, max_primitives = 12) out;

// Must match task shader's structure (unchanged)
struct MeshTaskPayload {
    float dummy;
};

// Then declare it as shared (unchanged)
taskPayloadSharedEXT MeshTaskPayload payload;

// Output to fragment shader (unchanged)
layout(location = 0) out PerVertexData {
    vec3 color;
} v_out[];

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

// Generate rotation quaternion from time
vec4 generateRotationQuat() {
    // Rotation speed: 45 degrees per second
    float angle = ubo.time * radians(45.0);

    // Rotation around Y axis (0,1,0)
    // Quaternion formula: cos(angle/2) + sin(angle/2) * axis
    float halfAngle = angle * 0.5;
    return vec4(0.0, sin(halfAngle), 0.0, cos(halfAngle));
}

// Reconstruct the model matrix from time-based quaternion
mat4 reconstructModelMatrix() {
    vec4 rotQuat = generateRotationQuat();
    return quatToMat4(rotQuat);
}

// Function to unpack 2 half-float values from a uint
vec2 unpackHalf2(uint packedValue) {
    return unpackHalf2x16(packedValue);
}

// Reconstruct the view matrix from packed camera data
mat4 reconstructViewMatrix() {
    // Unpack camera parameters
    vec2 distPitch = unpackHalf2(ubo.packedCamera[0]);
    vec2 yawUnused = unpackHalf2(ubo.packedCamera[1]);

    float distance = distPitch.x;
    float pitch = distPitch.y;
    float yaw = yawUnused.x;

    // Calculate camera position from spherical coordinates
    vec3 cameraPos;
    cameraPos.x = distance * sin(pitch) * cos(yaw);
    cameraPos.y = distance * sin(pitch) * sin(yaw);
    cameraPos.z = distance * cos(pitch);

    // Create view matrix (look at origin)
    vec3 target = vec3(0.0, 0.0, 0.0);
    vec3 up = vec3(0.0, 1.0, 0.0);

    vec3 f = normalize(target - cameraPos);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    mat4 view = mat4(
        vec4(s, 0.0),
        vec4(u, 0.0),
        vec4(-f, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    view[3][0] = -dot(s, cameraPos);
    view[3][1] = -dot(u, cameraPos);
    view[3][2] = dot(f, cameraPos);

    return view;
}

// Reconstruct the projection matrix from packed projection data
mat4 reconstructProjMatrix() {
    // Unpack projection parameters
    vec2 fovAspect = unpackHalf2(ubo.packedProj[0]);
    vec2 nearFar = unpackHalf2(ubo.packedProj[1]);

    float fov = fovAspect.x;
    float aspect = fovAspect.y;
    float near = nearFar.x;
    float far = nearFar.y;

    // Create projection matrix
    float tanHalfFov = tan(fov / 2.0);
    float f = 1.0 / tanHalfFov;
    float nf = 1.0 / (near - far);

    mat4 proj = mat4(
        vec4(f / aspect, 0.0, 0.0, 0.0),
        vec4(0.0, -f, 0.0, 0.0),  // Flip Y for Vulkan
        vec4(0.0, 0.0, far * nf, -1.0),
        vec4(0.0, 0.0, far * near * nf, 0.0)
    );

    return proj;
}

void main() {
    // Set the number of vertices and primitives to output
    SetMeshOutputsEXT(8, 12);

    // Reconstruct matrices from compressed data
    mat4 model = reconstructModelMatrix();
    mat4 view = reconstructViewMatrix();
    mat4 proj = reconstructProjMatrix();
    mat4 mvp = proj * view * model;

    // Output all vertices
    for (int i = 0; i < 8; i++) {
        // Transform vertex position with the reconstructed MVP matrix
        gl_MeshVerticesEXT[i].gl_Position = mvp * vec4(positions[i], 1.0);

        // Set vertex color
        v_out[i].color = colors[i];
    }

    // Output all primitives (triangles) - unchanged
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