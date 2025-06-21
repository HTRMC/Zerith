#include "block_properties.h"
#include "blocks.h"
#include "logger.h"

namespace Zerith {

// Dynamic block properties storage
std::vector<BlockCullingProperties> BlockProperties::s_blockProperties;

void BlockProperties::initialize() {
    // Initialize blocks first
    Blocks::initialize();
    
    size_t blockCount = Blocks::getBlockCount();
    
    // Resize to accommodate all blocks
    s_blockProperties.resize(blockCount);
    
    // Set properties from unified blocks system
    for (const auto& blockDef : Blocks::getAllBlocks()) {
        BlockType type = blockDef->getBlockType();
        s_blockProperties[type] = blockDef->getCullingProperties();
    }
    
    LOG_INFO("Initialized block properties for %zu blocks", blockCount);
}

const BlockCullingProperties& BlockProperties::getCullingProperties(BlockType type) {
    if (static_cast<size_t>(type) >= s_blockProperties.size()) {
        LOG_ERROR("Invalid block type: %d", static_cast<int>(type));
        // Return air properties as fallback
        return s_blockProperties[0];
    }
    return s_blockProperties[type];
}

} // namespace Zerith