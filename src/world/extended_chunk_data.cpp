#include "extended_chunk_data.h"
#include "block_properties.h"
#include "blocks.h"
#include <algorithm>

namespace Zerith {

ExtendedChunkData::ExtendedChunkData(const std::array<BlockType, EXTENDED_VOLUME>& blockData, 
                                     const glm::ivec3& chunkPos)
    : m_blocks(blockData), m_chunkPosition(chunkPos) {
    // Data is already complete - nothing to do
}

BlockType ExtendedChunkData::getBlock(int x, int y, int z) const {
    if (!isInExtendedBounds(x, y, z)) {
        return Blocks::AIR;
    }
    return m_blocks[getExtendedIndex(x, y, z)];
}

bool ExtendedChunkData::isFaceVisible(int x, int y, int z, int dx, int dy, int dz) const {
    // Check if current block is not air
    BlockType currentBlock = getBlock(x, y, z);
    if (currentBlock == Blocks::AIR) {
        return false;
    }
    
    // Check adjacent block using extended data
    int nx = x + dx;
    int ny = y + dy;
    int nz = z + dz;
    
    BlockType adjacentBlock = getBlock(nx, ny, nz);
    
    // If adjacent block is air, face is always visible
    if (adjacentBlock == Blocks::AIR) {
        return true;
    }
    
    // Get properties for both blocks
    const auto& currentProps = BlockProperties::getCullingProperties(currentBlock);
    const auto& adjacentProps = BlockProperties::getCullingProperties(adjacentBlock);
    
    // HACK: Never let stairs cull anything
    if (adjacentBlock == Blocks::OAK_STAIRS) {
        return true;
    }
    
    // Special handling for transparent blocks
    if (currentProps.isTransparent) {
        // If both blocks are the same type (e.g., glass-to-glass, water-to-water), cull the face
        if (currentBlock == adjacentBlock) {
            // Debug logging for water blocks
            if (currentBlock == Blocks::WATER) {
                // Temporarily enable debug logging to see if this code path is hit
                // LOG_DEBUG("EXTENDED: Water face culled: current=%d, adjacent=%d at (%d,%d,%d)->(%d,%d,%d)", 
                //          currentBlock, adjacentBlock, x, y, z, nx, ny, nz);
            }
            return false;
        }
        
        // Debug logging for water blocks that aren't being culled
        if (currentBlock == Blocks::WATER) {
            // LOG_DEBUG("EXTENDED: Water face NOT culled: current=%d, adjacent=%d at (%d,%d,%d)->(%d,%d,%d)", 
            //          currentBlock, adjacentBlock, x, y, z, nx, ny, nz);
        }
        
        // Special handling for liquids (water)
        if (currentBlock == Blocks::WATER) {
            // Determine which face of the adjacent block we're against
            int adjacentFaceIndex = -1;
            if (dy == -1) adjacentFaceIndex = 1; // Adjacent block's up face
            else if (dy == 1) adjacentFaceIndex = 0; // Adjacent block's down face
            else if (dz == -1) adjacentFaceIndex = 3; // Adjacent block's south face
            else if (dz == 1) adjacentFaceIndex = 2; // Adjacent block's north face
            else if (dx == -1) adjacentFaceIndex = 5; // Adjacent block's east face
            else if (dx == 1) adjacentFaceIndex = 4; // Adjacent block's west face
            
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
    
    // For solid blocks, use face culling logic
    // Determine which face we're checking based on direction
    int currentFaceIndex = -1;
    int adjacentFaceIndex = -1;
    
    if (dy == -1) {
        currentFaceIndex = 0;      // Current block's down face
        adjacentFaceIndex = 1;     // Adjacent block's up face
    } else if (dy == 1) {
        currentFaceIndex = 1;      // Current block's up face
        adjacentFaceIndex = 0;     // Adjacent block's down face
    } else if (dz == -1) {
        currentFaceIndex = 2;      // Current block's north face
        adjacentFaceIndex = 3;     // Adjacent block's south face
    } else if (dz == 1) {
        currentFaceIndex = 3;      // Current block's south face
        adjacentFaceIndex = 2;     // Adjacent block's north face
    } else if (dx == -1) {
        currentFaceIndex = 4;      // Current block's west face
        adjacentFaceIndex = 5;     // Adjacent block's east face
    } else if (dx == 1) {
        currentFaceIndex = 5;      // Current block's east face
        adjacentFaceIndex = 4;     // Adjacent block's west face
    }
    
    // Use face culling logic
    if (adjacentFaceIndex >= 0 && adjacentProps.faceCulling[adjacentFaceIndex] == CullFace::FULL && currentProps.canBeCulled) {
        // Additional check: don't cull stairs faces even if they can normally be culled
        if (currentBlock == Blocks::OAK_STAIRS) {
            return true; // Stairs faces are always visible
        }
        return false; // Face is culled
    }
    
    return true; // Face is visible
}

bool ExtendedChunkData::isFaceVisibleByDirection(int x, int y, int z, int faceDir) const {
    // Convert face direction to dx, dy, dz
    int dx = 0, dy = 0, dz = 0;
    switch (faceDir) {
        case 0: dy = -1; break; // Down
        case 1: dy = 1; break;  // Up
        case 2: dz = -1; break; // North
        case 3: dz = 1; break;  // South
        case 4: dx = -1; break; // West
        case 5: dx = 1; break;  // East
    }
    
    return isFaceVisible(x, y, z, dx, dy, dz);
}

} // namespace Zerith