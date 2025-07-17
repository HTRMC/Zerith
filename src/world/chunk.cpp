#include "chunk.h"
#include "logger.h"
#include "block_properties.h"
#include "blocks.h"
#include <algorithm>

namespace Zerith {

Chunk::Chunk(glm::ivec3 chunkPosition) 
    : m_chunkPosition(std::move(chunkPosition)) {
    // Initialize all blocks to air
    std::fill(m_blocks.begin(), m_blocks.end(), Blocks::AIR);
    std::fill(m_extendedBlocks.begin(), m_extendedBlocks.end(), Blocks::AIR);
    LOG_TRACE("Created chunk at position (%d, %d, %d)", m_chunkPosition.x, m_chunkPosition.y, m_chunkPosition.z);
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (!isInBounds(x, y, z)) {
        return Blocks::AIR;
    }
    return m_blocks[getIndex(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (isInBounds(x, y, z)) {
        m_blocks[getIndex(x, y, z)] = type;
    }
}

BlockType Chunk::getBlockWorld(const glm::vec3& worldPos) const {
    glm::ivec3 localPos = worldToLocal(worldPos);
    return getBlock(localPos.x, localPos.y, localPos.z);
}

glm::ivec3 Chunk::worldToLocal(const glm::vec3& worldPos) const {
    glm::vec3 chunkWorldPos = glm::vec3(m_chunkPosition) * static_cast<float>(CHUNK_SIZE);
    glm::vec3 localPos = worldPos - chunkWorldPos;
    return glm::ivec3(
        static_cast<int>(std::floor(localPos.x)),
        static_cast<int>(std::floor(localPos.y)),
        static_cast<int>(std::floor(localPos.z))
    );
}

bool Chunk::isFaceVisible(int x, int y, int z, int dx, int dy, int dz) const {
    // Check if current block is not air
    BlockType currentBlock = getBlock(x, y, z);
    if (currentBlock == Blocks::AIR) {
        return false;
    }
    
    // Check adjacent block
    int nx = x + dx;
    int ny = y + dy;
    int nz = z + dz;
    
    // If adjacent block is outside chunk bounds, assume it's visible
    if (!isInBounds(nx, ny, nz)) {
        return true;
    }
    
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
            return false;
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
    
    // Determine which face we're checking based on direction
    // dx,dy,dz -> face index mapping:
    // (0,-1,0) = 0 = down
    // (0,1,0)  = 1 = up
    // (0,0,-1) = 2 = north
    // (0,0,1)  = 3 = south
    // (-1,0,0) = 4 = west
    // (1,0,0)  = 5 = east
    int adjacentFaceIndex = -1;
    if (dy == -1) adjacentFaceIndex = 1; // Adjacent block's up face
    else if (dy == 1) adjacentFaceIndex = 0; // Adjacent block's down face
    else if (dz == -1) adjacentFaceIndex = 3; // Adjacent block's south face
    else if (dz == 1) adjacentFaceIndex = 2; // Adjacent block's north face
    else if (dx == -1) adjacentFaceIndex = 5; // Adjacent block's east face
    else if (dx == 1) adjacentFaceIndex = 4; // Adjacent block's west face
    
    // Check if the adjacent block's face can cull our face
    // But only if the current block allows itself to be culled
    if (adjacentFaceIndex >= 0 && adjacentProps.faceCulling[adjacentFaceIndex] == CullFace::FULL && currentProps.canBeCulled) {
        // Additional check: don't cull stairs faces even if they can normally be culled
        if (currentBlock == Blocks::OAK_STAIRS) {
            return true; // Stairs faces are always visible
        }
        return false; // Face is culled
    }
    
    return true; // Face is visible
}

bool Chunk::isFaceVisibleAdvanced(int x, int y, int z, int faceDir) const {
    // Convert face direction to dx,dy,dz
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

// Morton lookup tables for constexpr use
constexpr uint32_t morton_x[16] = {
    0x000000, 0x000001, 0x000008, 0x000009, 0x000040, 0x000041, 0x000048, 0x000049,
    0x000200, 0x000201, 0x000208, 0x000209, 0x000240, 0x000241, 0x000248, 0x000249
};

constexpr uint32_t morton_y[16] = {
    0x000000, 0x000002, 0x000010, 0x000012, 0x000080, 0x000082, 0x000090, 0x000092,
    0x000400, 0x000402, 0x000410, 0x000412, 0x000480, 0x000482, 0x000490, 0x000492
};

constexpr uint32_t morton_z[16] = {
    0x000000, 0x000004, 0x000020, 0x000024, 0x000100, 0x000104, 0x000120, 0x000124,
    0x000800, 0x000804, 0x000820, 0x000824, 0x000900, 0x000904, 0x000920, 0x000924
};

constexpr int Chunk::getIndex(int x, int y, int z) const {
    // Morton Z-order curve for better spatial locality in all dimensions
    // This approach interleaves bits of x, y, and z coordinates to create
    // a space-filling curve that preserves locality in all three dimensions
    
    // We assume CHUNK_SIZE is 16 (4 bits), so we need to interleave
    // 4 bits from each coordinate (x, y, z)
    
    // Limit to valid range
    x = x & 0xF;  // 0-15
    y = y & 0xF;  // 0-15
    z = z & 0xF;  // 0-15
    
    // Combine the lookup values
    return morton_x[x] | morton_y[y] | morton_z[z];
}

BlockType Chunk::getExtendedBlock(int x, int y, int z) const {
    if (!isInExtendedBounds(x, y, z)) {
        return Blocks::AIR;
    }
    return m_extendedBlocks[getExtendedIndex(x, y, z)];
}

void Chunk::setExtendedBlock(int x, int y, int z, BlockType type) {
    if (isInExtendedBounds(x, y, z)) {
        m_extendedBlocks[getExtendedIndex(x, y, z)] = type;
    }
}

constexpr int Chunk::getExtendedIndex(int x, int y, int z) const {
    // Convert -1..16 to 0..17 range
    int ex = x + 1;
    int ey = y + 1;
    int ez = z + 1;
    return ex + ey * EXTENDED_SIZE + ez * EXTENDED_SIZE * EXTENDED_SIZE;
}

constexpr bool Chunk::isInExtendedBounds(int x, int y, int z) const {
    return x >= -1 && x <= 16 &&
           y >= -1 && y <= 16 &&
           z >= -1 && z <= 16;
}

constexpr bool Chunk::isInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_SIZE &&
           y >= 0 && y < CHUNK_SIZE &&
           z >= 0 && z < CHUNK_SIZE;
}

} // namespace Zerith