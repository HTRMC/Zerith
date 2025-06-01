#pragma once

#include <cstdint>
#include <array>
#include "chunk.h"

namespace Zerith {

// Enum to represent which faces of a block can cull adjacent faces
enum class CullFace : uint8_t {
    NONE = 0,      // Face doesn't cull anything (e.g., slab top face)
    FULL = 1,      // Face culls full adjacent face (e.g., solid block)
    PARTIAL = 2    // Face partially culls (not used yet, for future)
};

// Structure to hold block culling properties
struct BlockCullingProperties {
    // For each face direction (down, up, north, south, west, east)
    // indicates if this block's face can cull an adjacent block's opposite face
    std::array<CullFace, 6> faceCulling;
    
    // Whether this block is transparent (like glass)
    bool isTransparent;
    
    // Whether this block should cull faces against other blocks
    bool canBeCulled;
};

// Block properties lookup table
class BlockProperties {
public:
    static const BlockCullingProperties& getCullingProperties(BlockType type);
    
private:
    static const std::array<BlockCullingProperties, 11> s_blockProperties;
};

} // namespace Zerith