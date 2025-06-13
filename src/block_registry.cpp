#include "block_registry.h"

namespace Zerith {

// Static instance
BlockRegistry* BlockRegistry::instance_ = nullptr;

// Block definitions
const BlockDefPtr Blocks::AIR = registerBlock("air", "Air",
    BlockSettings::create()
        .model("air")
        .material(BlockMaterial::AIR));

const BlockDefPtr Blocks::STONE = registerBlock("stone", "Stone",
    BlockSettings::create()
        .model("stone")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::DIRT = registerBlock("dirt", "Dirt",
    BlockSettings::create()
        .model("dirt")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::GRASS_BLOCK = registerBlock("grass_block", "Grass Block",
    BlockSettings::create()
        .model("grass_block")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::COBBLESTONE = registerBlock("cobblestone", "Cobblestone",
    BlockSettings::create()
        .model("cobblestone")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::SAND = registerBlock("sand", "Sand",
    BlockSettings::create()
        .model("sand")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::GRAVEL = registerBlock("gravel", "Gravel",
    BlockSettings::create()
        .model("gravel")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::OAK_LOG = registerBlock("oak_log", "Oak Log",
    BlockSettings::create()
        .model("oak_log")
        .material(BlockMaterial::WOOD));

const BlockDefPtr Blocks::OAK_PLANKS = registerBlock("oak_planks", "Oak Planks",
    BlockSettings::create()
        .model("oak_planks")
        .material(BlockMaterial::WOOD));

const BlockDefPtr Blocks::OAK_LEAVES = registerBlock("oak_leaves", "Oak Leaves",
    BlockSettings::create()
        .model("oak_leaves")
        .material(BlockMaterial::LEAVES));

const BlockDefPtr Blocks::OAK_SLAB = registerBlock("oak_slab", "Oak Slab",
    BlockSettings::create()
        .model("oak_slab")
        .material(BlockMaterial::WOOD)
        .slab());

const BlockDefPtr Blocks::OAK_STAIRS = registerBlock("oak_stairs", "Oak Stairs",
    BlockSettings::create()
        .model("oak_stairs")
        .material(BlockMaterial::WOOD)
        .stairs());

const BlockDefPtr Blocks::STONE_BRICKS = registerBlock("stone_bricks", "Stone Bricks",
    BlockSettings::create()
        .model("stone_bricks")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::BRICKS = registerBlock("bricks", "Bricks",
    BlockSettings::create()
        .model("bricks")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::COAL_ORE = registerBlock("coal_ore", "Coal Ore",
    BlockSettings::create()
        .model("coal_ore")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::IRON_ORE = registerBlock("iron_ore", "Iron Ore",
    BlockSettings::create()
        .model("iron_ore")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::DIAMOND_ORE = registerBlock("diamond_ore", "Diamond Ore",
    BlockSettings::create()
        .model("diamond_ore")
        .material(BlockMaterial::STONE));

const BlockDefPtr Blocks::GLASS = registerBlock("glass", "Glass",
    BlockSettings::create()
        .model("glass")
        .material(BlockMaterial::GLASS));

const BlockDefPtr Blocks::GLOWSTONE = registerBlock("glowstone", "Glowstone",
    BlockSettings::create()
        .model("glowstone")
        .material(BlockMaterial::SOLID));

const BlockDefPtr Blocks::WATER = registerBlock("water", "Water",
    BlockSettings::create()
        .model("water")
        .material(BlockMaterial::LIQUID));

const BlockDefPtr Blocks::CRAFTING_TABLE = registerBlock("crafting_table", "Crafting Table",
    BlockSettings::create()
        .model("crafting_table")
        .material(BlockMaterial::WOOD));

void Blocks::initialize() {
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