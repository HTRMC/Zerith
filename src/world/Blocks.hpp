#pragma once

#include "Block.hpp"
#include <memory>

// Forward declarations
class BlockRegistry;

class Blocks {
public:
    // Initialize all blocks and register them with the provided registry
    static void registerAllBlocks(BlockRegistry& registry);

    // Block IDs - these will be automatically set during registration
    static inline uint16_t AIR;
    static inline uint16_t STONE;
    static inline uint16_t GRASS_BLOCK;
    static inline uint16_t OAK_FENCE_POST;
    static inline uint16_t COBBLESTONE;
    static inline uint16_t GREEN_STAINED_GLASS;
    static inline uint16_t OAK_LOG;

private:
    // Helper method to register a block
    static uint16_t registerBlock(BlockRegistry& registry, const std::string& name,
                                 BlockRenderLayer renderLayer = BlockRenderLayer::LAYER_OPAQUE);
};