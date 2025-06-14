#include "binary_mesh_converter.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <set>

namespace Zerith {

std::vector<BinaryMeshConverter::FaceInstance> BinaryMeshConverter::convertQuadToFaces(
    const MeshQuad& quad,
    const glm::ivec3& chunkWorldPos,
    const BlockRegistry& blockRegistry,
    TextureArray& textureArray
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
    
    // Build the full texture path and register if needed
    std::string texturePath = "assets/zerith/textures/block/" + textureName + ".png";
    uint32_t textureLayer = textureArray.getOrRegisterTexture(texturePath);
    
    // Special handling for grass blocks - ensure overlay is also registered
    if (blockDef->getId() == "grass_block" && quad.faceDirection >= 2 && quad.faceDirection <= 5) {
        // Register the overlay texture to ensure it's available for the shader
        std::string overlayPath = "assets/zerith/textures/block/grass_block_side_overlay.png";
        textureArray.getOrRegisterTexture(overlayPath);
    }
    
    // Create the face instance
    faces.emplace_back(
        worldPos,           // position
        rotation,           // rotation quaternion
        scale,              // scale
        quad.faceDirection, // face direction
        uv,                 // UV coordinates
        textureLayer        // texture layer
    );
    
    return faces;
}

std::vector<BinaryMeshConverter::FaceInstance> BinaryMeshConverter::convertAllQuads(
    const std::vector<MeshQuad>& quads,
    const glm::ivec3& chunkWorldPos,
    const BlockRegistry& blockRegistry,
    TextureArray& textureArray
) {
    std::vector<FaceInstance> allFaces;
    allFaces.reserve(quads.size()); // At least one face per quad
    
    for (const auto& quad : quads) {
        auto quadFaces = convertQuadToFaces(quad, chunkWorldPos, blockRegistry, textureArray);
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
    
    // Get the base block ID
    std::string blockId = blockDef->getId();
    
    // Handle special cases for blocks with different textures per face
    // This is a simplified version - in a full implementation, this would
    // read from block model files or a texture configuration
    if (blockId == "grass_block") {
        switch (faceDirection) {
            case 0: return "dirt";                   // Down face
            case 1: return "grass_block_top";        // Up face
            default: return "grass_block_side";      // Side faces
        }
    }
    
    // For most blocks, use the block ID as the texture name
    return blockId;
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
    const BlockRegistry& blockRegistry,
    TextureArray& textureArray
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
        auto faces = BinaryMeshConverter::convertAllQuads(simpleQuads, chunkWorldPos, blockRegistry, textureArray);
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
    
    // For now, only use binary meshing for simple full cube blocks
    // This is a conservative approach to ensure complex blocks render correctly
    std::string blockId = blockDef->getId();
    
    // List of known full cube blocks that can use binary meshing
    static const std::set<std::string> fullCubeBlocks = {
        "stone", "granite", "diorite", "andesite",
        "deepslate", "cobblestone", "dirt", "grass_block",
        "planks", "oak_planks", "spruce_planks", "birch_planks",
        "jungle_planks", "acacia_planks", "dark_oak_planks",
        "sand", "gravel", "gold_ore", "iron_ore", "coal_ore",
        "log", "oak_log", "spruce_log", "birch_log",
        "sponge", "glass", "lapis_ore", "lapis_block",
        "sandstone", "wool", "gold_block", "iron_block",
        "brick", "bookshelf", "mossy_cobblestone", "obsidian",
        "diamond_ore", "diamond_block", "redstone_ore",
        "ice", "snow_block", "clay", "netherrack",
        "glowstone", "stone_bricks", "nether_bricks",
        "end_stone", "emerald_ore", "emerald_block",
        "quartz_block", "prismarine", "hay_block",
        "terracotta", "coal_block", "packed_ice",
        "red_sandstone", "purpur_block"
    };
    
    return fullCubeBlocks.find(blockId) != fullCubeBlocks.end();
}

std::vector<HybridChunkMeshGenerator::FaceInstance> HybridChunkMeshGenerator::generateComplexBlockMesh(
    const Chunk& chunk,
    const glm::ivec3& chunkWorldPos,
    const BlockRegistry& blockRegistry,
    const std::vector<BlockType>& complexBlockTypes
) {
    std::vector<FaceInstance> faces;
    
    // For complex blocks, we need to fall back to the traditional per-block meshing
    // This ensures stairs, slabs, and other non-cubic blocks render correctly
    
    // Create a set for quick lookup
    std::set<BlockType> complexSet(complexBlockTypes.begin(), complexBlockTypes.end());
    
    // Iterate through all blocks in the chunk
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                BlockType blockType = chunk.getBlock(x, y, z);
                
                // Skip if not a complex block
                if (complexSet.find(blockType) == complexSet.end()) {
                    continue;
                }
                
                // Skip air blocks
                auto blockDef = blockRegistry.getBlock(blockType);
                if (!blockDef || blockDef->getId() == "air") {
                    continue;
                }
                
                // For now, create a simple cube face for complex blocks
                // In a real implementation, this would load the actual model geometry
                glm::vec3 blockWorldPos = glm::vec3(chunkWorldPos) * static_cast<float>(Chunk::CHUNK_SIZE);
                blockWorldPos += glm::vec3(x, y, z);
                
                // Generate faces for all 6 directions if visible
                for (int dir = 0; dir < 6; ++dir) {
                    // Check face visibility
                    int dx = 0, dy = 0, dz = 0;
                    switch (dir) {
                        case 0: dy = -1; break; // Down
                        case 1: dy = 1; break;  // Up
                        case 2: dz = -1; break; // North
                        case 3: dz = 1; break;  // South
                        case 4: dx = -1; break; // West
                        case 5: dx = 1; break;  // East
                    }
                    
                    if (!chunk.isFaceVisible(x, y, z, dx, dy, dz)) {
                        continue;
                    }
                    
                    // Get texture and create face
                    std::string textureName = blockDef->getId();
                    uint32_t textureLayer = 0; // Default texture
                    
                    // Create face instance
                    faces.emplace_back(
                        blockWorldPos,
                        BinaryMeshConverter::getFaceRotation(dir),
                        glm::vec3(1.0f), // Unit scale for now
                        dir,
                        BinaryMeshConverter::getDefaultFaceUV(),
                        textureLayer
                    );
                }
            }
        }
    }
    
    return faces;
}

} // namespace Zerith