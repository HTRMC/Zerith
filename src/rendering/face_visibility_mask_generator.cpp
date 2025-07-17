#include "face_visibility_mask_generator.h"
#include "logger.h"

namespace Zerith {

FaceVisibilityMask FaceVisibilityMaskGenerator::generateMask(const ExtendedChunkData& extendedData) {
    FaceVisibilityMask mask;
    
    // Generate mask for each face direction
    for (int faceDir = 0; faceDir < 6; ++faceDir) {
        auto direction = static_cast<FaceVisibilityMask::FaceDirection>(faceDir);
        
        // Check each block position in the chunk
        for (int z = 0; z < FaceVisibilityMask::CHUNK_SIZE; ++z) {
            for (int y = 0; y < FaceVisibilityMask::CHUNK_SIZE; ++y) {
                for (int x = 0; x < FaceVisibilityMask::CHUNK_SIZE; ++x) {
                    bool visible = isFaceVisibleInternal(extendedData, x, y, z, direction);
                    mask.setFaceVisible(x, y, z, direction, visible);
                }
            }
        }
    }
    
    LOG_TRACE("Generated face visibility mask with %zu visible faces", mask.getTotalVisibleFaces());
    return mask;
}

bool FaceVisibilityMaskGenerator::isFaceVisibleInternal(const ExtendedChunkData& extendedData,
                                                       int x, int y, int z,
                                                       FaceVisibilityMask::FaceDirection direction) {
    // Convert local chunk coordinates to extended data coordinates (add 1 for border)
    int extX = x + 1;
    int extY = y + 1;
    int extZ = z + 1;
    
    // Get current block
    BlockType currentBlock = extendedData.getBlock(extX, extY, extZ);
    
    // If current block is air, face is never visible
    if (currentBlock == Blocks::AIR) {
        return false;
    }
    
    // Get neighbor block using direction offset
    glm::ivec3 offset = getDirectionOffset(direction);
    int neighborX = extX + offset.x;
    int neighborY = extY + offset.y;
    int neighborZ = extZ + offset.z;
    
    BlockType adjacentBlock = extendedData.getBlock(neighborX, neighborY, neighborZ);
    
    // If adjacent block is air, face is always visible
    if (adjacentBlock == Blocks::AIR) {
        return true;
    }
    
    // Get properties for both blocks (using existing cached system)
    const auto& currentProps = BlockProperties::getCullingProperties(currentBlock);
    const auto& adjacentProps = BlockProperties::getCullingProperties(adjacentBlock);
    
    // HACK: Never let stairs cull anything (preserve existing behavior)
    if (adjacentBlock == Blocks::OAK_STAIRS) {
        return true;
    }
    
    // Special handling for transparent blocks
    if (currentProps.isTransparent) {
        // If both blocks are the same type (e.g., glass-to-glass, water-to-water), cull the face
        if (currentBlock == adjacentBlock) {
            return false;
        }
        
        // Special handling for liquids (water)
        if (currentBlock == Blocks::WATER) {
            // Get adjacent block's face index that we're checking against
            int adjacentFaceIndex = getOppositeFaceIndex(direction);
            
            // If adjacent block is opaque and has full face culling, don't render water face
            if (!adjacentProps.isTransparent && adjacentFaceIndex >= 0 && 
                adjacentProps.faceCulling[adjacentFaceIndex] == CullFace::FULL) {
                return false;
            }
        }
        
        // Other transparent blocks (glass, leaves) should always show their faces
        return true;
    }
    
    // If adjacent block is transparent, always render the face
    if (adjacentProps.isTransparent) {
        return true;
    }
    
    // Check if the adjacent block's face can cull our face
    int adjacentFaceIndex = getOppositeFaceIndex(direction);
    
    // Check if adjacent block can cull our face and current block allows culling
    if (adjacentFaceIndex >= 0 && 
        adjacentProps.faceCulling[adjacentFaceIndex] == CullFace::FULL && 
        currentProps.canBeCulled) {
        
        // Additional check: don't cull stairs faces even if they can normally be culled
        if (currentBlock == Blocks::OAK_STAIRS) {
            return true; // Stairs faces are always visible
        }
        return false; // Face is culled
    }
    
    return true; // Face is visible
}

glm::ivec3 FaceVisibilityMaskGenerator::getDirectionOffset(FaceVisibilityMask::FaceDirection direction) {
    switch (direction) {
        case FaceVisibilityMask::FaceDirection::DOWN: return glm::ivec3(0, -1, 0);
        case FaceVisibilityMask::FaceDirection::UP: return glm::ivec3(0, 1, 0);
        case FaceVisibilityMask::FaceDirection::NORTH: return glm::ivec3(0, 0, -1);
        case FaceVisibilityMask::FaceDirection::SOUTH: return glm::ivec3(0, 0, 1);
        case FaceVisibilityMask::FaceDirection::WEST: return glm::ivec3(-1, 0, 0);
        case FaceVisibilityMask::FaceDirection::EAST: return glm::ivec3(1, 0, 0);
    }
    return glm::ivec3(0, 0, 0);
}

int FaceVisibilityMaskGenerator::getOppositeFaceIndex(FaceVisibilityMask::FaceDirection direction) {
    // When checking culling, we need the opposite face of the adjacent block
    // Face indices: 0=down, 1=up, 2=north, 3=south, 4=west, 5=east
    switch (direction) {
        case FaceVisibilityMask::FaceDirection::DOWN: return 1; // Adjacent block's up face
        case FaceVisibilityMask::FaceDirection::UP: return 0; // Adjacent block's down face
        case FaceVisibilityMask::FaceDirection::NORTH: return 3; // Adjacent block's south face
        case FaceVisibilityMask::FaceDirection::SOUTH: return 2; // Adjacent block's north face
        case FaceVisibilityMask::FaceDirection::WEST: return 5; // Adjacent block's east face
        case FaceVisibilityMask::FaceDirection::EAST: return 4; // Adjacent block's west face
    }
    return -1;
}

} // namespace Zerith