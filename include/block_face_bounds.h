#pragma once

#include <glm/glm.hpp>
#include <array>
#include "chunk.h"

namespace Zerith {

// Represents the 2D bounds of a face in block-local coordinates (0-1 range)
struct FaceBounds {
    glm::vec2 min;  // Minimum UV coordinates (0-1)
    glm::vec2 max;  // Maximum UV coordinates (0-1)
    
    FaceBounds() : min(0.0f), max(1.0f) {}
    FaceBounds(float minU, float minV, float maxU, float maxV) 
        : min(minU, minV), max(maxU, maxV) {}
    
    // Check if this face bounds overlaps with another
    bool overlaps(const FaceBounds& other) const {
        return !(max.x <= other.min.x || min.x >= other.max.x ||
                 max.y <= other.min.y || min.y >= other.max.y);
    }
    
    // Check if this face fully covers another
    bool covers(const FaceBounds& other) const {
        return min.x <= other.min.x && max.x >= other.max.x &&
               min.y <= other.min.y && max.y >= other.max.y;
    }
    
    // Get the area of the face (0-1)
    float area() const {
        return (max.x - min.x) * (max.y - min.y);
    }
    
    // Check if this is a full face (covers the entire 1x1 area)
    bool isFull() const {
        const float epsilon = 0.001f;
        return std::abs(min.x) < epsilon && std::abs(min.y) < epsilon &&
               std::abs(max.x - 1.0f) < epsilon && std::abs(max.y - 1.0f) < epsilon;
    }
};

// Structure to hold face bounds for all 6 faces of a block
struct BlockFaceBounds {
    std::array<FaceBounds, 6> faces;  // down, up, north, south, west, east
    
    BlockFaceBounds() {
        // Default to full faces
        for (auto& face : faces) {
            face = FaceBounds(0.0f, 0.0f, 1.0f, 1.0f);
        }
    }
};

// Singleton class to manage face bounds for all block types
class BlockFaceBoundsRegistry {
public:
    static BlockFaceBoundsRegistry& getInstance() {
        static BlockFaceBoundsRegistry instance;
        return instance;
    }
    
    // Get face bounds for a block type
    const BlockFaceBounds& getFaceBounds(BlockType type) const {
        size_t index = static_cast<size_t>(type);
        if (index < m_faceBounds.size()) {
            return m_faceBounds[index];
        }
        return m_defaultBounds;
    }
    
    // Set face bounds for a block type
    void setFaceBounds(BlockType type, const BlockFaceBounds& bounds) {
        size_t index = static_cast<size_t>(type);
        if (index < m_faceBounds.size()) {
            m_faceBounds[index] = bounds;
        }
    }
    
    // Check if two adjacent faces should be culled based on their bounds
    bool shouldCullFaces(BlockType currentBlock, int currentFace,
                        BlockType adjacentBlock, int adjacentFace) const {
        const auto& currentBounds = getFaceBounds(currentBlock);
        const auto& adjacentBounds = getFaceBounds(adjacentBlock);
        
        // If the adjacent face fully covers the current face, cull it
        return adjacentBounds.faces[adjacentFace].covers(currentBounds.faces[currentFace]);
    }

private:
    BlockFaceBoundsRegistry() {
        // Initialize with default bounds (full faces for all blocks)
        m_faceBounds.resize(static_cast<size_t>(BlockType::NUM_BLOCK_TYPES));
        
        // Set up specific bounds for partial blocks
        initializeBlockBounds();
    }
    
    void initializeBlockBounds();
    
    std::vector<BlockFaceBounds> m_faceBounds;
    BlockFaceBounds m_defaultBounds;  // Default full bounds
};

} // namespace Zerith