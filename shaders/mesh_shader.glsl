#version 450
#extension GL_EXT_mesh_shader : require
#extension GL_EXT_nonuniform_qualifier : require

/*
Coordinate System Details:

This shader uses Vulkan's standard coordinate system (Y-up, right-handed):
- Y is up
- X is right  
- Z is forward (into the screen)

The face instances below define cube faces positioned from (0,0,0) to (1,1,1)
with proper orientations for each face of a unit cube.
*/

// Define a single quad (unit square in XY plane at Z=0)
// This will be instanced for each face
const vec3 quadVertices[4] = {
    vec3(0.0, 1.0, 0.0),  // 0: Top-left
    vec3(0.0, 0.0, 0.0),  // 1: Bottom-left  
    vec3(1.0, 1.0, 0.0),  // 2: Top-right
    vec3(1.0, 0.0, 0.0)   // 3: Bottom-right
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
const uint FACES_PER_WORKGROUP = 32;  // Faces per mesh shader workgroup
const uint MAX_VERTICES = FACES_PER_WORKGROUP * VERTICES_PER_FACE;  // 128 vertices
const uint MAX_TRIANGLES = FACES_PER_WORKGROUP * TRIANGLES_PER_FACE;  // 64 triangles

// Mesh shader configuration - dynamic face count
layout(local_size_x = 32) in;
layout(triangles, max_vertices = MAX_VERTICES, max_primitives = MAX_TRIANGLES) out;

// Must match task shader's structure
struct MeshTaskPayload {
    uint chunkIndex;
    uint firstFaceIndex;
    uint faceCount;
};

// Then declare it as shared
taskPayloadSharedEXT MeshTaskPayload payload;

// Output to fragment shader
layout(location = 0) out PerVertexData {
    vec3 color;
    vec2 texCoord;
    flat uint faceIndex;
    flat uint textureLayer;
    float ao; // Per-vertex ambient occlusion
} v_out[];

// Updated uniform buffer with packed data
layout(binding = 0) uniform UniformBufferObject {
    float time;           // Time for animation
    mat4 view;            // View matrix
    mat4 proj;            // Projection matrix
    uint faceCount;       // Number of face instances to render
    vec4 cameraPos;       // Camera position for LOD
    vec4 frustumPlanes[6]; // Frustum planes for culling
} ubo;

// Push constants removed - using UBO directly for better performance

// Face instance structure for storage buffer
struct FaceInstanceData {
    vec4 position;  // vec3 + padding
    vec4 rotation;  // quaternion
    vec4 scale;     // face scale (width, height, 1.0, faceDirection)
    vec4 uv;        // UV coordinates [minU, minV, maxU, maxV]
    vec4 ao;        // Ambient occlusion values for 4 vertices
    uint textureLayer;  // Texture array layer index
    uint padding[3];    // Padding to maintain 16-byte alignment
};

// Storage buffer for dynamic face instances
layout(binding = 2, std430) restrict readonly buffer FaceInstanceBuffer {
    FaceInstanceData instances[];
} faceInstanceBuffer;

// Storage buffer for chunk block data (for GPU AO calculation)
layout(binding = 6, std430) restrict readonly buffer ChunkBlockBuffer {
    uint blockData[];  // Flattened 3D array: blockData[x + y*32 + z*32*32]
} chunkBlockBuffer;

// GPU AO calculation functions
bool isBlockOccluding(uint blockType) {
    // Air block (0) doesn't occlude
    if (blockType == 0u) return false;
    
    // Water and other transparent liquids don't occlude (assuming water is block type 290)
    // Note: This should match the block types from Blocks.h
    if (blockType == 290u) return false; // WATER
    if (blockType == 291u) return false; // LAVA
    
    // Glass and other transparent blocks don't occlude (simplified check)
    if (blockType == 356u) return false; // GLASS
    
    // All other blocks occlude for AO
    return true;
}

uint getBlockAt(int x, int y, int z) {
    // Check bounds
    if (x < 0 || x >= 32 || y < 0 || y >= 32 || z < 0 || z >= 32) {
        return 0u; // Treat out-of-bounds as air to prevent black chunk borders
    }
    
    uint index = uint(x + y * 32 + z * 32 * 32);
    return chunkBlockBuffer.blockData[index];
}

float calculateVertexAO(int blockX, int blockY, int blockZ, 
                       int dx1, int dy1, int dz1,  // First edge neighbor
                       int dx2, int dy2, int dz2,  // Second edge neighbor  
                       int dx3, int dy3, int dz3)  // Corner neighbor
{
    // Check the two edge neighbors
    bool side1 = isBlockOccluding(getBlockAt(blockX + dx1, blockY + dy1, blockZ + dz1));
    bool side2 = isBlockOccluding(getBlockAt(blockX + dx2, blockY + dy2, blockZ + dz2));
    
    // Check the corner neighbor
    bool corner = isBlockOccluding(getBlockAt(blockX + dx3, blockY + dy3, blockZ + dz3));
    
    // Classic AO algorithm - more visible values for testing
    if (side1 && side2) {
        return 0.3; // Heavily occluded but still visible
    }
    
    int occluders = (side1 ? 1 : 0) + (side2 ? 1 : 0) + (corner ? 1 : 0);
    
    switch (occluders) {
        case 0: return 1.0;    // No occlusion - full brightness
        case 1: return 0.9;    // One neighbor - very slight shadow
        case 2: return 0.7;    // Two neighbors - noticeable shadow
        case 3: return 0.4;    // All three neighbors - dark shadow
        default: return 0.3;   // Fallback - very dark
    }
}

vec4 calculateFaceAOGPU(int blockX, int blockY, int blockZ, int faceDirection) {
    vec4 ao = vec4(1.0);
    
    // Calculate AO for each vertex based on face direction
    switch (faceDirection) {
        case 0: // Down face (Y-)
            ao.x = calculateVertexAO(blockX, blockY, blockZ,  1, 0, 0,  0, 0, 1,  1, 0, 1); // TR
            ao.y = calculateVertexAO(blockX, blockY, blockZ, -1, 0, 0,  0, 0,-1, -1, 0,-1); // BL
            ao.z = calculateVertexAO(blockX, blockY, blockZ, -1, 0, 0,  0, 0, 1, -1, 0, 1); // TL
            ao.w = calculateVertexAO(blockX, blockY, blockZ,  1, 0, 0,  0, 0,-1,  1, 0,-1); // BR
            break;
            
        case 1: // Up face (Y+)
            ao.x = calculateVertexAO(blockX, blockY, blockZ,  1, 0, 0,  0, 0, 1,  1, 0, 1); // TR
            ao.y = calculateVertexAO(blockX, blockY, blockZ, -1, 0, 0,  0, 0,-1, -1, 0,-1); // BL
            ao.z = calculateVertexAO(blockX, blockY, blockZ, -1, 0, 0,  0, 0, 1, -1, 0, 1); // TL
            ao.w = calculateVertexAO(blockX, blockY, blockZ,  1, 0, 0,  0, 0,-1,  1, 0,-1); // BR
            break;
            
        case 2: // North face (Z-)
            ao.x = calculateVertexAO(blockX, blockY, blockZ,  1, 0, 0,  0, 1, 0,  1, 1, 0); // TR
            ao.y = calculateVertexAO(blockX, blockY, blockZ, -1, 0, 0,  0,-1, 0, -1,-1, 0); // BL
            ao.z = calculateVertexAO(blockX, blockY, blockZ, -1, 0, 0,  0, 1, 0, -1, 1, 0); // TL
            ao.w = calculateVertexAO(blockX, blockY, blockZ,  1, 0, 0,  0,-1, 0,  1,-1, 0); // BR
            break;
            
        case 3: // South face (Z+)
            ao.x = calculateVertexAO(blockX, blockY, blockZ,  1, 0, 0,  0, 1, 0,  1, 1, 0); // TR
            ao.y = calculateVertexAO(blockX, blockY, blockZ, -1, 0, 0,  0,-1, 0, -1,-1, 0); // BL
            ao.z = calculateVertexAO(blockX, blockY, blockZ, -1, 0, 0,  0, 1, 0, -1, 1, 0); // TL
            ao.w = calculateVertexAO(blockX, blockY, blockZ,  1, 0, 0,  0,-1, 0,  1,-1, 0); // BR
            break;
            
        case 4: // West face (X-)
            ao.x = calculateVertexAO(blockX, blockY, blockZ,  0, 0, 1,  0, 1, 0,  0, 1, 1); // TR
            ao.y = calculateVertexAO(blockX, blockY, blockZ,  0, 0,-1,  0,-1, 0,  0,-1,-1); // BL
            ao.z = calculateVertexAO(blockX, blockY, blockZ,  0, 0,-1,  0, 1, 0,  0, 1,-1); // TL
            ao.w = calculateVertexAO(blockX, blockY, blockZ,  0, 0, 1,  0,-1, 0,  0,-1, 1); // BR
            break;
            
        case 5: // East face (X+)
            ao.x = calculateVertexAO(blockX, blockY, blockZ,  0, 0,-1,  0, 1, 0,  0, 1,-1); // TR
            ao.y = calculateVertexAO(blockX, blockY, blockZ,  0, 0, 1,  0,-1, 0,  0,-1, 1); // BL
            ao.z = calculateVertexAO(blockX, blockY, blockZ,  0, 0, 1,  0, 1, 0,  0, 1, 1); // TL
            ao.w = calculateVertexAO(blockX, blockY, blockZ,  0, 0,-1,  0,-1, 0,  0,-1,-1); // BR
            break;
    }
    
    return ao;
}

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


// Helper to transform a quad vertex based on face transformation
vec3 transformQuadVertex(vec3 vertex, mat4 faceModel) {
    // Apply face transformation to get the world position
    vec4 worldPos = faceModel * vec4(vertex, 1.0);
    return worldPos.xyz;
}

void main() {
    // Calculate the face index within this chunk
    uint workgroupIndex = gl_WorkGroupID.x;
    uint localIndex = gl_LocalInvocationID.x;
    uint facesPerWorkgroup = 32;
    uint localFaceIndex = workgroupIndex * facesPerWorkgroup + localIndex;
    
    // Early exit if this invocation is beyond this chunk's face count
    if (localFaceIndex >= payload.faceCount) {
        return;
    }
    
    // Calculate global face index
    uint faceIndex = payload.firstFaceIndex + localFaceIndex;
    
    // Calculate how many faces this workgroup will process
    uint startFace = workgroupIndex * facesPerWorkgroup;
    uint endFace = min(startFace + facesPerWorkgroup, payload.faceCount);
    uint facesInThisWorkgroup = endFace - startFace;
    
    // Early exit if no faces to process
    if (facesInThisWorkgroup == 0) {
        return;
    }
    
    // Set the number of vertices and primitives for this workgroup
    uint actualVertices = facesInThisWorkgroup * VERTICES_PER_FACE;
    uint actualTriangles = facesInThisWorkgroup * TRIANGLES_PER_FACE;
    
    // Only the first thread in the workgroup should set the output counts
    if (localIndex == 0) {
        SetMeshOutputsEXT(actualVertices, actualTriangles);
    }
    barrier();
    
    // Use matrices directly from UBO
    mat4 cubeModel = reconstructModelMatrix();  // Overall cube rotation (still using time-based animation)
    mat4 view = ubo.view;
    mat4 proj = ubo.proj;
    mat4 vp = proj * view;
    
    // Base index for this face's vertices in the output arrays (relative to workgroup)
    uint baseVertexIndex = localIndex * VERTICES_PER_FACE;
    uint baseTriangleIndex = localIndex * TRIANGLES_PER_FACE;
    
    // Get face instance data from storage buffer
    vec3 facePosition = faceInstanceBuffer.instances[faceIndex].position.xyz;
    vec4 faceRotation = faceInstanceBuffer.instances[faceIndex].rotation;
    vec3 faceScale = faceInstanceBuffer.instances[faceIndex].scale.xyz;
    vec4 faceUV = faceInstanceBuffer.instances[faceIndex].uv;
    uint textureLayer = faceInstanceBuffer.instances[faceIndex].textureLayer;
    
    // Use pre-calculated AO values from CPU for now (GPU AO is experimental)
    vec4 faceAO = faceInstanceBuffer.instances[faceIndex].ao;
    
    // Create model matrix for this face
    mat4 faceRotationMatrix = quatToMat4(faceRotation);
    mat4 faceTranslation = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(facePosition, 1.0)
    );
    mat4 faceModel = faceTranslation * faceRotationMatrix;
    
    // Combine with overall cube animation
    mat4 mvp = vp * cubeModel;
    
    // Set texture coordinates for the quad using the UV data from the model
    // Convert from pixel coordinates (0-16) to normalized coordinates (0-1)
    float minU = faceUV.x / 16.0;
    float minV = faceUV.y / 16.0;
    float maxU = faceUV.z / 16.0;
    float maxV = faceUV.w / 16.0;
    
    // Note: In texture coordinates, (0,0) is top-left in Vulkan
    vec2 texCoords[4] = {
        vec2(minU, minV),  // 0: Top-left
        vec2(minU, maxV),  // 1: Bottom-left  
        vec2(maxU, minV),  // 2: Top-right
        vec2(maxU, maxV)   // 3: Bottom-right
    };
    
    // Output vertices for this face
    for (int i = 0; i < VERTICES_PER_FACE; i++) {
        uint vertexIndex = baseVertexIndex + i;
        
        // Apply face scaling to the quad vertex
        vec3 scaledVertex = quadVertices[i] * faceScale;
        
        // Transform scaled quad vertex to face position and orientation
        vec3 worldPos = transformQuadVertex(scaledVertex, faceModel);
        
        // Apply overall cube animation and projection
        gl_MeshVerticesEXT[vertexIndex].gl_Position = mvp * vec4(worldPos, 1.0);
        
        // Set per-vertex data
        v_out[vertexIndex].color = faceColors[faceIndex];
        v_out[vertexIndex].texCoord = texCoords[i];
        v_out[vertexIndex].faceIndex = faceIndex;
        v_out[vertexIndex].textureLayer = textureLayer;
        
        // Assign AO value based on vertex index
        // AO components: x=top-left, y=bottom-left, z=top-right, w=bottom-right
        float aoValues[4] = { faceAO.x, faceAO.y, faceAO.z, faceAO.w };
        v_out[vertexIndex].ao = aoValues[i];
    }
    
    // Output triangles for this face using triangle strip ordering
    // With vertices: 0=TL, 1=BL, 2=TR, 3=BR
    // Triangle 1: 0-2-1 (Top-left, Top-right, Bottom-left) - counter-clockwise for outward face
    gl_PrimitiveTriangleIndicesEXT[baseTriangleIndex] = 
        uvec3(baseVertexIndex + 0, baseVertexIndex + 2, baseVertexIndex + 1);
    
    // Triangle 2: 1-2-3 (Bottom-left, Top-right, Bottom-right) - counter-clockwise for outward face
    gl_PrimitiveTriangleIndicesEXT[baseTriangleIndex + 1] = 
        uvec3(baseVertexIndex + 1, baseVertexIndex + 2, baseVertexIndex + 3);
}