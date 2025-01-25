// shader.vert
#version 450
#define CHUNK_SIZE 16

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    uint instanceCount;
    vec3 padding;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in uint inTextureID;

// Face type constants
const int FACE_XN = 0;  // Left
const int FACE_XP = 1;  // Right
const int FACE_YN = 2;  // Back
const int FACE_YP = 3;  // Front
const int FACE_ZN = 4;  // Bottom
const int FACE_ZP = 5;  // Top

// Block type constants (matching your C++ enum)
const int BLOCK_AIR = 0;
const int BLOCK_STONE = 1;
const int BLOCK_GRASS_BLOCK = 8;
const int BLOCK_DIRT = 9;

layout(binding = 2) readonly buffer InstanceData {
    uint data[];
} instanceData;

struct ChunkPosition {
    vec3 position;
    uint instanceStart;
};

layout(binding = 3) readonly buffer ChunkPositions {
    ChunkPosition positions[];
} chunkPositions;

layout(binding = 4) readonly buffer ChunkIndices {
    uint indices[];
} chunkIndices;

layout(binding = 5) readonly buffer BlockTypes {
    uint blockTypes[];  // New buffer for block types
} blockTypeData;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out flat uint fragTextureID;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;

uint getTextureID(int blockType, int faceType) {
    switch(blockType) {
        case BLOCK_STONE:
            return 4; // Stone texture

        case BLOCK_GRASS_BLOCK:
            if (faceType == FACE_ZP) return 1;      // Top texture (grass)
            else if (faceType == FACE_ZN) return 0; // Bottom texture (dirt)
            else return 2;                          // Side texture (grass side)

        case BLOCK_DIRT:
            return 0; // Dirt texture

        default:
            return 5; // Missing texture
    }
}

vec3 getFaceNormal(int faceType) {
    switch(faceType) {
        case FACE_XP: return vec3(1.0, 0.0, 0.0);  // Right face (+X)
        case FACE_XN: return vec3(-1.0, 0.0, 0.0); // Left face (-X)
        case FACE_YP: return vec3(0.0, 1.0, 0.0);  // Front face (+Y)
        case FACE_YN: return vec3(0.0, -1.0, 0.0); // Back face (-Y)
        case FACE_ZP: return vec3(0.0, 0.0, 1.0);  // Top face (+Z)
        case FACE_ZN: return vec3(0.0, 0.0, -1.0); // Bottom face (-Z)
        default: return vec3(0.0, 1.0, 0.0);
    }
}

vec2 rotateUV(vec2 uv, int faceType) {
    switch(faceType) {
        case FACE_XP: // Right face (+X)
            return vec2(1.0 - uv.y, uv.x);
        case FACE_XN: // Left face (-X)
            return vec2(uv.y, 1.0 - uv.x);
        default:
            return uv;
    }
}

vec3 rotateVertex(vec3 pos, int faceType) {
    vec3 final = pos;

    switch(faceType) {
        case FACE_XP: // Right face (+X)
            final.xyz = vec3(1.0 - pos.z, pos.y, pos.x);
            break;
        case FACE_XN: // Left face (-X)
            final.xyz = vec3(pos.z, pos.y, 1.0 - pos.x);
            break;
        case FACE_YP: // Front face (+Y)
            final.xyz = vec3(pos.x, 1.0 - pos.z, pos.y);
            break;
        case FACE_YN: // Back face (-Y)
            final.xyz = vec3(1.0 - pos.x, pos.z, pos.y);
            break;
        case FACE_ZP: // Top face (+Z)
            final.xyz = vec3(pos.x, pos.y, 1.0 - pos.z);
            break;
        case FACE_ZN: // Bottom face (-Z)
            final.xyz = vec3(pos.x, pos.y, pos.z);
            break;
    }

    return final;
}

void main() {
    // Extract instance data
    uint instance_data = instanceData.data[gl_InstanceIndex];   // Get instance data

    int face_type = int(instance_data & 0x7);                   // Lower 3 bits for face type
    int x = int((instance_data >> 3) & 0xF);                    // Next 4 bits for X position
    int y = int((instance_data >> 7) & 0xF);                    // Next 4 bits for Y position
    int z = int((instance_data >> 11) & 0xF);                   // Next 4 bits for Z position
    int width = int((instance_data >> 15) & 0xF);               // Next 4 bits for width
    int height = int((instance_data >> 19) & 0xF);              // Next 4 bits for height


    vec3 scaledPos = vec3(
        inPosition.x * ((face_type == FACE_XN || face_type == FACE_XP) ? 1 : width),
        inPosition.y * ((face_type == FACE_ZN || face_type == FACE_ZP) ? height : 1),
        inPosition.z * ((face_type == FACE_YN || face_type == FACE_YP) ? width : 1)
    );

    vec2 finalUV = vec2(
        inTexCoord.x * ((face_type == FACE_XN || face_type == FACE_XP) ? 1 : width),
        inTexCoord.y * ((face_type == FACE_ZN || face_type == FACE_ZP) ? height : width)
    );

    int block_type = int(blockTypeData.blockTypes[gl_InstanceIndex]);

    uint chunkIndex = chunkIndices.indices[gl_InstanceIndex];

    // Rotate the vertex based on face type
    vec3 rotatedPos = rotateVertex(scaledPos, face_type);

    // Apply block position offset
    vec3 blockOffset = vec3(float(x), float(y), float(z));

    // Apply chunk position offset
    vec3 chunkOffset = chunkPositions.positions[chunkIndex].position;

    // Combine all positions
    vec3 worldPos = rotatedPos + blockOffset + chunkOffset;

    // Get face normal
    vec3 normal = getFaceNormal(face_type);

    // Final transformation
    gl_Position = ubo.proj * ubo.view * push.model * vec4(worldPos, 1.0);

    fragTexCoord = rotateUV(finalUV, face_type);
    fragTextureID = getTextureID(block_type, face_type);
    fragNormal = normal;
    fragPos = worldPos;
}