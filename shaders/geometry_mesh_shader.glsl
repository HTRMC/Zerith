#version 450
#extension GL_EXT_mesh_shader : require
#extension GL_EXT_nonuniform_qualifier : require

// Define a single quad (unit square in XY plane at Z=0)
const vec3 quadVertices[4] = {
    vec3(0.0, 1.0, 0.0),  // 0: Top-left
    vec3(0.0, 0.0, 0.0),  // 1: Bottom-left  
    vec3(1.0, 1.0, 0.0),  // 2: Top-right
    vec3(1.0, 0.0, 0.0)   // 3: Bottom-right
};

// Define cube face normals
const vec3 faceNormals[6] = {
    vec3( 0.0,  1.0,  0.0), // TOP FACE (Y+)
    vec3( 0.0, -1.0,  0.0), // BOTTOM FACE (Y-)
    vec3( 0.0,  0.0,  1.0), // FRONT FACE (Z+)
    vec3( 0.0,  0.0, -1.0), // BACK FACE (Z-)
    vec3(-1.0,  0.0,  0.0), // LEFT FACE (X-)
    vec3( 1.0,  0.0,  0.0)  // RIGHT FACE (X+)
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

// Constants for geometry
const uint VERTICES_PER_FACE = 4;
const uint TRIANGLES_PER_FACE = 2;
const uint FACES_PER_WORKGROUP = 32;
const uint MAX_VERTICES = FACES_PER_WORKGROUP * VERTICES_PER_FACE;
const uint MAX_TRIANGLES = FACES_PER_WORKGROUP * TRIANGLES_PER_FACE;

// Mesh shader configuration
layout(local_size_x = 32) in;
layout(triangles, max_vertices = MAX_VERTICES, max_primitives = MAX_TRIANGLES) out;

// Task payload
struct MeshTaskPayload {
    uint chunkIndex;
    uint firstFaceIndex;
    uint faceCount;
};
taskPayloadSharedEXT MeshTaskPayload payload;

// Output to fragment shader - extended for G-buffer
layout(location = 0) out PerVertexData {
    vec3 color;
    vec2 texCoord;
    flat uint faceIndex;
    flat uint textureLayer;
    vec3 worldPos;
    vec3 normal;
} v_out[];

// Uniform buffer
layout(binding = 0) uniform UniformBufferObject {
    float time;
    mat4 view;
    mat4 proj;
    uint faceCount;
    vec4 cameraPos;
    vec4 frustumPlanes[6];
} ubo;

// Face instance structure
struct FaceInstanceData {
    vec4 position;  // vec3 + padding
    vec4 rotation;  // quaternion
    vec4 scale;     // face scale
    vec4 uv;        // UV coordinates
    vec4 ao;        // Ambient occlusion values
    uint textureLayer;
    uint padding[3];
};

// Storage buffer for face instances
layout(binding = 2, std430) restrict readonly buffer FaceInstanceBuffer {
    FaceInstanceData instances[];
} faceInstanceBuffer;

// Quaternion to matrix conversion
mat4 quatToMat4(vec4 q) {
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

// Get face direction from scale.w
uint getFaceDirection(float scaleW) {
    return uint(scaleW);
}

mat4 reconstructModelMatrix() {
    return mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

vec3 transformQuadVertex(vec3 vertex, mat4 faceModel) {
    vec4 worldPos = faceModel * vec4(vertex, 1.0);
    return worldPos.xyz;
}

void main() {
    uint workgroupIndex = gl_WorkGroupID.x;
    uint localIndex = gl_LocalInvocationID.x;
    uint facesPerWorkgroup = 32;
    uint localFaceIndex = workgroupIndex * facesPerWorkgroup + localIndex;
    
    if (localFaceIndex >= payload.faceCount) {
        return;
    }
    
    uint faceIndex = payload.firstFaceIndex + localFaceIndex;
    
    uint startFace = workgroupIndex * facesPerWorkgroup;
    uint endFace = min(startFace + facesPerWorkgroup, payload.faceCount);
    uint facesInThisWorkgroup = endFace - startFace;
    
    if (facesInThisWorkgroup == 0) {
        return;
    }
    
    uint actualVertices = facesInThisWorkgroup * VERTICES_PER_FACE;
    uint actualTriangles = facesInThisWorkgroup * TRIANGLES_PER_FACE;
    
    if (localIndex == 0) {
        SetMeshOutputsEXT(actualVertices, actualTriangles);
    }
    barrier();
    
    mat4 cubeModel = reconstructModelMatrix();
    mat4 view = ubo.view;
    mat4 proj = ubo.proj;
    mat4 vp = proj * view;
    
    uint baseVertexIndex = localIndex * VERTICES_PER_FACE;
    uint baseTriangleIndex = localIndex * TRIANGLES_PER_FACE;
    
    // Get face instance data
    vec3 facePosition = faceInstanceBuffer.instances[faceIndex].position.xyz;
    vec4 faceRotation = faceInstanceBuffer.instances[faceIndex].rotation;
    vec3 faceScale = faceInstanceBuffer.instances[faceIndex].scale.xyz;
    float faceDirectionFloat = faceInstanceBuffer.instances[faceIndex].scale.w;
    vec4 faceUV = faceInstanceBuffer.instances[faceIndex].uv;
    uint textureLayer = faceInstanceBuffer.instances[faceIndex].textureLayer;
    
    // Get face direction
    uint faceDir = getFaceDirection(faceDirectionFloat);
    vec3 faceNormal = faceNormals[faceDir];
    
    // Create face transformation
    mat4 faceRotationMatrix = quatToMat4(faceRotation);
    mat4 faceTranslation = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(facePosition, 1.0)
    );
    mat4 faceModel = faceTranslation * faceRotationMatrix;
    
    // Transform normal to world space
    vec3 worldNormal = mat3(faceRotationMatrix) * faceNormal;
    
    mat4 mvp = vp * cubeModel;
    
    // UV coordinates
    float minU = faceUV.x / 16.0;
    float minV = faceUV.y / 16.0;
    float maxU = faceUV.z / 16.0;
    float maxV = faceUV.w / 16.0;
    
    vec2 texCoords[4] = {
        vec2(minU, minV),  // Top-left
        vec2(minU, maxV),  // Bottom-left  
        vec2(maxU, minV),  // Top-right
        vec2(maxU, maxV)   // Bottom-right
    };
    
    // Output vertices
    for (int i = 0; i < VERTICES_PER_FACE; i++) {
        uint vertexIndex = baseVertexIndex + i;
        
        vec3 scaledVertex = quadVertices[i] * faceScale;
        vec3 worldPos = transformQuadVertex(scaledVertex, faceModel);
        
        gl_MeshVerticesEXT[vertexIndex].gl_Position = mvp * vec4(worldPos, 1.0);
        
        v_out[vertexIndex].color = faceColors[faceDir];
        v_out[vertexIndex].texCoord = texCoords[i];
        v_out[vertexIndex].faceIndex = faceDir;
        v_out[vertexIndex].textureLayer = textureLayer;
        v_out[vertexIndex].worldPos = worldPos;
        v_out[vertexIndex].normal = worldNormal;
    }
    
    // Output triangles
    gl_PrimitiveTriangleIndicesEXT[baseTriangleIndex] = 
        uvec3(baseVertexIndex + 0, baseVertexIndex + 2, baseVertexIndex + 1);
    
    gl_PrimitiveTriangleIndicesEXT[baseTriangleIndex + 1] = 
        uvec3(baseVertexIndex + 1, baseVertexIndex + 2, baseVertexIndex + 3);
}