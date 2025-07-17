#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace Zerith {

// BlockType is now just an index into the block registry
using BlockType = uint8_t;

class Chunk {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int EXTENDED_SIZE = 18;  // Store overlapping data for neighbor-free meshing
    static constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
    static constexpr int EXTENDED_VOLUME = EXTENDED_SIZE * EXTENDED_SIZE * EXTENDED_SIZE;

    Chunk(glm::ivec3 chunkPosition = glm::ivec3(0));
    
    // Move constructor and assignment operator
    Chunk(Chunk&& other) noexcept = default;
    Chunk& operator=(Chunk&& other) noexcept = default;
    
    // Disable copy constructor and assignment (chunks are heavy objects)
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    // Block access (standard 16x16x16 range: 0-15)
    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);
    
    // Extended block access (18x18x18 range: -1 to 16)
    BlockType getExtendedBlock(int x, int y, int z) const;
    void setExtendedBlock(int x, int y, int z, BlockType type);
    
    // Get block at world coordinates
    BlockType getBlockWorld(const glm::vec3& worldPos) const;
    
    // Convert world position to chunk-local position
    glm::ivec3 worldToLocal(const glm::vec3& worldPos) const;
    
    // Get chunk position in world
    constexpr glm::ivec3 getChunkPosition() const { return m_chunkPosition; }
    
    // Check if a face should be rendered
    bool isFaceVisible(int x, int y, int z, int dx, int dy, int dz) const;
    
    // Check if a face should be rendered considering block properties
    bool isFaceVisibleAdvanced(int x, int y, int z, int faceDir) const;

private:
    // Convert 3D coordinates to 1D array index
    constexpr int getIndex(int x, int y, int z) const;
    constexpr int getExtendedIndex(int x, int y, int z) const;
    
    // Check if coordinates are within chunk bounds
    constexpr bool isInBounds(int x, int y, int z) const;
    constexpr bool isInExtendedBounds(int x, int y, int z) const;

    std::array<BlockType, CHUNK_VOLUME> m_blocks;           // Standard 16x16x16 storage
    std::array<BlockType, EXTENDED_VOLUME> m_extendedBlocks; // Extended 18x18x18 storage
    glm::ivec3 m_chunkPosition; // Position of chunk in chunk coordinates
};

} // namespace Zerith