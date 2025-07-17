#include "binary_mesh_converter.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <set>

#include "blockbench_parser.h"
#include "voxel_ao.h"

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
    
    // Debug: Log world position for stairs
    if (blockDef->getId().find("stairs") != std::string::npos) {
        LOG_INFO("Stairs world position: ElementIdx=%d, WorldPos=[%.2f,%.2f,%.2f], FaceDir=%d", 
                 quad.elementIndex, worldPos.x, worldPos.y, worldPos.z, quad.faceDirection);
    }
    
    // Calculate scale based on quad dimensions
    glm::vec3 scale = calculateQuadScale(quad);
    
    // Get rotation for this face direction
    glm::vec4 rotation = getFaceRotation(quad.faceDirection);
    
    // Get texture information
    std::string textureName = getBlockTexture(quad.blockType, quad.faceDirection);
    
    // Debug: Log texture and face bounds information for slabs and stairs
    if (blockDef->getId().find("slab") != std::string::npos || blockDef->getId().find("stairs") != std::string::npos) {
        LOG_INFO("Enhanced meshing: Block ID='%s', BlockType=%d, Texture='%s', FaceDir=%d, Bounds=[%.2f,%.2f,%.2f,%.2f], Pos=[%d,%d,%d], Size=[%d,%d,%d], ElementIdx=%d, ElementOffset=[%.2f,%.2f,%.2f]", 
                 blockDef->getId().c_str(), quad.blockType, textureName.c_str(), 
                 quad.faceDirection, quad.faceBounds.min.x, quad.faceBounds.min.y, 
                 quad.faceBounds.max.x, quad.faceBounds.max.y,
                 quad.position.x, quad.position.y, quad.position.z,
                 quad.size.x, quad.size.y, quad.size.z,
                 quad.elementIndex, quad.elementOffset.x, quad.elementOffset.y, quad.elementOffset.z);
    }
    
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
        renderLayer         // render layer
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

glm::vec4 BinaryMeshConverter::calculateQuadUV(
    const MeshQuad& quad,
    const BlockDefinition& blockDef,
    int faceDirection
) {
    // Use face bounds to calculate proper UV coordinates for partial blocks
    const FaceBounds& bounds = quad.faceBounds;
    
    // Convert face bounds (0-1) to texture coordinates (0-16)
    glm::vec4 baseUV = glm::vec4(
        bounds.min.x * 16.0f,
        bounds.min.y * 16.0f,
        bounds.max.x * 16.0f,
        bounds.max.y * 16.0f
    );
    
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
    
    // Handle slabs and stairs - they use the same texture as their base material
    if (blockId == "oak_slab" || blockId == "oak_stairs") {
        return "oak_planks";
    }
    
    // Handle other stair types
    if (blockId.find("stairs") != std::string::npos) {
        // Extract the base material from the stairs name
        std::string baseMaterial = blockId.substr(0, blockId.find("_stairs"));
        return baseMaterial + "_planks";
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
    
    // For multi-element blocks, add the element offset
    if (quad.elementIndex >= 0) {
        basePos += quad.elementOffset;
    }
    
    // Apply face bounds offset for proper partial block positioning
    const FaceBounds& bounds = quad.faceBounds;
    
    // Apply face-specific corner offsets accounting for face bounds
    // For multi-element blocks, we need to be careful about how we apply bounds
    // The element offset positions the element, bounds position within the element
    
    switch (quad.faceDirection) {
        case 0: // Down face - position at back corner (from.x, from.y, to.z)
            if (quad.elementIndex >= 0) {
                // Multi-element: element offset handles block-relative position
                return basePos + glm::vec3(0.0f, 0.0f, static_cast<float>(quad.size.z));
            } else {
                // Single element: use bounds for positioning
                return basePos + glm::vec3(bounds.min.x, 0.0f, bounds.min.y + static_cast<float>(quad.size.z));
            }
        case 1: // Up face - position at front-top corner (from.x, to.y, from.z)
            // For Up face, get the actual block height from the block definition
            // The face bounds for Up face represent XZ plane, not Y height
            // We need to get the actual model height for proper slab positioning
            {
                float blockHeight;
                if (quad.elementIndex >= 0) {
                    // For multi-element blocks, height is already included in the element offset
                    // The face bounds Y represents the actual face height within the element
                    blockHeight = 0.0f; // Height is handled by element offset
                } else {
                    const auto& registry = BlockFaceBoundsRegistry::getInstance();
                    const auto& blockBounds = registry.getFaceBounds(quad.blockType);
                    // For side faces, bounds.max.y represents the actual height
                    blockHeight = blockBounds.faces[2].max.y; // Use North face bounds for height
                }
                if (quad.elementIndex >= 0) {
                    // Multi-element: Y position comes from element offset + element height
                    // For Up face, we need to position at the top of the element
                    // The element offset positions the bottom, so add the element's Y size
                    return basePos + glm::vec3(0.0f, quad.elementSize.y, 0.0f);
                } else {
                    // Single element: use calculated height
                    return basePos + glm::vec3(bounds.min.x, blockHeight, bounds.min.y);
                }
            }
        case 2: // North face - no offset needed (from.x, from.y, from.z)
            if (quad.elementIndex >= 0) {
                return basePos;
            } else {
                return basePos + glm::vec3(bounds.min.x, bounds.min.y, 0.0f);
            }
        case 3: // South face - position at back-bottom corner (to.x, from.y, to.z)
            if (quad.elementIndex >= 0) {
                // For South face, we're at the +Z side of the element
                // Position at element end X, element end Z
                return basePos + glm::vec3(quad.elementSize.x, 0.0f, quad.elementSize.z);
            } else {
                return basePos + glm::vec3(
                    bounds.min.x + static_cast<float>(quad.size.x),
                    bounds.min.y,
                    static_cast<float>(quad.size.z)
                );
            }
        case 4: // West face - position at back corner (from.x, from.y, to.z)
            // For West/East faces, bounds are in ZY plane
            if (quad.elementIndex >= 0) {
                return basePos + glm::vec3(0.0f, 0.0f, static_cast<float>(quad.size.z));
            } else {
                return basePos + glm::vec3(0.0f, bounds.min.y, bounds.min.x + static_cast<float>(quad.size.z));
            }
        case 5: // East face - position at front corner (to.x, from.y, from.z)
            // For West/East faces, bounds are in ZY plane
            if (quad.elementIndex >= 0) {
                // For East face, we're at the +X side of the element
                // X position is elementSize.x, not quad.size.x (which is merged size)
                return basePos + glm::vec3(quad.elementSize.x, 0.0f, 0.0f);
            } else {
                return basePos + glm::vec3(static_cast<float>(quad.size.x), bounds.min.y, bounds.min.x);
            }
        default:
            if (quad.elementIndex >= 0) {
                return basePos;
            } else {
                return basePos + glm::vec3(bounds.min.x, bounds.min.y, 0.0f);
            }
    }
}

glm::vec3 BinaryMeshConverter::calculateQuadScale(const MeshQuad& quad) {
    // Calculate the base scale using face bounds for proper partial block rendering
    const FaceBounds& bounds = quad.faceBounds;
    float faceWidth = bounds.max.x - bounds.min.x;
    float faceHeight = bounds.max.y - bounds.min.y;
    
    // Map quad dimensions to correct axes based on face direction
    // Use face bounds for the face dimensions and quad.size for the merged extent
    switch (quad.faceDirection) {
        case 0: // Down face (XZ plane)
        case 1: // Up face (XZ plane)
            return glm::vec3(
                static_cast<float>(quad.size.x) * faceWidth,
                static_cast<float>(quad.size.z) * faceHeight,
                1.0f
            );
        case 2: // North face (XY plane)
        case 3: // South face (XY plane)
            return glm::vec3(
                static_cast<float>(quad.size.x) * faceWidth,
                static_cast<float>(quad.size.y) * faceHeight,
                1.0f
            );
        case 4: // West face (ZY plane)
        case 5: // East face (ZY plane)
            return glm::vec3(
                static_cast<float>(quad.size.z) * faceWidth,
                static_cast<float>(quad.size.y) * faceHeight,
                1.0f
            );
        default:
            return glm::vec3(
                static_cast<float>(quad.size.x) * faceWidth,
                static_cast<float>(quad.size.y) * faceHeight,
                static_cast<float>(quad.size.z)
            );
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

std::optional<std::vector<HybridChunkMeshGenerator::FaceInstance>> HybridChunkMeshGenerator::generateOptimizedMesh(
    const Chunk& chunk,
    const glm::ivec3& chunkWorldPos,
    TextureArray& textureArray
) {
    // Create binary chunk data
    BinaryChunkData binaryData(chunk);
    
    // Categorize blocks by their meshing requirements
    std::vector<BlockType> fullCubeBlocks;
    std::vector<BlockType> partialBlocks;
    bool hasComplexBlocks = false;
    
    for (BlockType blockType : binaryData.getActiveBlockTypes()) {
        auto blockDef = Blocks::getBlock(blockType);
        if (!blockDef || blockDef->getId() == "air") {
            continue;
        }
        
        // All blocks should be able to use enhanced greedy meshing
        
        try {
            std::string modelPath = "assets/zerith/models/block/" + blockDef->getModelName() + ".json";
            auto model = BlockbenchParser::parseFromFileWithParents(modelPath, nullptr);
            
            if (isFullCubeModel(model)) {
                fullCubeBlocks.push_back(blockType);
            } else if (canUseEnhancedBinaryMeshing(blockType, model)) {
                partialBlocks.push_back(blockType);
            } else {
                hasComplexBlocks = true;
                break;
            }
        } catch (const std::exception& e) {
            hasComplexBlocks = true;
            break;
        }
    }
    
    // If any complex blocks are present, signal that traditional meshing should be used
    if (hasComplexBlocks) {
        return std::nullopt;
    }
    
    // Generate mesh using appropriate algorithm for each block type
    std::vector<FaceInstance> allFaces;
    
    // Use standard binary meshing for full cube blocks
    for (BlockType blockType : fullCubeBlocks) {
        for (int faceDir = 0; faceDir < 6; ++faceDir) {
            auto quads = BinaryGreedyMesher::generateQuads(binaryData, blockType, faceDir);
            auto faces = BinaryMeshConverter::convertAllQuadsWithAO(quads, chunkWorldPos, chunk, textureArray);
            allFaces.insert(allFaces.end(), faces.begin(), faces.end());
        }
    }
    
    // Use enhanced bounds-aware meshing for partial blocks
    for (BlockType blockType : partialBlocks) {
        auto blockDef = Blocks::getBlock(blockType);
        bool isStairs = blockDef && blockDef->getId().find("stairs") != std::string::npos;
        
        for (int faceDir = 0; faceDir < 6; ++faceDir) {
            std::vector<BinaryGreedyMesher::MeshQuad> quads;
            
            if (isStairs) {
                // Use multi-element meshing for stairs
                quads = BinaryGreedyMesher::generateQuadsMultiElement(binaryData, blockType, faceDir);
            } else {
                // Use regular bounds-aware meshing for simple partial blocks
                quads = BinaryGreedyMesher::generateQuadsWithBounds(binaryData, blockType, faceDir);
            }
            
            auto faces = BinaryMeshConverter::convertAllQuadsWithAO(quads, chunkWorldPos, chunk, textureArray);
            allFaces.insert(allFaces.end(), faces.begin(), faces.end());
        }
    }
    
    return allFaces;
}

std::optional<std::vector<HybridChunkMeshGenerator::FaceInstance>> HybridChunkMeshGenerator::generateOptimizedMeshWithNeighbors(
    const Chunk& chunk,
    const glm::ivec3& chunkWorldPos,
    TextureArray& textureArray,
    const Chunk* neighborXMinus,
    const Chunk* neighborXPlus,
    const Chunk* neighborYMinus,
    const Chunk* neighborYPlus,
    const Chunk* neighborZMinus,
    const Chunk* neighborZPlus
) {
    // Create binary chunk data
    BinaryChunkData binaryData(chunk);
    
    // Categorize blocks by their meshing requirements
    std::vector<BlockType> fullCubeBlocks;
    std::vector<BlockType> partialBlocks;
    bool hasComplexBlocks = false;
    
    for (BlockType blockType : binaryData.getActiveBlockTypes()) {
        auto blockDef = Blocks::getBlock(blockType);
        if (!blockDef || blockDef->getId() == "air") {
            continue;
        }
        
        // All blocks should be able to use enhanced greedy meshing
        
        try {
            std::string modelPath = "assets/zerith/models/block/" + blockDef->getModelName() + ".json";
            auto model = BlockbenchParser::parseFromFileWithParents(modelPath, nullptr);
            
            if (isFullCubeModel(model)) {
                fullCubeBlocks.push_back(blockType);
            } else if (canUseEnhancedBinaryMeshing(blockType, model)) {
                partialBlocks.push_back(blockType);
            } else {
                hasComplexBlocks = true;
                break;
            }
        } catch (const std::exception& e) {
            hasComplexBlocks = true;
            break;
        }
    }
    
    // If any complex blocks are present, signal that traditional meshing should be used
    if (hasComplexBlocks) {
        return std::nullopt;
    }
    
    // Create binary chunk data only for neighbors that can use binary meshing
    std::unique_ptr<BinaryChunkData> neighborXMinusData = nullptr;
    std::unique_ptr<BinaryChunkData> neighborXPlusData = nullptr;
    std::unique_ptr<BinaryChunkData> neighborYMinusData = nullptr;
    std::unique_ptr<BinaryChunkData> neighborYPlusData = nullptr;
    std::unique_ptr<BinaryChunkData> neighborZMinusData = nullptr;
    std::unique_ptr<BinaryChunkData> neighborZPlusData = nullptr;
    
    // Check if each neighbor can use binary meshing before creating binary data
    if (neighborXMinus && canNeighborUseBinaryMeshing(*neighborXMinus)) {
        neighborXMinusData = std::make_unique<BinaryChunkData>(*neighborXMinus);
    }
    if (neighborXPlus && canNeighborUseBinaryMeshing(*neighborXPlus)) {
        neighborXPlusData = std::make_unique<BinaryChunkData>(*neighborXPlus);
    }
    if (neighborYMinus && canNeighborUseBinaryMeshing(*neighborYMinus)) {
        neighborYMinusData = std::make_unique<BinaryChunkData>(*neighborYMinus);
    }
    if (neighborYPlus && canNeighborUseBinaryMeshing(*neighborYPlus)) {
        neighborYPlusData = std::make_unique<BinaryChunkData>(*neighborYPlus);
    }
    if (neighborZMinus && canNeighborUseBinaryMeshing(*neighborZMinus)) {
        neighborZMinusData = std::make_unique<BinaryChunkData>(*neighborZMinus);
    }
    if (neighborZPlus && canNeighborUseBinaryMeshing(*neighborZPlus)) {
        neighborZPlusData = std::make_unique<BinaryChunkData>(*neighborZPlus);
    }
    
    // Generate mesh using appropriate algorithm for each block type
    std::vector<FaceInstance> allFaces;
    
    // Use standard binary meshing for full cube blocks
    for (BlockType blockType : fullCubeBlocks) {
        for (int faceDir = 0; faceDir < 6; ++faceDir) {
            auto quads = BinaryGreedyMesher::generateQuadsWithNeighbors(
                binaryData, blockType, faceDir,
                neighborXMinusData.get(), neighborXPlusData.get(),
                neighborYMinusData.get(), neighborYPlusData.get(),
                neighborZMinusData.get(), neighborZPlusData.get(),
                neighborXMinus, neighborXPlus, neighborYMinus, neighborYPlus,
                neighborZMinus, neighborZPlus
            );
            auto faces = BinaryMeshConverter::convertAllQuadsWithAO(quads, chunkWorldPos, chunk, textureArray);
            allFaces.insert(allFaces.end(), faces.begin(), faces.end());
        }
    }
    
    // Use enhanced bounds-aware meshing for partial blocks
    // Note: For now, partial blocks don't use neighbor data for cross-chunk face culling
    // This could be enhanced in the future if needed
    for (BlockType blockType : partialBlocks) {
        auto blockDef = Blocks::getBlock(blockType);
        bool isStairs = blockDef && blockDef->getId().find("stairs") != std::string::npos;
        
        for (int faceDir = 0; faceDir < 6; ++faceDir) {
            std::vector<BinaryGreedyMesher::MeshQuad> quads;
            
            if (isStairs) {
                // Use multi-element meshing for stairs
                quads = BinaryGreedyMesher::generateQuadsMultiElement(binaryData, blockType, faceDir);
            } else {
                // Use regular bounds-aware meshing for simple partial blocks
                quads = BinaryGreedyMesher::generateQuadsWithBounds(binaryData, blockType, faceDir);
            }
            
            auto faces = BinaryMeshConverter::convertAllQuadsWithAO(quads, chunkWorldPos, chunk, textureArray);
            allFaces.insert(allFaces.end(), faces.begin(), faces.end());
        }
    }
    
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
        if (isFullCubeModel(model)) {
            return true;
        }
        
        // Enhanced: Also allow partial blocks for bounds-aware greedy meshing
        // For now, we'll allow simple partial blocks like slabs and stairs
        return canUseEnhancedBinaryMeshing(blockType, model);
    } catch (const std::exception& e) {
        // If we can't load the model, assume it's complex and use traditional meshing
        return false;
    }
}

bool HybridChunkMeshGenerator::canNeighborUseBinaryMeshing(
    const Chunk& chunk
) {
    // Create temporary binary chunk data to get active block types
    BinaryChunkData binaryData(chunk);
    
    // Check if all blocks can use binary meshing
    for (BlockType blockType : binaryData.getActiveBlockTypes()) {
        if (!canUseBinaryMeshing(blockType)) {
            return false;
        }
    }
    
    return true;
}

bool HybridChunkMeshGenerator::canUseEnhancedBinaryMeshing(
    BlockType blockType,
    const BlockbenchModel::Model& model
) {
    // Get block definition
    auto blockDef = Blocks::getBlock(blockType);
    if (!blockDef) {
        return false;
    }
    
    // For now, enable enhanced binary meshing for specific known simple partial blocks
    std::string blockId = blockDef->getId();
    
    // Allow all blocks including translucent ones like water
    
    // Allow simple partial blocks like slabs
    if (blockId.find("slab") != std::string::npos) {
        return true;
    }
    
    // Enable stairs to use multi-element greedy meshing
    if (blockId.find("stairs") != std::string::npos) {
        return true;
    }
    
    // Allow water blocks
    if (blockId == "water") {
        return true;
    }
    
    // Check if the model has reasonable complexity (not too many elements)
    if (model.elements.size() > 5) {
        return false; // Too complex for enhanced meshing
    }
    
    // Check if all elements are axis-aligned (no rotations)
    // Note: The current BlockbenchModel::Element doesn't support rotation yet,
    // so we can assume all elements are axis-aligned for now
    
    // If we get here, it's a simple partial block that can use enhanced binary meshing
    return true;
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