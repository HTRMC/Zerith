#include "binary_mesh_converter.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

namespace Zerith {

std::vector<BinaryMeshConverter::FaceInstance> BinaryMeshConverter::convertQuadToFaces(
    const MeshQuad& quad,
    const glm::ivec3& chunkWorldPos,
    const BlockRegistry& blockRegistry
) {
    std::vector<FaceInstance> faces;
    
    // Get block information
    auto blockDef = blockRegistry.getBlock(quad.blockType);
    if (!blockDef) {
        return faces; // Unknown block type
    }
    
    // Calculate world position
    glm::vec3 worldPos = calculateQuadWorldPosition(quad, chunkWorldPos);
    
    // Calculate scale based on quad dimensions
    glm::vec3 scale = calculateQuadScale(quad);
    
    // Get rotation for this face direction
    glm::vec4 rotation = getFaceRotation(quad.faceDirection);
    
    // Get texture information
    std::string textureName = getBlockTexture(quad.blockType, quad.faceDirection, blockRegistry);
    
    // Calculate UV coordinates with proper tiling
    glm::vec4 uv = calculateQuadUV(quad, *blockDef, quad.faceDirection);
    
    // Create the face instance
    faces.emplace_back(
        worldPos,           // position
        rotation,           // rotation quaternion
        scale,              // scale
        quad.faceDirection, // face direction
        uv,                 // UV coordinates
        0,                  // texture layer (to be set by texture system)
        textureName         // texture name
    );
    
    return faces;
}

std::vector<BinaryMeshConverter::FaceInstance> BinaryMeshConverter::convertAllQuads(
    const std::vector<MeshQuad>& quads,
    const glm::ivec3& chunkWorldPos,
    const BlockRegistry& blockRegistry
) {
    std::vector<FaceInstance> allFaces;
    allFaces.reserve(quads.size()); // At least one face per quad
    
    for (const auto& quad : quads) {
        auto quadFaces = convertQuadToFaces(quad, chunkWorldPos, blockRegistry);
        allFaces.insert(allFaces.end(), quadFaces.begin(), quadFaces.end());
    }
    
    return allFaces;
}

glm::vec4 BinaryMeshConverter::calculateQuadUV(
    const MeshQuad& quad,
    const BlockDefinition& blockDef,
    int faceDirection
) {
    // Start with default UV coordinates
    glm::vec4 baseUV = getDefaultFaceUV();
    
    // Adjust UV for tiling based on quad size
    return adjustUVForTiling(baseUV, quad.size, faceDirection);
}

std::string BinaryMeshConverter::getBlockTexture(
    BlockType blockType,
    int faceDirection,
    const BlockRegistry& blockRegistry
) {
    auto blockDef = blockRegistry.getBlock(blockType);
    if (!blockDef) {
        return "missing_texture";
    }
    
    // Try to get face-specific texture from block definition
    // This would need to be implemented based on your block registry structure
    // For now, return a generic texture name
    return blockDef->getId() + "_texture";
}

glm::vec3 BinaryMeshConverter::calculateQuadWorldPosition(
    const MeshQuad& quad,
    const glm::ivec3& chunkWorldPos
) {
    // Convert chunk coordinates to world coordinates
    glm::vec3 chunkWorldOffset = glm::vec3(chunkWorldPos) * static_cast<float>(Chunk::CHUNK_SIZE);
    glm::vec3 basePos = chunkWorldOffset + glm::vec3(quad.position);
    
    // Apply face-specific corner offsets to match original BlockbenchInstanceGenerator behavior
    switch (quad.faceDirection) {
        case 0: // Down face - position at back corner (from.x, from.y, to.z)
            return basePos + glm::vec3(0.0f, 0.0f, static_cast<float>(quad.size.z));
        case 1: // Up face - position at front-top corner (from.x, to.y, from.z)
            return basePos + glm::vec3(0.0f, static_cast<float>(quad.size.y), 0.0f);
        case 2: // North face - no offset needed (from.x, from.y, from.z)
            return basePos;
        case 3: // South face - position at back-bottom corner (to.x, from.y, to.z)
            return basePos + glm::vec3(static_cast<float>(quad.size.x), 0.0f, static_cast<float>(quad.size.z));
        case 4: // West face - position at back corner (from.x, from.y, to.z)
            return basePos + glm::vec3(0.0f, 0.0f, static_cast<float>(quad.size.z));
        case 5: // East face - position at front corner (to.x, from.y, from.z)
            return basePos + glm::vec3(static_cast<float>(quad.size.x), 0.0f, 0.0f);
        default:
            return basePos;
    }
}

glm::vec3 BinaryMeshConverter::calculateQuadScale(const MeshQuad& quad) {
    // Map quad dimensions to correct axes based on face direction
    // This matches the original BlockbenchInstanceGenerator scale calculations
    switch (quad.faceDirection) {
        case 0: // Down face (XZ plane)
        case 1: // Up face (XZ plane)
            return glm::vec3(static_cast<float>(quad.size.x), static_cast<float>(quad.size.z), 1.0f);
        case 2: // North face (XY plane)
        case 3: // South face (XY plane)
            return glm::vec3(static_cast<float>(quad.size.x), static_cast<float>(quad.size.y), 1.0f);
        case 4: // West face (ZY plane)
        case 5: // East face (ZY plane)
            return glm::vec3(static_cast<float>(quad.size.z), static_cast<float>(quad.size.y), 1.0f);
        default:
            return glm::vec3(static_cast<float>(quad.size.x), static_cast<float>(quad.size.y), static_cast<float>(quad.size.z));
    }
}

glm::vec4 BinaryMeshConverter::getFaceRotation(int faceDirection) {
    // Use the same rotation logic as the original generator
    glm::quat rotation;
    
    switch (faceDirection) {
        case 0: // Down face (Y-)
            rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            break;
        case 1: // Up face (Y+)
            rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            break;
        case 2: // North face (Z-)
            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity
            break;
        case 3: // South face (Z+)
            rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        case 4: // West face (X-)
            rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        case 5: // East face (X+)
            rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        default:
            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity
            break;
    }
    
    // Convert quaternion to vec4
    return glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w);
}

glm::vec4 BinaryMeshConverter::adjustUVForTiling(
    const glm::vec4& baseUV,
    const glm::ivec3& quadSize,
    int faceDirection
) {
    glm::vec4 tiledUV = baseUV;
    
    // Calculate tiling factors based on quad dimensions
    glm::vec2 tileFactor;
    
    switch (faceDirection) {
        case 0: // Down (XZ plane)
        case 1: // Up (XZ plane)
            tileFactor = glm::vec2(quadSize.x, quadSize.z);
            break;
        case 2: // North (XY plane)
        case 3: // South (XY plane)
            tileFactor = glm::vec2(quadSize.x, quadSize.y);
            break;
        case 4: // West (ZY plane)
        case 5: // East (ZY plane)
            tileFactor = glm::vec2(quadSize.z, quadSize.y);
            break;
    }
    
    // Scale UV coordinates to repeat texture across the merged quad
    // This maintains the texture appearance as if each block still has its own texture
    // UV coordinates are in pixel coordinates (0-16 per block)
    tiledUV.z = baseUV.z * tileFactor.x; // U max = 16 * quad_width
    tiledUV.w = baseUV.w * tileFactor.y; // V max = 16 * quad_height
    
    return tiledUV;
}

glm::vec4 BinaryMeshConverter::getDefaultFaceUV() {
    // Default UV coordinates covering the full texture (in pixel coordinates 0-16)
    return glm::vec4(0.0f, 0.0f, 16.0f, 16.0f);
}

// Hybrid Chunk Mesh Generator Implementation

std::vector<HybridChunkMeshGenerator::FaceInstance> HybridChunkMeshGenerator::generateOptimizedMesh(
    const Chunk& chunk,
    const glm::ivec3& chunkWorldPos,
    const BlockRegistry& blockRegistry
) {
    std::vector<FaceInstance> allFaces;
    
    // Create binary chunk data
    BinaryChunkData binaryData(chunk);
    
    // Separate block types into simple and complex
    std::vector<BlockType> simpleBlocks;
    std::vector<BlockType> complexBlocks;
    
    for (BlockType blockType : binaryData.getActiveBlockTypes()) {
        if (canUseBinaryMeshing(blockType, blockRegistry)) {
            simpleBlocks.push_back(blockType);
        } else {
            complexBlocks.push_back(blockType);
        }
    }
    
    // Generate mesh for simple blocks using binary greedy meshing
    if (!simpleBlocks.empty()) {
        // Use binary greedy meshing for all simple blocks at once
        auto allQuads = BinaryGreedyMesher::generateAllQuads(binaryData);
        
        // Filter quads to only include simple block types
        std::vector<BinaryGreedyMesher::MeshQuad> simpleQuads;
        for (const auto& quad : allQuads) {
            if (std::find(simpleBlocks.begin(), simpleBlocks.end(), quad.blockType) != simpleBlocks.end()) {
                simpleQuads.push_back(quad);
            }
        }
        
        // Convert all simple quads to faces
        auto faces = BinaryMeshConverter::convertAllQuads(simpleQuads, chunkWorldPos, blockRegistry);
        allFaces.insert(allFaces.end(), faces.begin(), faces.end());
    }
    
    // Generate mesh for complex blocks using traditional method
    if (!complexBlocks.empty()) {
        auto complexFaces = generateComplexBlockMesh(chunk, chunkWorldPos, blockRegistry, complexBlocks);
        allFaces.insert(allFaces.end(), complexFaces.begin(), complexFaces.end());
    }
    
    return allFaces;
}

bool HybridChunkMeshGenerator::canUseBinaryMeshing(
    BlockType blockType,
    const BlockRegistry& blockRegistry
) {
    auto blockDef = blockRegistry.getBlock(blockType);
    if (!blockDef) {
        return false;
    }
    
    // For now, assume all blocks can use binary meshing
    // In a real implementation, you'd check if the block is a simple full cube
    // Complex blocks like stairs, slabs, etc. would return false
    
    // Example criteria:
    // - Block must be a full cube (16x16x16 units)
    // - Block must not have complex geometry
    // - Block must not be transparent (or handle transparency separately)
    
    return true; // Simplified for this implementation
}

std::vector<HybridChunkMeshGenerator::FaceInstance> HybridChunkMeshGenerator::generateComplexBlockMesh(
    const Chunk& chunk,
    const glm::ivec3& chunkWorldPos,
    const BlockRegistry& blockRegistry,
    const std::vector<BlockType>& complexBlockTypes
) {
    std::vector<FaceInstance> faces;
    
    // This would integrate with your existing chunk mesh generator
    // For now, return empty vector as this needs to be implemented
    // based on your existing complex block meshing logic
    
    // TODO: Integrate with existing ChunkMeshGenerator for complex blocks
    
    return faces;
}

} // namespace Zerith