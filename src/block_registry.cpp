#include "block_registry.h"

namespace Zerith {

// Static instance
BlockRegistry* BlockRegistry::instance_ = nullptr;

// Block definitions
const BlockDefPtr Blocks::AIR = registerBlock("air",
    BlockSettings::create()
        .model("air")
        .material(BlockMaterial::AIR));

const BlockDefPtr Blocks::STONE = registerBlock("stone",
    BlockSettings::create()
        .model("stone")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::DIRT = registerBlock("dirt",
    BlockSettings::create()
        .model("dirt")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::GRASS_BLOCK = registerBlock("grass_block",
    BlockSettings::create()
        .model("grass_block")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::COBBLESTONE = registerBlock("cobblestone",
    BlockSettings::create()
        .model("cobblestone")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::SAND = registerBlock("sand",
    BlockSettings::create()
        .model("sand")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::GRAVEL = registerBlock("gravel",
    BlockSettings::create()
        .model("gravel")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::OAK_LOG = registerBlock("oak_log",
    BlockSettings::create()
        .model("oak_log")
        .material(BlockMaterial::WOOD));

const BlockDefPtr Blocks::OAK_PLANKS = registerBlock("oak_planks",
    BlockSettings::create()
        .model("oak_planks")
        .material(BlockMaterial::WOOD));

const BlockDefPtr Blocks::OAK_LEAVES = registerBlock("oak_leaves",
    BlockSettings::create()
        .model("oak_leaves")
        .material(BlockMaterial::LEAVES));

const BlockDefPtr Blocks::OAK_SLAB = registerBlock("oak_slab",
    BlockSettings::create()
        .model("oak_slab")
        .material(BlockMaterial::WOOD)
        .slab());

const BlockDefPtr Blocks::OAK_STAIRS = registerBlock("oak_stairs",
    BlockSettings::create()
        .model("oak_stairs")
        .material(BlockMaterial::WOOD)
        .stairs());

const BlockDefPtr Blocks::STONE_BRICKS = registerBlock("stone_bricks",
    BlockSettings::create()
        .model("stone_bricks")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::BRICKS = registerBlock("bricks",
    BlockSettings::create()
        .model("bricks")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::COAL_ORE = registerBlock("coal_ore",
    BlockSettings::create()
        .model("coal_ore")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::IRON_ORE = registerBlock("iron_ore",
    BlockSettings::create()
        .model("iron_ore")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::DIAMOND_ORE = registerBlock("diamond_ore",
    BlockSettings::create()
        .model("diamond_ore")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::GLASS = registerBlock("glass",
    BlockSettings::create()
        .model("glass")
        .material(BlockMaterial::GLASS));

const BlockDefPtr Blocks::GLOWSTONE = registerBlock("glowstone",
    BlockSettings::create()
        .model("glowstone")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::WATER = registerBlock("water",
    BlockSettings::create()
        .model("water")
        .material(BlockMaterial::LIQUID));

const BlockDefPtr Blocks::CRAFTING_TABLE = registerBlock("crafting_table",
    BlockSettings::create()
        .model("crafting_table")
        .material(BlockMaterial::WOOD));

void Blocks::initialize() {
    // Load translations first
    TranslationManager::getInstance().loadLanguageFile("assets/zerith/lang/en_us.json");
    
    // Force static initialization of all blocks
    // This ensures all blocks are registered before use
    (void)AIR;
    (void)STONE;
    (void)DIRT;
    (void)GRASS_BLOCK;
    (void)COBBLESTONE;
    (void)SAND;
    (void)GRAVEL;
    (void)OAK_LOG;
    (void)OAK_PLANKS;
    (void)OAK_LEAVES;
    (void)OAK_SLAB;
    (void)OAK_STAIRS;
    (void)STONE_BRICKS;
    (void)BRICKS;
    (void)COAL_ORE;
    (void)IRON_ORE;
    (void)DIAMOND_ORE;
    (void)GLASS;
    (void)GLOWSTONE;
    (void)WATER;
    (void)CRAFTING_TABLE;
}

} // namespace Zerith