#include "block_properties.h"

namespace Zerith {

// Initialize block properties for each block type
const std::array<BlockCullingProperties, 11> BlockProperties::s_blockProperties = {{
    // AIR - doesn't cull anything
    { {CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE}, true, false },
    
    // OAK_PLANKS - full solid block
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, false, true },
    
    // OAK_SLAB - only bottom face can cull (it's at y=0), side faces don't cull because they're half-height
    // Note: The slab top face (at y=8) also doesn't cull because it's in the middle of the block
    { {CullFace::FULL, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE}, false, true },
    
    // OAK_STAIRS - complex shape, don't cull any faces for now
    // TODO: Implement proper culling based on stair orientation
    { {CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE}, false, true },
    
    // GRASS_BLOCK - full solid block
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, false, true },
    
    // STONE - full solid block
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, false, true },
    
    // DIRT - full solid block
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, false, true },
    
    // OAK_LOG - full solid block
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, false, true },
    
    // OAK_LEAVES - full solid block but with transparency
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, true, true },
    
    // CRAFTING_TABLE - full solid block
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, false, true },
    
    // GLASS - transparent block that culls other glass faces but not opaque faces
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, true, true }
}};

const BlockCullingProperties& BlockProperties::getCullingProperties(BlockType type) {
    return s_blockProperties[static_cast<uint8_t>(type)];
}

} // namespace Zerith