#include "chunk.h"
#include "logger.h"
#include "block_properties.h"
#include <algorithm>

namespace Zerith {

Chunk::Chunk(glm::ivec3 chunkPosition) 
    : m_chunkPosition(std::move(chunkPosition)) {
    // Initialize all blocks to air
    std::fill(m_blocks.begin(), m_blocks.end(), BlockType::AIR);
    LOG_TRACE("Created chunk at position (%d, %d, %d)", m_chunkPosition.x, m_chunkPosition.y, m_chunkPosition.z);
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (!isInBounds(x, y, z)) {
        return BlockType::AIR;
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
    if (currentBlock == BlockType::AIR) {
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
    if (adjacentBlock == BlockType::AIR) {
        return true;
    }
    
    // Get properties for both blocks
    const auto& currentProps = BlockProperties::getCullingProperties(currentBlock);
    const auto& adjacentProps = BlockProperties::getCullingProperties(adjacentBlock);
    
    // Special handling for transparent blocks
    if (currentProps.isTransparent) {
        // If both blocks are the same type (e.g., glass-to-glass), cull the face
        if (currentBlock == adjacentBlock) {
            return false;
        }
        // Glass should always show its faces except when adjacent to same type
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
    if (adjacentFaceIndex >= 0 && adjacentProps.faceCulling[adjacentFaceIndex] == CullFace::FULL) {
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

int Chunk::getIndex(int x, int y, int z) const {
    return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
}

bool Chunk::isInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_SIZE &&
           y >= 0 && y < CHUNK_SIZE &&
           z >= 0 && z < CHUNK_SIZE;
}

} // namespace Zerith