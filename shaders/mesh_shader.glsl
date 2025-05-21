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
    vec3(1.0, 0.0, 0.0), // red - top
    vec3(0.0, 1.0, 0.0), // green - bottom
    vec3(0.0, 0.0, 1.0), // blue - front
    vec3(1.0, 1.0, 0.0), // yellow - back
    vec3(1.0, 0.0, 1.0), // magenta - left
    vec3(0.0, 1.0, 1.0)  // cyan - right
};

// Face instance data for each of the 6 cube faces
struct FaceInstance {
    vec3 position;
    vec4 rotation; // quaternion
};

// Fixed face definitions for a perfect unit cube from (0,0,0) to (1,1,1)
const FaceInstance faceInstances[6] = {
    // Top face (Y+)
    FaceInstance(
        vec3(0.0, 1.0, 0.0),  // position at top
        vec4(0.7071, 0.0, 0.0, 0.7071)  // 90° around X
    ),
    
    // Bottom face (Y-)
    FaceInstance(
        vec3(0.0, 0.0, 1.0),  // position at bottom
        vec4(-0.7071, 0.0, 0.0, 0.7071)  // -90° around X
    ),
    
    // Front face (Z+)
    FaceInstance(
        vec3(0.0, 0.0, 0.0),  // position at front
        vec4(0.0, 0.0, 0.0, 1.0)  // no rotation
    ),
    
    // Back face (Z-)
    FaceInstance(
        vec3(0.0, 0.0, 1.0),  // position at back
        vec4(0.0, 1.0, 0.0, 0.0)  // 180° rotation
    ),
    
    // Left face (X-)
    FaceInstance(
        vec3(0.0, 0.0, 0.0),  // position at left
        vec4(0.0, 0.7071, 0.0, 0.7071)  // 90° around Y
    ),
    
    // Right face (X+)
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

// Reconstruct the model matrix including rotation animation
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
    cameraPos.y = distance * cos(pitch);
    cameraPos.z = distance * sin(pitch) * sin(yaw);

    // Create view matrix (look at origin)
    vec3 target = vec3(0.5, 0.5, 0.5);  // Center of the cube
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
    }
    
    // Output triangles for this face
    // Triangle 1: 0-1-2
    gl_PrimitiveTriangleIndicesEXT[baseTriangleIndex] = 
        uvec3(baseVertexIndex, baseVertexIndex + 1, baseVertexIndex + 2);
    
    // Triangle 2: 0-2-3
    gl_PrimitiveTriangleIndicesEXT[baseTriangleIndex + 1] = 
        uvec3(baseVertexIndex, baseVertexIndex + 2, baseVertexIndex + 3);
}