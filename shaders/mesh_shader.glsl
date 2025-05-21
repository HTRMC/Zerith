#version 450
#extension GL_EXT_mesh_shader : require

// Define a single quad (unit square in XY plane at Z=0)
// This will be instanced for each face
const vec3 quadVertices[4] = {
    vec3(0.0, 0.0, 0.0),  // Bottom-left
    vec3(1.0, 0.0, 0.0),  // Bottom-right
    vec3(1.0, 1.0, 0.0),  // Top-right
    vec3(0.0, 1.0, 0.0)   // Top-left
};

// Define cube face colors
const vec3 faceColors[6] = {
    vec3(1.0, 0.0, 0.0), // RED - TOP FACE (Y+)
    vec3(0.0, 1.0, 0.0), // GREEN - BOTTOM FACE (Y-)
    vec3(0.0, 0.0, 1.0), // BLUE - FRONT FACE (Z+)
    vec3(1.0, 1.0, 0.0), // YELLOW - BACK FACE (Z-)
    vec3(1.0, 0.0, 1.0), // MAGENTA - LEFT FACE (X-)
    vec3(0.0, 1.0, 1.0)  // CYAN - RIGHT FACE (X+)
};

// Face instance data for each of the 6 cube faces
struct FaceInstance {
    vec3 position;
    vec4 rotation; // quaternion
};

// Fixed face definitions for a perfect unit cube from (0,0,0) to (1,1,1)
const FaceInstance faceInstances[6] = {
    // Top face (Y+) - RED
    FaceInstance(
        vec3(0.0, 1.0, 0.0),  // position at top
        vec4(0.7071, 0.0, 0.0, 0.7071)  // 90° around X
    ),
    
    // Bottom face (Y-) - GREEN
    FaceInstance(
        vec3(0.0, 0.0, 1.0),  // position at bottom
        vec4(-0.7071, 0.0, 0.0, 0.7071)  // -90° around X
    ),
    
    // Front face (Z+) - BLUE
    FaceInstance(
        vec3(0.0, 0.0, 0.0),  // position at front
        vec4(0.0, 0.0, 0.0, 1.0)  // no rotation
    ),
    
    // Back face (Z-) - YELLOW
    FaceInstance(
        vec3(1.0, 0.0, 1.0),  // position at back
        vec4(0.0, 1.0, 0.0, 0.0)  // 180° rotation
    ),
    
    // Left face (X-) - MAGENTA
    FaceInstance(
        vec3(0.0, 0.0, 1.0),  // position at left
        vec4(0.0, 0.7071, 0.0, 0.7071)  // 90° around Y
    ),
    
    // Right face (X+) - CYAN
    FaceInstance(
        vec3(1.0, 0.0, 0.0),  // position at right
        vec4(0.0, -0.7071, 0.0, 0.7071)  // -90° around Y
    )
};

// Mesh shader configuration - one workgroup will handle all six faces
layout(local_size_x = 6) in;
layout(triangles, max_vertices = 24, max_primitives = 12) out;

// Must match task shader's structure
struct MeshTaskPayload {
    float dummy;
};

// Then declare it as shared
taskPayloadSharedEXT MeshTaskPayload payload;

// Output to fragment shader
layout(location = 0) out PerVertexData {
    vec3 color;
    vec2 texCoord;
    flat uint faceIndex;
} v_out[];

// Updated uniform buffer with packed data
layout(binding = 0) uniform CompressedUBO {
    float time;           // Time for animation
    uvec2 packedCamera;   // packed camera position and orientation
    uvec2 packedProj;     // packed projection parameters
} ubo;

// Function to convert a quaternion to a 4x4 rotation matrix
mat4 quatToMat4(vec4 q) {
    // Normalize the quaternion
    q = normalize(q);
    
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

// Generate rotation quaternion from time for overall cube animation
vec4 generateRotationQuat() {
    // Rotation speed: 45 degrees per second
    float angle = ubo.time * radians(45.0);

    // Rotation around Y axis (0,1,0)
    // Quaternion formula: cos(angle/2) + sin(angle/2) * axis
    float halfAngle = angle * 0.5;
    return vec4(0.0, sin(halfAngle), 0.0, cos(halfAngle));
}

// Model matrix with no rotation (identity matrix)
mat4 reconstructModelMatrix() {
    // Return identity matrix (no rotation)
    return mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

// Function to unpack 2 half-float values from a uint
vec2 unpackHalf2(uint packedValue) {
    return unpackHalf2x16(packedValue);
}

// Reconstruct the view matrix from packed camera data
mat4 reconstructViewMatrix() {
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

// Reconstruct the projection matrix from packed projection data
mat4 reconstructProjMatrix() {
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
        vec4(0.0, f, 0.0, 0.0),
        vec4(0.0, 0.0, far * nf, -1.0),
        vec4(0.0, 0.0, far * near * nf, 0.0)
    );

    return proj;
}

// Helper to transform a quad vertex based on face transformation
vec3 transformQuadVertex(vec3 vertex, mat4 faceModel) {
    // Apply face transformation to get the world position
    vec4 worldPos = faceModel * vec4(vertex, 1.0);
    return worldPos.xyz;
}

void main() {
    // Calculate the face index from the local invocation ID
    uint faceIndex = gl_LocalInvocationID.x;
    
    // Each face has 4 vertices and 2 triangles
    uint verticesPerFace = 4;
    uint trianglesPerFace = 2;
    
    // Set the number of vertices and primitives to output
    SetMeshOutputsEXT(24, 12); // (6 faces * 4 vertices, 6 faces * 2 triangles)
    
    // Reconstruct matrices from compressed data
    mat4 cubeModel = reconstructModelMatrix();  // Overall cube rotation
    mat4 view = reconstructViewMatrix();
    mat4 proj = reconstructProjMatrix();
    mat4 vp = proj * view;
    
    // Base index for this face's vertices in the output arrays
    uint baseVertexIndex = faceIndex * verticesPerFace;
    uint baseTriangleIndex = faceIndex * trianglesPerFace;
    
    // Get face instance data
    FaceInstance face = faceInstances[faceIndex];
    
    // Create model matrix for this face
    mat4 faceRotation = quatToMat4(face.rotation);
    mat4 faceTranslation = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(face.position, 1.0)
    );
    mat4 faceModel = faceTranslation * faceRotation;
    
    // Combine with overall cube animation
    mat4 mvp = vp * cubeModel;
    
    // Set texture coordinates for the quad
    // Note: In texture coordinates, (0,0) is top-left in Vulkan
    vec2 texCoords[4] = {
        vec2(0.0, 0.0),  // Bottom-left
        vec2(1.0, 0.0),  // Bottom-right
        vec2(1.0, 1.0),  // Top-right
        vec2(0.0, 1.0)   // Top-left
    };
    
    // Output vertices for this face
    for (int i = 0; i < 4; i++) {
        uint vertexIndex = baseVertexIndex + i;
        
        // Transform quad vertex to face position and orientation
        vec3 worldPos = transformQuadVertex(quadVertices[i], faceModel);
        
        // Apply overall cube animation and projection
        gl_MeshVerticesEXT[vertexIndex].gl_Position = mvp * vec4(worldPos, 1.0);
        
        // Set per-vertex data
        v_out[vertexIndex].color = faceColors[faceIndex];
        v_out[vertexIndex].texCoord = texCoords[i];
        v_out[vertexIndex].faceIndex = faceIndex;
    }
    
    // Output triangles for this face
    // Triangle 1: 0-1-2
    gl_PrimitiveTriangleIndicesEXT[baseTriangleIndex] = 
        uvec3(baseVertexIndex, baseVertexIndex + 1, baseVertexIndex + 2);
    
    // Triangle 2: 0-2-3
    gl_PrimitiveTriangleIndicesEXT[baseTriangleIndex + 1] = 
        uvec3(baseVertexIndex, baseVertexIndex + 2, baseVertexIndex + 3);
}