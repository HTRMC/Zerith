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
layout(binding = 0) uniform CompressedUBO {
    float time;
    uvec2 packedCamera;
    uvec2 packedProj;
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

// Unpack half precision floats
vec2 unpackHalf2(uint packed) {
    return vec2(
        unpackHalf2x16(packed).x,
        unpackHalf2x16(packed).y
    );
}

// Generate view and projection matrices from packed data
mat4 generateViewMatrix() {
    // Unpack camera parameters
    vec2 posXPitch = unpackHalf2(ubo.packedCamera[0]);
    vec2 posYYaw = unpackHalf2(ubo.packedCamera[1]);
    vec2 posZFar = unpackHalf2(ubo.packedProj[1]);
    
    // Camera position in 3D space
    vec3 cameraPos = vec3(posXPitch.x, posYYaw.x, posZFar.x);
    
    // Camera orientation angles
    float pitch = posXPitch.y;
    float yaw = posYYaw.y;
    
    // Calculate the three camera basis vectors using the standard view space convention
    
    // Forward vector (camera looks down the negative z-axis in view space)
    vec3 forward;
    forward.x = sin(yaw) * cos(pitch);
    forward.y = sin(pitch);
    forward.z = -cos(yaw) * cos(pitch);
    forward = normalize(forward);
    
    // Right vector (positive x-axis in view space)
    // Use a fixed world up vector for stability
    vec3 worldUp = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(forward, worldUp));
    
    // Camera's up vector (positive y-axis in view space)
    vec3 up = normalize(cross(right, forward));
    
    // Build view matrix directly using GLM conventions
    // For a camera view matrix, we need the inverse of the camera transform
    // This is equivalent to:
    // 1. The rows of the rotation matrix are the camera's local basis vectors
    // 2. The translation is -position transformed by the rotation
    
    // First, create the rotation part (transpose of the camera's orientation)
    mat4 view;
    view[0][0] = right.x;   view[0][1] = up.x;   view[0][2] = -forward.x;   view[0][3] = 0.0;
    view[1][0] = right.y;   view[1][1] = up.y;   view[1][2] = -forward.y;   view[1][3] = 0.0;
    view[2][0] = right.z;   view[2][1] = up.z;   view[2][2] = -forward.z;   view[2][3] = 0.0;
    
    // Then add the translation part (negated and rotated camera position)
    view[3][0] = -dot(right, cameraPos);
    view[3][1] = -dot(up, cameraPos);
    view[3][2] = dot(forward, cameraPos);
    view[3][3] = 1.0;
    
    return view;
}

mat4 generateProjectionMatrix() {
    // Unpack projection parameters
    vec2 fovAspect = unpackHalf2(ubo.packedProj[0]);
    vec2 posZFar = unpackHalf2(ubo.packedProj[1]);

    float fov = fovAspect.x;
    float aspect = fovAspect.y;
    float near = 0.1;  // Use fixed near plane
    float far = posZFar.y;   // Far plane from the packed data

    // Create projection matrix
    float tanHalfFov = tan(fov / 2.0);
    float f = 1.0 / tanHalfFov;
    float nf = 1.0 / (near - far);

    mat4 proj = mat4(
        vec4(f / aspect, 0.0, 0.0, 0.0),
        vec4(0.0, -f, 0.0, 0.0),
        vec4(0.0, 0.0, far * nf, -1.0),
        vec4(0.0, 0.0, far * near * nf, 0.0)
    );

    return proj;
}

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
    
    // Get view and projection matrices
    mat4 view = generateViewMatrix();
    mat4 proj = generateProjectionMatrix();
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