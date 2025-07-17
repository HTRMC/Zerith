#pragma once

#include <bitset>
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

namespace Zerith {

/**
 * Pre-computed face visibility mask for fast face culling during mesh generation.
 * Uses bit masks to store visibility state for all faces in a chunk, eliminating
 * the need for repeated property lookups and conditional checks.
 */
class FaceVisibilityMask {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
    
    // Face direction constants
    enum class FaceDirection : int {
        DOWN = 0,   // (0, -1, 0)
        UP = 1,     // (0, 1, 0)  
        NORTH = 2,  // (0, 0, -1)
        SOUTH = 3,  // (0, 0, 1)
        WEST = 4,   // (-1, 0, 0)
        EAST = 5    // (1, 0, 0)
    };
    
    // One bit mask per face direction
    using FaceMask = std::bitset<CHUNK_VOLUME>;
    
    FaceVisibilityMask() = default;
    
    /**
     * Check if a face is visible using pre-computed mask.
     * This is the fast O(1) replacement for isFaceVisible().
     * 
     * @param x Block X coordinate (0-15)
     * @param y Block Y coordinate (0-15) 
     * @param z Block Z coordinate (0-15)
     * @param direction Face direction to check
     * @return true if face should be rendered, false if culled
     */
    bool isFaceVisible(int x, int y, int z, FaceDirection direction) const {
        int index = coordsToIndex(x, y, z);
        return m_visibleFaces[static_cast<int>(direction)][index];
    }
    
    /**
     * Check if a face is visible using direction vector.
     * Converts direction vector to FaceDirection enum.
     */
    bool isFaceVisible(int x, int y, int z, int dx, int dy, int dz) const {
        FaceDirection direction = directionVectorToFace(dx, dy, dz);
        return isFaceVisible(x, y, z, direction);
    }
    
    /**
     * Set face visibility state (used during mask generation).
     */
    void setFaceVisible(int x, int y, int z, FaceDirection direction, bool visible) {
        int index = coordsToIndex(x, y, z);
        m_visibleFaces[static_cast<int>(direction)][index] = visible;
    }
    
    /**
     * Get the complete mask for a face direction.
     * Useful for bitwise operations in binary meshing.
     */
    const FaceMask& getFaceMask(FaceDirection direction) const {
        return m_visibleFaces[static_cast<int>(direction)];
    }
    
    /**
     * Get total number of visible faces across all directions.
     * Useful for early-exit optimizations.
     */
    size_t getTotalVisibleFaces() const {
        size_t total = 0;
        for (int i = 0; i < 6; ++i) {
            total += m_visibleFaces[i].count();
        }
        return total;
    }
    
    /**
     * Clear all face visibility (set all faces to invisible).
     */
    void clear() {
        for (int i = 0; i < 6; ++i) {
            m_visibleFaces[i].reset();
        }
    }
    
    /**
     * Convert 3D coordinates to 1D bit index.
     * Uses same indexing as BinaryChunkData for consistency.
     */
    static constexpr int coordsToIndex(int x, int y, int z) {
        return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
    }
    
    /**
     * Convert 1D bit index to 3D coordinates.
     */
    static constexpr glm::ivec3 indexToCoords(int index) {
        int z = index / (CHUNK_SIZE * CHUNK_SIZE);
        int y = (index % (CHUNK_SIZE * CHUNK_SIZE)) / CHUNK_SIZE;
        int x = index % CHUNK_SIZE;
        return glm::ivec3(x, y, z);
    }
    
    /**
     * Convert direction vector to FaceDirection enum.
     */
    static FaceDirection directionVectorToFace(int dx, int dy, int dz) {
        if (dy == -1) return FaceDirection::DOWN;
        if (dy == 1) return FaceDirection::UP;
        if (dz == -1) return FaceDirection::NORTH;
        if (dz == 1) return FaceDirection::SOUTH;
        if (dx == -1) return FaceDirection::WEST;
        if (dx == 1) return FaceDirection::EAST;
        
        // Should never reach here with valid direction vectors
        return FaceDirection::DOWN;
    }
    
    /**
     * Convert FaceDirection enum to direction vector.
     */
    static glm::ivec3 faceToDirectionVector(FaceDirection direction) {
        switch (direction) {
            case FaceDirection::DOWN: return glm::ivec3(0, -1, 0);
            case FaceDirection::UP: return glm::ivec3(0, 1, 0);
            case FaceDirection::NORTH: return glm::ivec3(0, 0, -1);
            case FaceDirection::SOUTH: return glm::ivec3(0, 0, 1);
            case FaceDirection::WEST: return glm::ivec3(-1, 0, 0);
            case FaceDirection::EAST: return glm::ivec3(1, 0, 0);
        }
        return glm::ivec3(0, 0, 0);
    }

private:
    // Bit masks for each face direction
    // Index matches FaceDirection enum values
    std::array<FaceMask, 6> m_visibleFaces;
};

} // namespace Zerith