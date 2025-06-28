#include "binary_mesh_converter.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <set>

#include "blockbench_parser.h"
#include "voxel_ao.h"
#include "chunk_manager.h"
#include "logger.h"

namespace Zerith {

std::vector<BinaryMeshConverter::FaceInstance> BinaryMeshConverter::convertQuadToFaces(
    const MeshQuad& quad,
    const glm::ivec3& chunkWorldPos,
    TextureArray& textureArray
) {
    std::vector<FaceInstance> faces;
    
    // Get block information
    auto blockDef = Blocks::getBlock(quad.blockType);
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
    std::string textureName = getBlockTexture(quad.blockType, quad.faceDirection);
    
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
    
    // Get the render layer for this block type
    Zerith::RenderLayer renderLayer = Zerith::Blocks::getRenderLayer(quad.blockType);
    
    // Create the face instance with the correct render layer
    faces.emplace_back(
        worldPos,           // position
        rotation,           // rotation quaternion
        scale,              // scale
        quad.faceDirection, // face direction
        uv,                 // UV coordinates
        textureLayer,       // texture layer
        renderLayer,        // render layer
        glm::vec4(1.0f)     // default AO (will be calculated later if needed)
    );
    
    return faces;
}

std::vector<BinaryMeshConverter::FaceInstance> BinaryMeshConverter::convertAllQuads(
    const std::vector<MeshQuad>& quads,
    const glm::ivec3& chunkWorldPos,
    TextureArray& textureArray
) {
    std::vector<FaceInstance> allFaces;
    allFaces.reserve(quads.size()); // At least one face per quad
    
    for (const auto& quad : quads) {
        auto quadFaces = convertQuadToFaces(quad, chunkWorldPos, textureArray);
        allFaces.insert(allFaces.end(), quadFaces.begin(), quadFaces.end());
    }
    
    return allFaces;
}

std::vector<BinaryMeshConverter::FaceInstance> BinaryMeshConverter::convertAllQuadsWithAO(
    const std::vector<MeshQuad>& quads,
    const glm::ivec3& chunkWorldPos,
    const Chunk& chunk,
    TextureArray& textureArray
) {
    std::vector<FaceInstance> allFaces;
    allFaces.reserve(quads.size());
    
    for (const auto& quad : quads) {
        auto quadFaces = convertQuadToFaces(quad, chunkWorldPos, textureArray);
        
        // Calculate AO for each face generated from this quad
        for (auto& face : quadFaces) {
            // For greedy meshed quads, use the original quad position for AO calculation
            // This represents the bottom-left corner of the merged area
            int x = quad.position.x;
            int y = quad.position.y;
            int z = quad.position.z;
            
            // For merged quads, we use default AO (no occlusion)
            // Cross-chunk AO not available in binary meshing without ChunkManager
            face.ao = glm::vec4(1.0f);
        }
        
        allFaces.insert(allFaces.end(), quadFaces.begin(), quadFaces.end());
    }
    
    return allFaces;
}

std::vector<BinaryMeshConverter::FaceInstance> BinaryMeshConverter::convertAllQuadsWithAO(
    const std::vector<MeshQuad>& quads,
    const glm::ivec3& chunkWorldPos,
    const ChunkManager* chunkManager,
    TextureArray& textureArray
) {
    LOG_INFO("BINARY MESH CONVERTER: Converting %zu quads with AO enabled (ChunkManager: %s)", 
             quads.size(), chunkManager ? "available" : "null");
    
    std::vector<FaceInstance> allFaces;
    allFaces.reserve(quads.size());
    
    for (const auto& quad : quads) {
        auto quadFaces = convertQuadToFaces(quad, chunkWorldPos, textureArray);
        
        // Calculate AO for each face generated from this quad
        for (auto& face : quadFaces) {
            // For binary meshed quads, we need to calculate AO that accounts for the larger face size
            // Use the center of the quad for AO calculation
            int centerX = quad.position.x + quad.size.x / 2;
            int centerY = quad.position.y + quad.size.y / 2;
            int centerZ = quad.position.z + quad.size.z / 2;
            
            if (chunkManager) {
                // Convert chunk coordinates to world coordinates (multiply by CHUNK_SIZE)
                glm::ivec3 chunkWorldPosInWorldCoords = chunkWorldPos * Chunk::CHUNK_SIZE;
                
                LOG_INFO("BINARY AO DEBUG: chunkPos=(%d,%d,%d), chunkWorldPos=(%d,%d,%d), localPos=(%d,%d,%d)", 
                         chunkWorldPos.x, chunkWorldPos.y, chunkWorldPos.z,
                         chunkWorldPosInWorldCoords.x, chunkWorldPosInWorldCoords.y, chunkWorldPosInWorldCoords.z,
                         centerX, centerY, centerZ);
                
                // Calculate AO using cross-chunk support
                face.ao = VoxelAO::calculateFaceAO(chunkManager, chunkWorldPosInWorldCoords, centerX, centerY, centerZ, quad.faceDirection);
                
                // Check if VoxelAO debug mode is enabled
                if (face.ao.x == face.ao.y && face.ao.y == face.ao.z && face.ao.z == face.ao.w) {
                    LOG_INFO("BINARY AO DEBUG: All AO values identical (%.2f) - might be debug mode or uniform occlusion", face.ao.x);
                }
                LOG_INFO("BINARY AO: Calculated AO for quad at (%d,%d,%d) face %d: (%.2f,%.2f,%.2f,%.2f)", 
                         centerX, centerY, centerZ, quad.faceDirection, 
                         face.ao.x, face.ao.y, face.ao.z, face.ao.w);
                
                // Debug: Check if AO values are all zero (completely black)
                if (face.ao.x == 0.0f && face.ao.y == 0.0f && face.ao.z == 0.0f && face.ao.w == 0.0f) {
                    LOG_WARN("BINARY AO: All AO values are 0.0 (completely black) - this indicates full occlusion");
                    
                    // Temporarily override with partial AO to test if this is the issue
                    face.ao = glm::vec4(0.6f); // Medium gray instead of black
                    LOG_INFO("BINARY AO: Overriding with test AO values (0.6,0.6,0.6,0.6) to check if this fixes the issue");
                }
                
                // For larger faces, we might want to sample multiple points and average them
                // This provides better AO for merged faces that span multiple blocks
                if (quad.size.x > 1 || quad.size.y > 1 || quad.size.z > 1) {
                    // Sample the four corners of the quad for more accurate AO on large faces
                    glm::vec4 cornerAO[4];
                    
                    // Calculate sample positions based on face orientation
                    glm::ivec3 samples[4];
                    calculateQuadCornerSamples(quad, samples);
                    
                    // Sample AO at each corner
                    for (int i = 0; i < 4; i++) {
                        cornerAO[i] = VoxelAO::calculateFaceAO(chunkManager, chunkWorldPosInWorldCoords, 
                                                             samples[i].x, samples[i].y, samples[i].z, 
                                                             quad.faceDirection);
                    }
                    
                    // Average the corner samples for final AO
                    face.ao = (cornerAO[0] + cornerAO[1] + cornerAO[2] + cornerAO[3]) * 0.25f;
                }
            } else {
                // Fallback: no occlusion if ChunkManager is not available
                face.ao = glm::vec4(1.0f);
            }
        }
        
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
    int faceDirection
) {
    auto blockDef = Blocks::getBlock(blockType);
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

void BinaryMeshConverter::calculateQuadCornerSamples(
    const MeshQuad& quad,
    glm::ivec3 samples[4]
) {
    // Calculate the four corner positions of the quad for AO sampling
    // These represent the blocks that would be at the corners of the merged face
    
    glm::ivec3 basePos = quad.position;
    glm::ivec3 size = quad.size;
    
    switch (quad.faceDirection) {
        case 0: // Down face (XZ plane)
        case 1: // Up face (XZ plane)
            samples[0] = basePos; // Bottom-left
            samples[1] = basePos + glm::ivec3(size.x - 1, 0, 0); // Bottom-right
            samples[2] = basePos + glm::ivec3(0, 0, size.z - 1); // Top-left
            samples[3] = basePos + glm::ivec3(size.x - 1, 0, size.z - 1); // Top-right
            break;
            
        case 2: // North face (XY plane)
        case 3: // South face (XY plane)
            samples[0] = basePos; // Bottom-left
            samples[1] = basePos + glm::ivec3(size.x - 1, 0, 0); // Bottom-right
            samples[2] = basePos + glm::ivec3(0, size.y - 1, 0); // Top-left
            samples[3] = basePos + glm::ivec3(size.x - 1, size.y - 1, 0); // Top-right
            break;
            
        case 4: // West face (ZY plane)
        case 5: // East face (ZY plane)
            samples[0] = basePos; // Bottom-left
            samples[1] = basePos + glm::ivec3(0, 0, size.z - 1); // Bottom-right
            samples[2] = basePos + glm::ivec3(0, size.y - 1, 0); // Top-left
            samples[3] = basePos + glm::ivec3(0, size.y - 1, size.z - 1); // Top-right
            break;
            
        default:
            // Fallback: use the center position for all samples
            glm::ivec3 center = basePos + size / 2;
            for (int i = 0; i < 4; i++) {
                samples[i] = center;
            }
            break;
    }
}

// Hybrid Chunk Mesh Generator Implementation

std::optional<std::vector<HybridChunkMeshGenerator::FaceInstance>> HybridChunkMeshGenerator::generateOptimizedMesh(
    const Chunk& chunk,
    const glm::ivec3& chunkWorldPos,
    TextureArray& textureArray
) {
    // Create binary chunk data
    BinaryChunkData binaryData(chunk);
    
    // Check if all blocks can use binary meshing
    bool canUseFullBinaryMeshing = true;
    for (BlockType blockType : binaryData.getActiveBlockTypes()) {
        if (!canUseBinaryMeshing(blockType)) {
            canUseFullBinaryMeshing = false;
            break;
        }
    }
    
    // If any complex blocks are present, signal that traditional meshing should be used
    if (!canUseFullBinaryMeshing) {
        return std::nullopt;
    }
    
    // All blocks are simple - use binary greedy meshing for optimal performance
    std::vector<FaceInstance> allFaces;
    auto allQuads = BinaryGreedyMesher::generateAllQuads(binaryData);
    auto faces = BinaryMeshConverter::convertAllQuadsWithAO(allQuads, chunkWorldPos, chunk, textureArray);
    allFaces.insert(allFaces.end(), faces.begin(), faces.end());
    
    return allFaces;
}

std::optional<std::vector<HybridChunkMeshGenerator::FaceInstance>> HybridChunkMeshGenerator::generateOptimizedMeshWithAO(
    const Chunk& chunk,
    const glm::ivec3& chunkWorldPos,
    const ChunkManager* chunkManager,
    TextureArray& textureArray
) {
    // Create binary chunk data
    BinaryChunkData binaryData(chunk);
    
    // Check if all blocks can use binary meshing
    bool canUseFullBinaryMeshing = true;
    for (BlockType blockType : binaryData.getActiveBlockTypes()) {
        if (!canUseBinaryMeshing(blockType)) {
            canUseFullBinaryMeshing = false;
            break;
        }
    }
    
    // If any complex blocks are present, signal that traditional meshing should be used
    if (!canUseFullBinaryMeshing) {
        return std::nullopt;
    }
    
    // All blocks are simple - use binary greedy meshing with cross-chunk AO
    std::vector<FaceInstance> allFaces;
    auto allQuads = BinaryGreedyMesher::generateAllQuads(binaryData);
    auto faces = BinaryMeshConverter::convertAllQuadsWithAO(allQuads, chunkWorldPos, chunkManager, textureArray);
    allFaces.insert(allFaces.end(), faces.begin(), faces.end());
    
    return allFaces;
}

bool HybridChunkMeshGenerator::canUseBinaryMeshing(
    BlockType blockType
) {
    auto blockDef = Blocks::getBlock(blockType);
    if (!blockDef) {
        return false;
    }
    
    // Skip air blocks
    if (blockDef->getId() == "air") {
        return false;
    }
    
    // Load the block's model to check if it's a full cube
    std::string modelPath = "assets/zerith/models/block/" + blockDef->getModelName() + ".json";
    
    try {
        // Parse the model to check its geometry
        auto model = BlockbenchParser::parseFromFileWithParents(modelPath, nullptr);
        
        // Check if the model represents a full cube (from 0,0,0 to 16,16,16)
        return isFullCubeModel(model);
    } catch (const std::exception& e) {
        // If we can't load the model, assume it's complex and use traditional meshing
        return false;
    }
}

std::vector<HybridChunkMeshGenerator::FaceInstance> HybridChunkMeshGenerator::generateComplexBlockMesh(
    const Chunk& chunk,
    const glm::ivec3& chunkWorldPos,
    const Blocks& blocks,
    const std::vector<BlockType>& complexBlockTypes
) {
    std::vector<FaceInstance> faces;
    
    // Create a set for quick lookup
    std::set<BlockType> complexSet(complexBlockTypes.begin(), complexBlockTypes.end());
    
    // Iterate through all blocks in the chunk and generate faces for complex blocks
    // using the traditional per-block approach
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                BlockType blockType = chunk.getBlock(x, y, z);
                
                // Skip if not a complex block
                if (complexSet.find(blockType) == complexSet.end()) {
                    continue;
                }
                
                // Skip air blocks
                auto blockDef = Blocks::getBlock(blockType);
                if (!blockDef || blockDef->getId() == "air") {
                    continue;
                }
                
                // For complex blocks, we need to load their actual model and generate
                // proper geometry. For now, we'll return empty to make them invisible
                // rather than render incorrect geometry.
                // The proper solution is to disable binary meshing entirely and
                // fall back to traditional meshing for the whole chunk when complex blocks are present.
            }
        }
    }
    
    return faces;
}

bool HybridChunkMeshGenerator::isFullCubeModel(const BlockbenchModel::Model& model) {
    // A full cube model should have exactly one element that spans from [0,0,0] to [16,16,16]
    // Or multiple elements that together form a complete cube without gaps or overlaps
    
    if (model.elements.empty()) {
        return false;
    }
    
    // For simplicity, check if there's a single element that covers the full cube
    for (const auto& element : model.elements) {
        // Check if this element spans the full cube
        if (element.from[0] == 0.0f && element.from[1] == 0.0f && element.from[2] == 0.0f &&
            element.to[0] == 16.0f && element.to[1] == 16.0f && element.to[2] == 16.0f) {
            return true;
        }
    }
    
    // If no single element covers the full cube, check if all elements together
    // form a complete cube (this would be more complex to implement perfectly,
    // so for now we'll be conservative and only allow single-element full cubes)
    return false;
}

} // namespace Zerith