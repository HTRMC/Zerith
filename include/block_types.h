#pragma once

#include "chunk.h"

namespace Zerith {

// Convenience constants for commonly used block types
// These will be initialized after block registry is loaded
struct BlockTypes {
    static BlockType AIR;
    static BlockType STONE; 
    static BlockType DIRT;
    static BlockType GRASS_BLOCK;
    static BlockType COBBLESTONE;
    static BlockType SAND;
    static BlockType GRAVEL;
    static BlockType OAK_LOG;
    static BlockType OAK_PLANKS;
    static BlockType OAK_LEAVES;
    static BlockType OAK_SLAB;
    static BlockType OAK_STAIRS;
    static BlockType STONE_BRICKS;
    static BlockType BRICKS;
    static BlockType COAL_ORE;
    static BlockType IRON_ORE;
    static BlockType DIAMOND_ORE;
    static BlockType GLASS;
    static BlockType GLOWSTONE;
    static BlockType WATER;
    static BlockType CRAFTING_TABLE;
    
    // Initialize all block type constants from the registry
    static void initialize();
};

} // namespace Zerith