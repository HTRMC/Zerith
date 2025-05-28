#include "chunk.h"
#include "logger.h"
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
    if (getBlock(x, y, z) == BlockType::AIR) {
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
    
    // Face is visible if adjacent block is air
    return getBlock(nx, ny, nz) == BlockType::AIR;
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