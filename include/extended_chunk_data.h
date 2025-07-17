#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include "chunk.h"

namespace Zerith {

/**
 * Extended chunk data that includes a 1-block border from neighboring chunks.
 * This provides 18x18x18 block data (16x16x16 chunk + 1-block border on each side)
 * for efficient face culling without chunk border artifacts.
 */
class ExtendedChunkData {
public:
    static constexpr int EXTENDED_SIZE = 18;
    static constexpr int EXTENDED_VOLUME = EXTENDED_SIZE * EXTENDED_SIZE * EXTENDED_SIZE;
    static constexpr int BORDER_SIZE = 1;

    /**
     * Construct extended chunk data from pre-sampled 18x18x18 block data.
     * This constructor takes the complete 18x18x18 data that should already
     * include the center chunk + 1-block border from all directions.
     * @param blockData The 18x18x18 block data array
     * @param chunkPos The position of the center chunk
     */
    ExtendedChunkData(const std::array<BlockType, EXTENDED_VOLUME>& blockData, 
                      const glm::ivec3& chunkPos);

    /**
     * Get block at extended coordinates.
     * Coordinates range from -1 to 16 (inclusive).
     * Center chunk data is at coordinates 0-15.
     * Border data is at coordinates -1 and 16.
     */
    BlockType getBlock(int x, int y, int z) const;

    /**
     * Check if a face should be rendered using the extended data.
     * This prevents chunk border artifacts by having access to neighboring blocks.
     * @param x, y, z Position in center chunk (0-15)
     * @param dx, dy, dz Face direction offset
     */
    bool isFaceVisible(int x, int y, int z, int dx, int dy, int dz) const;

    /**
     * Check if a face should be rendered using face direction enum.
     * @param x, y, z Position in center chunk (0-15)
     * @param faceDir Face direction (0=down, 1=up, 2=north, 3=south, 4=west, 5=east)
     */
    bool isFaceVisibleByDirection(int x, int y, int z, int faceDir) const;

private:
    /**
     * Convert extended coordinates to 1D array index.
     * Extended coordinates range from -1 to 16, but array indices are 0-17.
     */
    constexpr int getExtendedIndex(int x, int y, int z) const {
        int ex = x + BORDER_SIZE;  // Convert -1..16 to 0..17
        int ey = y + BORDER_SIZE;
        int ez = z + BORDER_SIZE;
        return ex + ey * EXTENDED_SIZE + ez * EXTENDED_SIZE * EXTENDED_SIZE;
    }

    /**
     * Check if extended coordinates are within bounds.
     */
    constexpr bool isInExtendedBounds(int x, int y, int z) const {
        return x >= -BORDER_SIZE && x <= Chunk::CHUNK_SIZE &&
               y >= -BORDER_SIZE && y <= Chunk::CHUNK_SIZE &&
               z >= -BORDER_SIZE && z <= Chunk::CHUNK_SIZE;
    }

    std::array<BlockType, EXTENDED_VOLUME> m_blocks;
    glm::ivec3 m_chunkPosition; // Position of the center chunk
};

} // namespace Zerith