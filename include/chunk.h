#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace Zerith {

enum class BlockType : uint8_t {
    AIR = 0,
    OAK_PLANKS = 1,
    OAK_SLAB = 2,
    OAK_STAIRS = 3,
    GRASS_BLOCK = 4,
    STONE = 5,
    DIRT = 6,
    OAK_LOG = 7,
    OAK_LEAVES = 8,
    CRAFTING_TABLE = 9,
    GLASS = 10,
    NUM_BLOCK_TYPES = 11  // Must be last
};

class Chunk {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

    Chunk(glm::ivec3 chunkPosition = glm::ivec3(0));
    
    // Move constructor and assignment operator
    Chunk(Chunk&& other) noexcept = default;
    Chunk& operator=(Chunk&& other) noexcept = default;
    
    // Disable copy constructor and assignment (chunks are heavy objects)
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    // Block access
    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);
    
    // Get block at world coordinates
    BlockType getBlockWorld(const glm::vec3& worldPos) const;
    
    // Convert world position to chunk-local position
    glm::ivec3 worldToLocal(const glm::vec3& worldPos) const;
    
    // Get chunk position in world
    glm::ivec3 getChunkPosition() const { return m_chunkPosition; }
    
    // Check if a face should be rendered
    bool isFaceVisible(int x, int y, int z, int dx, int dy, int dz) const;
    
    // Check if a face should be rendered considering block properties
    bool isFaceVisibleAdvanced(int x, int y, int z, int faceDir) const;

private:
    // Convert 3D coordinates to 1D array index
    int getIndex(int x, int y, int z) const;
    
    // Check if coordinates are within chunk bounds
    bool isInBounds(int x, int y, int z) const;

    std::array<BlockType, CHUNK_VOLUME> m_blocks;
    glm::ivec3 m_chunkPosition; // Position of chunk in chunk coordinates
};

} // namespace Zerith