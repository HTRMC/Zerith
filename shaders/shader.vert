// shader.vert
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
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
    uint instance_data = instanceData.data[gl_InstanceIndex];
    int face_type = int(instance_data & 0x7);  // Lower 3 bits for face type
    int x = int((instance_data >> 3) & 0x1F);  // Next 5 bits for X position
    int y = int((instance_data >> 8) & 0x1F);  // Next 5 bits for Y position
    int z = int((instance_data >> 13) & 0x1F); // Next 5 bits for Z position
    int block_type = int((instance_data >> 18) & 0x3F); // Next 6 bits for block type

    // Rotate the vertex based on face type
    vec3 rotatedPos = rotateVertex(inPosition, face_type);

    // Apply position offset
    vec3 worldPos = rotatedPos + vec3(float(x), float(y), float(z));

    // Get face normal
    vec3 normal = getFaceNormal(face_type);

    // Final transformation
    gl_Position = ubo.proj * ubo.view * push.model * vec4(worldPos, 1.0);

    fragTexCoord = rotateUV(inTexCoord, face_type);
    fragTextureID = getTextureID(block_type, face_type);
    fragNormal = normal;
    fragPos = worldPos;
}