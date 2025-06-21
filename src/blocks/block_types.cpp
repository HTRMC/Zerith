#include "block_types.h"
#include "block_registry.h"

namespace Zerith {

// Initialize static members
BlockType BlockTypes::AIR = 0;
BlockType BlockTypes::STONE = 0;
BlockType BlockTypes::DIRT = 0;
BlockType BlockTypes::GRASS_BLOCK = 0;
BlockType BlockTypes::COBBLESTONE = 0;
BlockType BlockTypes::SAND = 0;
BlockType BlockTypes::GRAVEL = 0;
BlockType BlockTypes::OAK_LOG = 0;
BlockType BlockTypes::OAK_PLANKS = 0;
BlockType BlockTypes::OAK_LEAVES = 0;
BlockType BlockTypes::OAK_SLAB = 0;
BlockType BlockTypes::OAK_STAIRS = 0;
BlockType BlockTypes::STONE_BRICKS = 0;
BlockType BlockTypes::BRICKS = 0;
BlockType BlockTypes::COAL_ORE = 0;
BlockType BlockTypes::IRON_ORE = 0;
BlockType BlockTypes::DIAMOND_ORE = 0;
BlockType BlockTypes::GLASS = 0;
BlockType BlockTypes::GLOWSTONE = 0;
BlockType BlockTypes::WATER = 0;
BlockType BlockTypes::CRAFTING_TABLE = 0;

void BlockTypes::initialize() {
    // Make sure blocks are initialized first
    Blocks::initialize();
    
    // Get the registry and populate our constants
    auto& registry = BlockRegistry::getInstance();
    
    // Helper lambda to safely get block type
    auto getBlockType = [&registry](const std::string& id) -> BlockType {
        auto block = registry.getBlock(id);
        return block ? block->getBlockType() : 0;
    };
    
    AIR = getBlockType("air");
    STONE = getBlockType("stone");
    DIRT = getBlockType("dirt");
    GRASS_BLOCK = getBlockType("grass_block");
    COBBLESTONE = getBlockType("cobblestone");
    SAND = getBlockType("sand");
    GRAVEL = getBlockType("gravel");
    OAK_LOG = getBlockType("oak_log");
    OAK_PLANKS = getBlockType("oak_planks");
    OAK_LEAVES = getBlockType("oak_leaves");
    OAK_SLAB = getBlockType("oak_slab");
    OAK_STAIRS = getBlockType("oak_stairs");
    STONE_BRICKS = getBlockType("stone_bricks");
    BRICKS = getBlockType("bricks");
    COAL_ORE = getBlockType("coal_ore");
    IRON_ORE = getBlockType("iron_ore");
    DIAMOND_ORE = getBlockType("diamond_ore");
    GLASS = getBlockType("glass");
    GLOWSTONE = getBlockType("glowstone");
    WATER = getBlockType("water");
    CRAFTING_TABLE = getBlockType("crafting_table");
}

} // namespace Zerith