#include "block_properties.h"

namespace Zerith {

// Initialize block properties for each block type
const std::array<BlockCullingProperties, 11> BlockProperties::s_blockProperties = {{
    // AIR - doesn't cull anything
    { {CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE}, true, false },
    
    // OAK_PLANKS - full solid block
    { {CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL, CullFace::FULL}, false, true },
    
    // OAK_SLAB - only bottom face can cull (it's at y=0), other faces are partial
    { {CullFace::FULL, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE}, false, true },
    
    // OAK_STAIRS - complex shape, only certain faces can cull
    // Stairs have a bottom slab (0-8) and an upper portion (8-16) on one side
    // Only the bottom face and the back face (opposite the steps) can fully cull
    { {CullFace::FULL, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE}, false, true },
    
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
    
    // GLASS - transparent block that doesn't cull but can be culled
    { {CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE, CullFace::NONE}, true, true }
}};

const BlockCullingProperties& BlockProperties::getCullingProperties(BlockType type) {
    return s_blockProperties[static_cast<uint8_t>(type)];
}

} // namespace Zerith