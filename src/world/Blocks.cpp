#include "Blocks.hpp"
#include "Block.hpp"
#include "Logger.hpp"

uint16_t Blocks::registerBlock(BlockRegistry& registry, const std::string& name, BlockRenderLayer renderLayer) {
    // The ID corresponds to the position in the registration sequence
    static uint16_t id = 0;
    uint16_t currentId = id++;

    registry.registerBlock(currentId, name, renderLayer);
    LOG_DEBUG("Registered block: %s (ID: %d, Layer: %d)", name.c_str(), currentId, static_cast<int>(renderLayer));

    return currentId;
}

void Blocks::registerAllBlocks(BlockRegistry& registry) {
    // Register blocks in the desired order, IDs will be assigned sequentially
    AIR = registerBlock(registry, "air", BlockRenderLayer::LAYER_CUTOUT);
    STONE = registerBlock(registry, "stone", BlockRenderLayer::LAYER_OPAQUE);
    GRASS_BLOCK = registerBlock(registry, "grass_block", BlockRenderLayer::LAYER_OPAQUE);
    OAK_FENCE_POST = registerBlock(registry, "oak_fence_post", BlockRenderLayer::LAYER_CUTOUT);
    COBBLESTONE = registerBlock(registry, "cobblestone", BlockRenderLayer::LAYER_OPAQUE);
    GREEN_STAINED_GLASS = registerBlock(registry, "green_stained_glass", BlockRenderLayer::LAYER_TRANSLUCENT);
    OAK_LOG = registerBlock(registry, "oak_log", BlockRenderLayer::LAYER_OPAQUE);

    LOG_INFO("Initialized block registry with %d block types", registry.getBlockCount());
}