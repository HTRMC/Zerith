#include "blocks.h"
#include "blocks/fluid_block.h"
#include "logger.h"

namespace Zerith
{
    // Static members
    std::vector<BlockDefPtr> Blocks::blocks_;
    std::unordered_map<std::string, size_t> Blocks::idToIndex_;
    bool Blocks::initialized_ = false;

    // Block type constants
    BlockType Blocks::AIR = 0;
    BlockType Blocks::STONE = 0;
    BlockType Blocks::GRANITE = 0;
    BlockType Blocks::POLISHED_GRANITE = 0;
    BlockType Blocks::DIORITE = 0;
    BlockType Blocks::POLISHED_DIORITE = 0;
    BlockType Blocks::ANDESITE = 0;
    BlockType Blocks::POLISHED_ANDESITE = 0;
    BlockType Blocks::GRASS_BLOCK = 0;
    BlockType Blocks::DIRT = 0;
    BlockType Blocks::COARSE_DIRT = 0;
    BlockType Blocks::PODZOL = 0;
    BlockType Blocks::COBBLESTONE = 0;
    BlockType Blocks::OAK_PLANKS = 0;
    BlockType Blocks::SPRUCE_PLANKS = 0;
    BlockType Blocks::BIRCH_PLANKS = 0;
    BlockType Blocks::JUNGLE_PLANKS = 0;
    BlockType Blocks::ACACIA_PLANKS = 0;
    BlockType Blocks::CHERRY_PLANKS = 0;
    BlockType Blocks::DARK_OAK_PLANKS = 0;
    BlockType Blocks::PALE_OAK_WOOD = 0;
    BlockType Blocks::PALE_OAK_PLANKS = 0;
    BlockType Blocks::MANGROVE_PLANKS = 0;
    BlockType Blocks::BAMBOO_PLANKS = 0;
    BlockType Blocks::BAMBOO_MOSAIC = 0;
    BlockType Blocks::OAK_SAPLING = 0;
    BlockType Blocks::SPRUCE_SAPLING = 0;
    BlockType Blocks::BIRCH_SAPLING = 0;
    BlockType Blocks::JUNGLE_SAPLING = 0;
    BlockType Blocks::ACACIA_SAPLING = 0;
    BlockType Blocks::CHERRY_SAPLING = 0;
    BlockType Blocks::DARK_OAK_SAPLING = 0;
    BlockType Blocks::PALE_OAK_SAPLING = 0;
    BlockType Blocks::MANGROVE_PROPAGULE = 0;
    BlockType Blocks::BEDROCK = 0;
    BlockType Blocks::WATER = 0;
    BlockType Blocks::LAVA = 0;
    BlockType Blocks::SAND = 0;
    BlockType Blocks::SUSPICIOUS_SAND = 0;
    BlockType Blocks::RED_SAND = 0;
    BlockType Blocks::GRAVEL = 0;
    BlockType Blocks::SUSPICIOUS_GRAVEL = 0;
    BlockType Blocks::GOLD_ORE = 0;
    BlockType Blocks::DEEPSLATE_GOLD_ORE = 0;
    BlockType Blocks::IRON_ORE = 0;
    BlockType Blocks::DEEPSLATE_IRON_ORE = 0;
    BlockType Blocks::COAL_ORE = 0;
    BlockType Blocks::DEEPSLATE_COAL_ORE = 0;
    BlockType Blocks::NETHER_GOLD_ORE = 0;
    BlockType Blocks::OAK_LOG = 0;
    BlockType Blocks::SPRUCE_LOG = 0;
    BlockType Blocks::BIRCH_LOG = 0;
    BlockType Blocks::JUNGLE_LOG = 0;
    BlockType Blocks::ACACIA_LOG = 0;
    BlockType Blocks::CHERRY_LOG = 0;
    BlockType Blocks::DARK_OAK_LOG = 0;
    BlockType Blocks::PALE_OAK_LOG = 0;
    BlockType Blocks::MANGROVE_LOG = 0;
    BlockType Blocks::MANGROVE_ROOTS = 0;
    BlockType Blocks::MUDDY_MANGROVE_ROOTS = 0;
    BlockType Blocks::BAMBOO_BLOCK = 0;
    BlockType Blocks::STRIPPED_SPRUCE_LOG = 0;
    BlockType Blocks::STRIPPED_BIRCH_LOG = 0;
    BlockType Blocks::STRIPPED_JUNGLE_LOG = 0;
    BlockType Blocks::STRIPPED_ACACIA_LOG = 0;
    BlockType Blocks::STRIPPED_CHERRY_LOG = 0;
    BlockType Blocks::STRIPPED_DARK_OAK_LOG = 0;
    BlockType Blocks::STRIPPED_PALE_OAK_LOG = 0;
    BlockType Blocks::STRIPPED_OAK_LOG = 0;
    BlockType Blocks::STRIPPED_MANGROVE_LOG = 0;
    BlockType Blocks::STRIPPED_BAMBOO_BLOCK = 0;
    BlockType Blocks::OAK_WOOD = 0;
    BlockType Blocks::SPRUCE_WOOD = 0;
    BlockType Blocks::BIRCH_WOOD = 0;
    BlockType Blocks::JUNGLE_WOOD = 0;
    BlockType Blocks::ACACIA_WOOD = 0;
    BlockType Blocks::CHERRY_WOOD = 0;
    BlockType Blocks::DARK_OAK_WOOD = 0;
    BlockType Blocks::MANGROVE_WOOD = 0;
    BlockType Blocks::STRIPPED_OAK_WOOD = 0;
    BlockType Blocks::STRIPPED_SPRUCE_WOOD = 0;
    BlockType Blocks::STRIPPED_BIRCH_WOOD = 0;
    BlockType Blocks::STRIPPED_JUNGLE_WOOD = 0;
    BlockType Blocks::STRIPPED_ACACIA_WOOD = 0;
    BlockType Blocks::STRIPPED_CHERRY_WOOD = 0;
    BlockType Blocks::STRIPPED_DARK_OAK_WOOD = 0;
    BlockType Blocks::STRIPPED_PALE_OAK_WOOD = 0;
    BlockType Blocks::STRIPPED_MANGROVE_WOOD = 0;
    BlockType Blocks::OAK_LEAVES = 0;
    BlockType Blocks::SPRUCE_LEAVES = 0;
    BlockType Blocks::BIRCH_LEAVES = 0;
    BlockType Blocks::JUNGLE_LEAVES = 0;
    BlockType Blocks::ACACIA_LEAVES = 0;
    BlockType Blocks::CHERRY_LEAVES = 0;
    BlockType Blocks::DARK_OAK_LEAVES = 0;
    BlockType Blocks::PALE_OAK_LEAVES = 0;
    BlockType Blocks::MANGROVE_LEAVES = 0;
    BlockType Blocks::AZALEA_LEAVES = 0;
    BlockType Blocks::FLOWERING_AZALEA_LEAVES = 0;
    BlockType Blocks::SPONGE = 0;
    BlockType Blocks::WET_SPONGE = 0;
    BlockType Blocks::GLASS = 0;
    BlockType Blocks::LAPIS_ORE = 0;
    BlockType Blocks::DEEPSLATE_LAPIS_ORE = 0;
    BlockType Blocks::LAPIS_BLOCK = 0;
    BlockType Blocks::DISPENSER = 0;
    BlockType Blocks::SANDSTONE = 0;
    BlockType Blocks::CHISELED_SANDSTONE = 0;
    BlockType Blocks::CUT_SANDSTONE = 0;
    BlockType Blocks::NOTE_BLOCK = 0;
    BlockType Blocks::WHITE_BED = 0;
    BlockType Blocks::ORANGE_BED = 0;
    BlockType Blocks::MAGENTA_BED = 0;
    BlockType Blocks::LIGHT_BLUE_BED = 0;
    BlockType Blocks::YELLOW_BED = 0;
    BlockType Blocks::LIME_BED = 0;
    BlockType Blocks::PINK_BED = 0;
    BlockType Blocks::GRAY_BED = 0;
    BlockType Blocks::LIGHT_GRAY_BED = 0;
    BlockType Blocks::CYAN_BED = 0;
    BlockType Blocks::PURPLE_BED = 0;
    BlockType Blocks::BLUE_BED = 0;
    BlockType Blocks::BROWN_BED = 0;
    BlockType Blocks::GREEN_BED = 0;
    BlockType Blocks::RED_BED = 0;
    BlockType Blocks::BLACK_BED = 0;
    BlockType Blocks::POWERED_RAIL = 0;
    BlockType Blocks::DETECTOR_RAIL = 0;
    BlockType Blocks::STICKY_PISTON = 0;
    BlockType Blocks::COBWEB = 0;
    BlockType Blocks::SHORT_GRASS = 0;
    BlockType Blocks::FERN = 0;
    BlockType Blocks::DEAD_BUSH = 0;
    BlockType Blocks::BUSH = 0;
    BlockType Blocks::SHORT_DRY_GRASS = 0;
    BlockType Blocks::TALL_DRY_GRASS = 0;
    BlockType Blocks::SEAGRASS = 0;
    BlockType Blocks::TALL_SEAGRASS = 0;
    BlockType Blocks::PISTON = 0;
    BlockType Blocks::PISTON_HEAD = 0;
    BlockType Blocks::WHITE_WOOL = 0;
    BlockType Blocks::ORANGE_WOOL = 0;
    BlockType Blocks::MAGENTA_WOOL = 0;
    BlockType Blocks::LIGHT_BLUE_WOOL = 0;
    BlockType Blocks::YELLOW_WOOL = 0;
    BlockType Blocks::LIME_WOOL = 0;
    BlockType Blocks::PINK_WOOL = 0;
    BlockType Blocks::GRAY_WOOL = 0;
    BlockType Blocks::LIGHT_GRAY_WOOL = 0;
    BlockType Blocks::CYAN_WOOL = 0;
    BlockType Blocks::PURPLE_WOOL = 0;
    BlockType Blocks::BLUE_WOOL = 0;
    BlockType Blocks::BROWN_WOOL = 0;
    BlockType Blocks::GREEN_WOOL = 0;
    BlockType Blocks::RED_WOOL = 0;
    BlockType Blocks::BLACK_WOOL = 0;
    BlockType Blocks::MOVING_PISTON = 0;
    BlockType Blocks::DANDELION = 0;
    BlockType Blocks::TORCHFLOWER = 0;
    BlockType Blocks::POPPY = 0;
    BlockType Blocks::BLUE_ORCHID = 0;
    BlockType Blocks::ALLIUM = 0;
    BlockType Blocks::AZURE_BLUET = 0;
    BlockType Blocks::RED_TULIP = 0;
    BlockType Blocks::ORANGE_TULIP = 0;
    BlockType Blocks::WHITE_TULIP = 0;
    BlockType Blocks::PINK_TULIP = 0;
    BlockType Blocks::OXEYE_DAISY = 0;
    BlockType Blocks::CORNFLOWER = 0;
    BlockType Blocks::WITHER_ROSE = 0;
    BlockType Blocks::LILY_OF_THE_VALLEY = 0;
    BlockType Blocks::BROWN_MUSHROOM = 0;
    BlockType Blocks::RED_MUSHROOM = 0;
    BlockType Blocks::GOLD_BLOCK = 0;
    BlockType Blocks::IRON_BLOCK = 0;
    BlockType Blocks::BRICKS = 0;
    BlockType Blocks::TNT = 0;
    BlockType Blocks::BOOKSHELF = 0;
    BlockType Blocks::CHISELED_BOOKSHELF = 0;
    BlockType Blocks::MOSSY_COBBLESTONE = 0;
    BlockType Blocks::OBSIDIAN = 0;
    BlockType Blocks::TORCH = 0;
    BlockType Blocks::WALL_TORCH = 0;
    BlockType Blocks::FIRE = 0;
    BlockType Blocks::SOUL_FIRE = 0;
    BlockType Blocks::SPAWNER = 0;
    BlockType Blocks::CREAKING_HEART = 0;
    BlockType Blocks::OAK_STAIRS = 0;
    BlockType Blocks::CHEST = 0;
    BlockType Blocks::REDSTONE_WIRE = 0;
    BlockType Blocks::DIAMOND_ORE = 0;
    BlockType Blocks::DEEPSLATE_DIAMOND_ORE = 0;
    BlockType Blocks::DIAMOND_BLOCK = 0;
    BlockType Blocks::CRAFTING_TABLE = 0;
    BlockType Blocks::WHEAT = 0;
    BlockType Blocks::FARMLAND = 0;
    BlockType Blocks::FURNACE = 0;
    BlockType Blocks::OAK_SIGN = 0;
    BlockType Blocks::SPRUCE_SIGN = 0;
    BlockType Blocks::BIRCH_SIGN = 0;
    BlockType Blocks::ACACIA_SIGN = 0;
    BlockType Blocks::CHERRY_SIGN = 0;
    BlockType Blocks::JUNGLE_SIGN = 0;
    BlockType Blocks::DARK_OAK_SIGN = 0;
    BlockType Blocks::PALE_OAK_SIGN = 0;
    BlockType Blocks::MANGROVE_SIGN = 0;
    BlockType Blocks::BAMBOO_SIGN = 0;
    BlockType Blocks::OAK_DOOR = 0;
    BlockType Blocks::LADDER = 0;
    BlockType Blocks::RAIL = 0;
    BlockType Blocks::COBBLESTONE_STAIRS = 0;
    BlockType Blocks::OAK_WALL_SIGN = 0;
    BlockType Blocks::SPRUCE_WALL_SIGN = 0;
    BlockType Blocks::BIRCH_WALL_SIGN = 0;
    BlockType Blocks::ACACIA_WALL_SIGN = 0;
    BlockType Blocks::CHERRY_WALL_SIGN = 0;
    BlockType Blocks::JUNGLE_WALL_SIGN = 0;
    BlockType Blocks::DARK_OAK_WALL_SIGN = 0;
    BlockType Blocks::PALE_OAK_WALL_SIGN = 0;
    BlockType Blocks::MANGROVE_WALL_SIGN = 0;
    BlockType Blocks::BAMBOO_WALL_SIGN = 0;
    BlockType Blocks::OAK_HANGING_SIGN = 0;
    BlockType Blocks::SPRUCE_HANGING_SIGN = 0;
    BlockType Blocks::BIRCH_HANGING_SIGN = 0;
    BlockType Blocks::ACACIA_HANGING_SIGN = 0;
    BlockType Blocks::CHERRY_HANGING_SIGN = 0;
    BlockType Blocks::JUNGLE_HANGING_SIGN = 0;
    BlockType Blocks::DARK_OAK_HANGING_SIGN = 0;
    BlockType Blocks::PALE_OAK_HANGING_SIGN = 0;
    BlockType Blocks::CRIMSON_HANGING_SIGN = 0;
    BlockType Blocks::WARPED_HANGING_SIGN = 0;
    BlockType Blocks::MANGROVE_HANGING_SIGN = 0;
    BlockType Blocks::BAMBOO_HANGING_SIGN = 0;
    BlockType Blocks::OAK_WALL_HANGING_SIGN = 0;
    BlockType Blocks::SPRUCE_WALL_HANGING_SIGN = 0;
    BlockType Blocks::BIRCH_WALL_HANGING_SIGN = 0;
    BlockType Blocks::ACACIA_WALL_HANGING_SIGN = 0;
    BlockType Blocks::CHERRY_WALL_HANGING_SIGN = 0;
    BlockType Blocks::JUNGLE_WALL_HANGING_SIGN = 0;
    BlockType Blocks::DARK_OAK_WALL_HANGING_SIGN = 0;
    BlockType Blocks::PALE_OAK_WALL_HANGING_SIGN = 0;
    BlockType Blocks::MANGROVE_WALL_HANGING_SIGN = 0;
    BlockType Blocks::CRIMSON_WALL_HANGING_SIGN = 0;
    BlockType Blocks::WARPED_WALL_HANGING_SIGN = 0;
    BlockType Blocks::BAMBOO_WALL_HANGING_SIGN = 0;
    BlockType Blocks::LEVER = 0;
    BlockType Blocks::STONE_PRESSURE_PLATE = 0;
    BlockType Blocks::IRON_DOOR = 0;
    BlockType Blocks::OAK_PRESSURE_PLATE = 0;
    BlockType Blocks::SPRUCE_PRESSURE_PLATE = 0;
    BlockType Blocks::BIRCH_PRESSURE_PLATE = 0;
    BlockType Blocks::JUNGLE_PRESSURE_PLATE = 0;
    BlockType Blocks::ACACIA_PRESSURE_PLATE = 0;
    BlockType Blocks::CHERRY_PRESSURE_PLATE = 0;
    BlockType Blocks::DARK_OAK_PRESSURE_PLATE = 0;
    BlockType Blocks::PALE_OAK_PRESSURE_PLATE = 0;
    BlockType Blocks::MANGROVE_PRESSURE_PLATE = 0;
    BlockType Blocks::BAMBOO_PRESSURE_PLATE = 0;
    BlockType Blocks::REDSTONE_ORE = 0;
    BlockType Blocks::DEEPSLATE_REDSTONE_ORE = 0;
    BlockType Blocks::REDSTONE_TORCH = 0;
    BlockType Blocks::REDSTONE_WALL_TORCH = 0;
    BlockType Blocks::STONE_BUTTON = 0;
    BlockType Blocks::SNOW = 0;
    BlockType Blocks::ICE = 0;
    BlockType Blocks::SNOW_BLOCK = 0;
    BlockType Blocks::CACTUS = 0;
    BlockType Blocks::CACTUS_FLOWER = 0;
    BlockType Blocks::CLAY = 0;
    BlockType Blocks::SUGAR_CANE = 0;
    BlockType Blocks::JUKEBOX = 0;
    BlockType Blocks::OAK_FENCE = 0;
    BlockType Blocks::NETHERRACK = 0;
    BlockType Blocks::SOUL_SAND = 0;
    BlockType Blocks::SOUL_SOIL = 0;
    BlockType Blocks::BASALT = 0;
    BlockType Blocks::POLISHED_BASALT = 0;
    BlockType Blocks::SOUL_TORCH = 0;
    BlockType Blocks::SOUL_WALL_TORCH = 0;
    BlockType Blocks::GLOWSTONE = 0;
    BlockType Blocks::NETHER_PORTAL = 0;
    BlockType Blocks::CARVED_PUMPKIN = 0;
    BlockType Blocks::JACK_O_LANTERN = 0;
    BlockType Blocks::CAKE = 0;
    BlockType Blocks::REPEATER = 0;
    BlockType Blocks::WHITE_STAINED_GLASS = 0;
    BlockType Blocks::ORANGE_STAINED_GLASS = 0;
    BlockType Blocks::MAGENTA_STAINED_GLASS = 0;
    BlockType Blocks::LIGHT_BLUE_STAINED_GLASS = 0;
    BlockType Blocks::YELLOW_STAINED_GLASS = 0;
    BlockType Blocks::LIME_STAINED_GLASS = 0;
    BlockType Blocks::PINK_STAINED_GLASS = 0;
    BlockType Blocks::GRAY_STAINED_GLASS = 0;
    BlockType Blocks::LIGHT_GRAY_STAINED_GLASS = 0;
    BlockType Blocks::CYAN_STAINED_GLASS = 0;
    BlockType Blocks::PURPLE_STAINED_GLASS = 0;
    BlockType Blocks::BLUE_STAINED_GLASS = 0;
    BlockType Blocks::BROWN_STAINED_GLASS = 0;
    BlockType Blocks::GREEN_STAINED_GLASS = 0;
    BlockType Blocks::RED_STAINED_GLASS = 0;
    BlockType Blocks::BLACK_STAINED_GLASS = 0;
    BlockType Blocks::OAK_TRAPDOOR = 0;
    BlockType Blocks::SPRUCE_TRAPDOOR = 0;
    BlockType Blocks::BIRCH_TRAPDOOR = 0;
    BlockType Blocks::JUNGLE_TRAPDOOR = 0;
    BlockType Blocks::ACACIA_TRAPDOOR = 0;
    BlockType Blocks::CHERRY_TRAPDOOR = 0;
    BlockType Blocks::DARK_OAK_TRAPDOOR = 0;
    BlockType Blocks::PALE_OAK_TRAPDOOR = 0;
    BlockType Blocks::MANGROVE_TRAPDOOR = 0;
    BlockType Blocks::BAMBOO_TRAPDOOR = 0;
    BlockType Blocks::STONE_BRICKS = 0;
    BlockType Blocks::MOSSY_STONE_BRICKS = 0;
    BlockType Blocks::CRACKED_STONE_BRICKS = 0;
    BlockType Blocks::CHISELED_STONE_BRICKS = 0;
    BlockType Blocks::PACKED_MUD = 0;
    BlockType Blocks::MUD_BRICKS = 0;
    BlockType Blocks::INFESTED_STONE = 0;
    BlockType Blocks::INFESTED_COBBLESTONE = 0;
    BlockType Blocks::INFESTED_STONE_BRICKS = 0;
    BlockType Blocks::INFESTED_MOSSY_STONE_BRICKS = 0;
    BlockType Blocks::INFESTED_CRACKED_STONE_BRICKS = 0;
    BlockType Blocks::INFESTED_CHISELED_STONE_BRICKS = 0;
    BlockType Blocks::BROWN_MUSHROOM_BLOCK = 0;
    BlockType Blocks::RED_MUSHROOM_BLOCK = 0;
    BlockType Blocks::MUSHROOM_STEM = 0;
    BlockType Blocks::IRON_BARS = 0;
    BlockType Blocks::CHAIN = 0;
    BlockType Blocks::GLASS_PANE = 0;
    BlockType Blocks::PUMPKIN = 0;
    BlockType Blocks::MELON = 0;
    BlockType Blocks::ATTACHED_PUMPKIN_STEM = 0;
    BlockType Blocks::ATTACHED_MELON_STEM = 0;
    BlockType Blocks::PUMPKIN_STEM = 0;
    BlockType Blocks::MELON_STEM = 0;
    BlockType Blocks::VINE = 0;
    BlockType Blocks::GLOW_LICHEN = 0;
    BlockType Blocks::RESIN_CLUMP = 0;
    BlockType Blocks::OAK_FENCE_GATE = 0;
    BlockType Blocks::BRICK_STAIRS = 0;
    BlockType Blocks::STONE_BRICK_STAIRS = 0;
    BlockType Blocks::MUD_BRICK_STAIRS = 0;
    BlockType Blocks::MYCELIUM = 0;
    BlockType Blocks::LILY_PAD = 0;
    BlockType Blocks::RESIN_BLOCK = 0;
    BlockType Blocks::RESIN_BRICKS = 0;
    BlockType Blocks::RESIN_BRICK_STAIRS = 0;
    BlockType Blocks::RESIN_BRICK_SLAB = 0;
    BlockType Blocks::RESIN_BRICK_WALL = 0;
    BlockType Blocks::CHISELED_RESIN_BRICKS = 0;
    BlockType Blocks::NETHER_BRICKS = 0;
    BlockType Blocks::NETHER_BRICK_FENCE = 0;
    BlockType Blocks::NETHER_BRICK_STAIRS = 0;
    BlockType Blocks::NETHER_WART = 0;
    BlockType Blocks::ENCHANTING_TABLE = 0;
    BlockType Blocks::BREWING_STAND = 0;
    BlockType Blocks::CAULDRON = 0;
    BlockType Blocks::WATER_CAULDRON = 0;
    BlockType Blocks::LAVA_CAULDRON = 0;
    BlockType Blocks::POWDER_SNOW_CAULDRON = 0;
    BlockType Blocks::END_PORTAL = 0;
    BlockType Blocks::END_PORTAL_FRAME = 0;
    BlockType Blocks::END_STONE = 0;
    BlockType Blocks::DRAGON_EGG = 0;
    BlockType Blocks::REDSTONE_LAMP = 0;
    BlockType Blocks::COCOA = 0;
    BlockType Blocks::SANDSTONE_STAIRS = 0;
    BlockType Blocks::EMERALD_ORE = 0;
    BlockType Blocks::DEEPSLATE_EMERALD_ORE = 0;
    BlockType Blocks::ENDER_CHEST = 0;
    BlockType Blocks::TRIPWIRE_HOOK = 0;
    BlockType Blocks::TRIPWIRE = 0;
    BlockType Blocks::EMERALD_BLOCK = 0;
    BlockType Blocks::SPRUCE_STAIRS = 0;
    BlockType Blocks::BIRCH_STAIRS = 0;
    BlockType Blocks::JUNGLE_STAIRS = 0;
    BlockType Blocks::COMMAND_BLOCK = 0;
    BlockType Blocks::BEACON = 0;
    BlockType Blocks::COBBLESTONE_WALL = 0;
    BlockType Blocks::MOSSY_COBBLESTONE_WALL = 0;
    BlockType Blocks::FLOWER_POT = 0;
    BlockType Blocks::POTTED_TORCHFLOWER = 0;
    BlockType Blocks::POTTED_OAK_SAPLING = 0;
    BlockType Blocks::POTTED_SPRUCE_SAPLING = 0;
    BlockType Blocks::POTTED_BIRCH_SAPLING = 0;
    BlockType Blocks::POTTED_JUNGLE_SAPLING = 0;
    BlockType Blocks::POTTED_ACACIA_SAPLING = 0;
    BlockType Blocks::POTTED_CHERRY_SAPLING = 0;
    BlockType Blocks::POTTED_DARK_OAK_SAPLING = 0;
    BlockType Blocks::POTTED_PALE_OAK_SAPLING = 0;
    BlockType Blocks::POTTED_MANGROVE_PROPAGULE = 0;
    BlockType Blocks::POTTED_FERN = 0;
    BlockType Blocks::POTTED_DANDELION = 0;
    BlockType Blocks::POTTED_POPPY = 0;
    BlockType Blocks::POTTED_BLUE_ORCHID = 0;
    BlockType Blocks::POTTED_ALLIUM = 0;
    BlockType Blocks::POTTED_AZURE_BLUET = 0;
    BlockType Blocks::POTTED_RED_TULIP = 0;
    BlockType Blocks::POTTED_ORANGE_TULIP = 0;
    BlockType Blocks::POTTED_WHITE_TULIP = 0;
    BlockType Blocks::POTTED_PINK_TULIP = 0;
    BlockType Blocks::POTTED_OXEYE_DAISY = 0;
    BlockType Blocks::POTTED_CORNFLOWER = 0;
    BlockType Blocks::POTTED_LILY_OF_THE_VALLEY = 0;
    BlockType Blocks::POTTED_WITHER_ROSE = 0;
    BlockType Blocks::POTTED_RED_MUSHROOM = 0;
    BlockType Blocks::POTTED_BROWN_MUSHROOM = 0;
    BlockType Blocks::POTTED_DEAD_BUSH = 0;
    BlockType Blocks::POTTED_CACTUS = 0;
    BlockType Blocks::CARROTS = 0;
    BlockType Blocks::POTATOES = 0;
    BlockType Blocks::OAK_BUTTON = 0;
    BlockType Blocks::SPRUCE_BUTTON = 0;
    BlockType Blocks::BIRCH_BUTTON = 0;
    BlockType Blocks::JUNGLE_BUTTON = 0;
    BlockType Blocks::ACACIA_BUTTON = 0;
    BlockType Blocks::CHERRY_BUTTON = 0;
    BlockType Blocks::DARK_OAK_BUTTON = 0;
    BlockType Blocks::PALE_OAK_BUTTON = 0;
    BlockType Blocks::MANGROVE_BUTTON = 0;
    BlockType Blocks::BAMBOO_BUTTON = 0;
    BlockType Blocks::SKELETON_SKULL = 0;
    BlockType Blocks::SKELETON_WALL_SKULL = 0;
    BlockType Blocks::WITHER_SKELETON_SKULL = 0;
    BlockType Blocks::WITHER_SKELETON_WALL_SKULL = 0;
    BlockType Blocks::ZOMBIE_HEAD = 0;
    BlockType Blocks::ZOMBIE_WALL_HEAD = 0;
    BlockType Blocks::PLAYER_HEAD = 0;
    BlockType Blocks::PLAYER_WALL_HEAD = 0;
    BlockType Blocks::CREEPER_HEAD = 0;
    BlockType Blocks::CREEPER_WALL_HEAD = 0;
    BlockType Blocks::DRAGON_HEAD = 0;
    BlockType Blocks::DRAGON_WALL_HEAD = 0;
    BlockType Blocks::PIGLIN_HEAD = 0;
    BlockType Blocks::PIGLIN_WALL_HEAD = 0;
    BlockType Blocks::ANVIL = 0;
    BlockType Blocks::CHIPPED_ANVIL = 0;
    BlockType Blocks::DAMAGED_ANVIL = 0;
    BlockType Blocks::TRAPPED_CHEST = 0;
    BlockType Blocks::LIGHT_WEIGHTED_PRESSURE_PLATE = 0;
    BlockType Blocks::HEAVY_WEIGHTED_PRESSURE_PLATE = 0;
    BlockType Blocks::COMPARATOR = 0;
    BlockType Blocks::DAYLIGHT_DETECTOR = 0;
    BlockType Blocks::REDSTONE_BLOCK = 0;
    BlockType Blocks::NETHER_QUARTZ_ORE = 0;
    BlockType Blocks::HOPPER = 0;
    BlockType Blocks::QUARTZ_BLOCK = 0;
    BlockType Blocks::CHISELED_QUARTZ_BLOCK = 0;
    BlockType Blocks::QUARTZ_PILLAR = 0;
    BlockType Blocks::QUARTZ_STAIRS = 0;
    BlockType Blocks::ACTIVATOR_RAIL = 0;
    BlockType Blocks::DROPPER = 0;
    BlockType Blocks::WHITE_TERRACOTTA = 0;
    BlockType Blocks::ORANGE_TERRACOTTA = 0;
    BlockType Blocks::MAGENTA_TERRACOTTA = 0;
    BlockType Blocks::LIGHT_BLUE_TERRACOTTA = 0;
    BlockType Blocks::YELLOW_TERRACOTTA = 0;
    BlockType Blocks::LIME_TERRACOTTA = 0;
    BlockType Blocks::PINK_TERRACOTTA = 0;
    BlockType Blocks::GRAY_TERRACOTTA = 0;
    BlockType Blocks::LIGHT_GRAY_TERRACOTTA = 0;
    BlockType Blocks::CYAN_TERRACOTTA = 0;
    BlockType Blocks::PURPLE_TERRACOTTA = 0;
    BlockType Blocks::BLUE_TERRACOTTA = 0;
    BlockType Blocks::BROWN_TERRACOTTA = 0;
    BlockType Blocks::GREEN_TERRACOTTA = 0;
    BlockType Blocks::RED_TERRACOTTA = 0;
    BlockType Blocks::BLACK_TERRACOTTA = 0;
    BlockType Blocks::WHITE_STAINED_GLASS_PANE = 0;
    BlockType Blocks::ORANGE_STAINED_GLASS_PANE = 0;
    BlockType Blocks::MAGENTA_STAINED_GLASS_PANE = 0;
    BlockType Blocks::LIGHT_BLUE_STAINED_GLASS_PANE = 0;
    BlockType Blocks::YELLOW_STAINED_GLASS_PANE = 0;
    BlockType Blocks::LIME_STAINED_GLASS_PANE = 0;
    BlockType Blocks::PINK_STAINED_GLASS_PANE = 0;
    BlockType Blocks::GRAY_STAINED_GLASS_PANE = 0;
    BlockType Blocks::LIGHT_GRAY_STAINED_GLASS_PANE = 0;
    BlockType Blocks::CYAN_STAINED_GLASS_PANE = 0;
    BlockType Blocks::PURPLE_STAINED_GLASS_PANE = 0;
    BlockType Blocks::BLUE_STAINED_GLASS_PANE = 0;
    BlockType Blocks::BROWN_STAINED_GLASS_PANE = 0;
    BlockType Blocks::GREEN_STAINED_GLASS_PANE = 0;
    BlockType Blocks::RED_STAINED_GLASS_PANE = 0;
    BlockType Blocks::BLACK_STAINED_GLASS_PANE = 0;
    BlockType Blocks::ACACIA_STAIRS = 0;
    BlockType Blocks::CHERRY_STAIRS = 0;
    BlockType Blocks::DARK_OAK_STAIRS = 0;
    BlockType Blocks::PALE_OAK_STAIRS = 0;
    BlockType Blocks::MANGROVE_STAIRS = 0;
    BlockType Blocks::BAMBOO_STAIRS = 0;
    BlockType Blocks::BAMBOO_MOSAIC_STAIRS = 0;
    BlockType Blocks::SLIME_BLOCK = 0;
    BlockType Blocks::BARRIER = 0;
    BlockType Blocks::LIGHT = 0;
    BlockType Blocks::IRON_TRAPDOOR = 0;
    BlockType Blocks::PRISMARINE = 0;
    BlockType Blocks::PRISMARINE_BRICKS = 0;
    BlockType Blocks::DARK_PRISMARINE = 0;
    BlockType Blocks::PRISMARINE_STAIRS = 0;
    BlockType Blocks::PRISMARINE_BRICK_STAIRS = 0;
    BlockType Blocks::DARK_PRISMARINE_STAIRS = 0;
    BlockType Blocks::PRISMARINE_SLAB = 0;
    BlockType Blocks::PRISMARINE_BRICK_SLAB = 0;
    BlockType Blocks::DARK_PRISMARINE_SLAB = 0;
    BlockType Blocks::SEA_LANTERN = 0;
    BlockType Blocks::HAY_BLOCK = 0;
    BlockType Blocks::WHITE_CARPET = 0;
    BlockType Blocks::ORANGE_CARPET = 0;
    BlockType Blocks::MAGENTA_CARPET = 0;
    BlockType Blocks::LIGHT_BLUE_CARPET = 0;
    BlockType Blocks::YELLOW_CARPET = 0;
    BlockType Blocks::LIME_CARPET = 0;
    BlockType Blocks::PINK_CARPET = 0;
    BlockType Blocks::GRAY_CARPET = 0;
    BlockType Blocks::LIGHT_GRAY_CARPET = 0;
    BlockType Blocks::CYAN_CARPET = 0;
    BlockType Blocks::PURPLE_CARPET = 0;
    BlockType Blocks::BLUE_CARPET = 0;
    BlockType Blocks::BROWN_CARPET = 0;
    BlockType Blocks::GREEN_CARPET = 0;
    BlockType Blocks::RED_CARPET = 0;
    BlockType Blocks::BLACK_CARPET = 0;
    BlockType Blocks::TERRACOTTA = 0;
    BlockType Blocks::COAL_BLOCK = 0;
    BlockType Blocks::PACKED_ICE = 0;
    BlockType Blocks::SUNFLOWER = 0;
    BlockType Blocks::LILAC = 0;
    BlockType Blocks::ROSE_BUSH = 0;
    BlockType Blocks::PEONY = 0;
    BlockType Blocks::TALL_GRASS = 0;
    BlockType Blocks::LARGE_FERN = 0;
    BlockType Blocks::WHITE_BANNER = 0;
    BlockType Blocks::ORANGE_BANNER = 0;
    BlockType Blocks::MAGENTA_BANNER = 0;
    BlockType Blocks::LIGHT_BLUE_BANNER = 0;
    BlockType Blocks::YELLOW_BANNER = 0;
    BlockType Blocks::LIME_BANNER = 0;
    BlockType Blocks::PINK_BANNER = 0;
    BlockType Blocks::GRAY_BANNER = 0;
    BlockType Blocks::LIGHT_GRAY_BANNER = 0;
    BlockType Blocks::CYAN_BANNER = 0;
    BlockType Blocks::PURPLE_BANNER = 0;
    BlockType Blocks::BLUE_BANNER = 0;
    BlockType Blocks::BROWN_BANNER = 0;
    BlockType Blocks::GREEN_BANNER = 0;
    BlockType Blocks::RED_BANNER = 0;
    BlockType Blocks::BLACK_BANNER = 0;
    BlockType Blocks::WHITE_WALL_BANNER = 0;
    BlockType Blocks::ORANGE_WALL_BANNER = 0;
    BlockType Blocks::MAGENTA_WALL_BANNER = 0;
    BlockType Blocks::LIGHT_BLUE_WALL_BANNER = 0;
    BlockType Blocks::YELLOW_WALL_BANNER = 0;
    BlockType Blocks::LIME_WALL_BANNER = 0;
    BlockType Blocks::PINK_WALL_BANNER = 0;
    BlockType Blocks::GRAY_WALL_BANNER = 0;
    BlockType Blocks::LIGHT_GRAY_WALL_BANNER = 0;
    BlockType Blocks::CYAN_WALL_BANNER = 0;
    BlockType Blocks::PURPLE_WALL_BANNER = 0;
    BlockType Blocks::BLUE_WALL_BANNER = 0;
    BlockType Blocks::BROWN_WALL_BANNER = 0;
    BlockType Blocks::GREEN_WALL_BANNER = 0;
    BlockType Blocks::RED_WALL_BANNER = 0;
    BlockType Blocks::BLACK_WALL_BANNER = 0;
    BlockType Blocks::RED_SANDSTONE = 0;
    BlockType Blocks::CHISELED_RED_SANDSTONE = 0;
    BlockType Blocks::CUT_RED_SANDSTONE = 0;
    BlockType Blocks::RED_SANDSTONE_STAIRS = 0;
    BlockType Blocks::OAK_SLAB = 0;
    BlockType Blocks::SPRUCE_SLAB = 0;
    BlockType Blocks::BIRCH_SLAB = 0;
    BlockType Blocks::JUNGLE_SLAB = 0;
    BlockType Blocks::ACACIA_SLAB = 0;
    BlockType Blocks::CHERRY_SLAB = 0;
    BlockType Blocks::DARK_OAK_SLAB = 0;
    BlockType Blocks::PALE_OAK_SLAB = 0;
    BlockType Blocks::MANGROVE_SLAB = 0;
    BlockType Blocks::BAMBOO_SLAB = 0;
    BlockType Blocks::BAMBOO_MOSAIC_SLAB = 0;
    BlockType Blocks::STONE_SLAB = 0;
    BlockType Blocks::SMOOTH_STONE_SLAB = 0;
    BlockType Blocks::SANDSTONE_SLAB = 0;
    BlockType Blocks::CUT_SANDSTONE_SLAB = 0;
    BlockType Blocks::PETRIFIED_OAK_SLAB = 0;
    BlockType Blocks::COBBLESTONE_SLAB = 0;
    BlockType Blocks::BRICK_SLAB = 0;
    BlockType Blocks::STONE_BRICK_SLAB = 0;
    BlockType Blocks::MUD_BRICK_SLAB = 0;
    BlockType Blocks::NETHER_BRICK_SLAB = 0;
    BlockType Blocks::QUARTZ_SLAB = 0;
    BlockType Blocks::RED_SANDSTONE_SLAB = 0;
    BlockType Blocks::CUT_RED_SANDSTONE_SLAB = 0;
    BlockType Blocks::PURPUR_SLAB = 0;
    BlockType Blocks::SMOOTH_STONE = 0;
    BlockType Blocks::SMOOTH_SANDSTONE = 0;
    BlockType Blocks::SMOOTH_QUARTZ = 0;
    BlockType Blocks::SMOOTH_RED_SANDSTONE = 0;
    BlockType Blocks::SPRUCE_FENCE_GATE = 0;
    BlockType Blocks::BIRCH_FENCE_GATE = 0;
    BlockType Blocks::JUNGLE_FENCE_GATE = 0;
    BlockType Blocks::ACACIA_FENCE_GATE = 0;
    BlockType Blocks::CHERRY_FENCE_GATE = 0;
    BlockType Blocks::DARK_OAK_FENCE_GATE = 0;
    BlockType Blocks::PALE_OAK_FENCE_GATE = 0;
    BlockType Blocks::MANGROVE_FENCE_GATE = 0;
    BlockType Blocks::BAMBOO_FENCE_GATE = 0;
    BlockType Blocks::SPRUCE_FENCE = 0;
    BlockType Blocks::BIRCH_FENCE = 0;
    BlockType Blocks::JUNGLE_FENCE = 0;
    BlockType Blocks::ACACIA_FENCE = 0;
    BlockType Blocks::CHERRY_FENCE = 0;
    BlockType Blocks::DARK_OAK_FENCE = 0;
    BlockType Blocks::PALE_OAK_FENCE = 0;
    BlockType Blocks::MANGROVE_FENCE = 0;
    BlockType Blocks::BAMBOO_FENCE = 0;
    BlockType Blocks::SPRUCE_DOOR = 0;
    BlockType Blocks::BIRCH_DOOR = 0;
    BlockType Blocks::JUNGLE_DOOR = 0;
    BlockType Blocks::ACACIA_DOOR = 0;
    BlockType Blocks::CHERRY_DOOR = 0;
    BlockType Blocks::DARK_OAK_DOOR = 0;
    BlockType Blocks::PALE_OAK_DOOR = 0;
    BlockType Blocks::MANGROVE_DOOR = 0;
    BlockType Blocks::BAMBOO_DOOR = 0;
    BlockType Blocks::END_ROD = 0;
    BlockType Blocks::CHORUS_PLANT = 0;
    BlockType Blocks::CHORUS_FLOWER = 0;
    BlockType Blocks::PURPUR_BLOCK = 0;
    BlockType Blocks::PURPUR_PILLAR = 0;
    BlockType Blocks::PURPUR_STAIRS = 0;
    BlockType Blocks::END_STONE_BRICKS = 0;
    BlockType Blocks::TORCHFLOWER_CROP = 0;
    BlockType Blocks::PITCHER_CROP = 0;
    BlockType Blocks::PITCHER_PLANT = 0;
    BlockType Blocks::BEETROOTS = 0;
    BlockType Blocks::DIRT_PATH = 0;
    BlockType Blocks::END_GATEWAY = 0;
    BlockType Blocks::REPEATING_COMMAND_BLOCK = 0;
    BlockType Blocks::CHAIN_COMMAND_BLOCK = 0;
    BlockType Blocks::FROSTED_ICE = 0;
    BlockType Blocks::MAGMA_BLOCK = 0;
    BlockType Blocks::NETHER_WART_BLOCK = 0;
    BlockType Blocks::RED_NETHER_BRICKS = 0;
    BlockType Blocks::BONE_BLOCK = 0;
    BlockType Blocks::STRUCTURE_VOID = 0;
    BlockType Blocks::OBSERVER = 0;
    BlockType Blocks::SHULKER_BOX = 0;
    BlockType Blocks::WHITE_SHULKER_BOX = 0;
    BlockType Blocks::ORANGE_SHULKER_BOX = 0;
    BlockType Blocks::MAGENTA_SHULKER_BOX = 0;
    BlockType Blocks::LIGHT_BLUE_SHULKER_BOX = 0;
    BlockType Blocks::YELLOW_SHULKER_BOX = 0;
    BlockType Blocks::LIME_SHULKER_BOX = 0;
    BlockType Blocks::PINK_SHULKER_BOX = 0;
    BlockType Blocks::GRAY_SHULKER_BOX = 0;
    BlockType Blocks::LIGHT_GRAY_SHULKER_BOX = 0;
    BlockType Blocks::CYAN_SHULKER_BOX = 0;
    BlockType Blocks::PURPLE_SHULKER_BOX = 0;
    BlockType Blocks::BLUE_SHULKER_BOX = 0;
    BlockType Blocks::BROWN_SHULKER_BOX = 0;
    BlockType Blocks::GREEN_SHULKER_BOX = 0;
    BlockType Blocks::RED_SHULKER_BOX = 0;
    BlockType Blocks::BLACK_SHULKER_BOX = 0;
    BlockType Blocks::WHITE_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::ORANGE_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::MAGENTA_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::LIGHT_BLUE_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::YELLOW_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::LIME_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::PINK_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::GRAY_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::LIGHT_GRAY_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::CYAN_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::PURPLE_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::BLUE_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::BROWN_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::GREEN_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::RED_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::BLACK_GLAZED_TERRACOTTA = 0;
    BlockType Blocks::WHITE_CONCRETE = 0;
    BlockType Blocks::ORANGE_CONCRETE = 0;
    BlockType Blocks::MAGENTA_CONCRETE = 0;
    BlockType Blocks::LIGHT_BLUE_CONCRETE = 0;
    BlockType Blocks::YELLOW_CONCRETE = 0;
    BlockType Blocks::LIME_CONCRETE = 0;
    BlockType Blocks::PINK_CONCRETE = 0;
    BlockType Blocks::GRAY_CONCRETE = 0;
    BlockType Blocks::LIGHT_GRAY_CONCRETE = 0;
    BlockType Blocks::CYAN_CONCRETE = 0;
    BlockType Blocks::PURPLE_CONCRETE = 0;
    BlockType Blocks::BLUE_CONCRETE = 0;
    BlockType Blocks::BROWN_CONCRETE = 0;
    BlockType Blocks::GREEN_CONCRETE = 0;
    BlockType Blocks::RED_CONCRETE = 0;
    BlockType Blocks::BLACK_CONCRETE = 0;
    BlockType Blocks::WHITE_CONCRETE_POWDER = 0;
    BlockType Blocks::ORANGE_CONCRETE_POWDER = 0;
    BlockType Blocks::MAGENTA_CONCRETE_POWDER = 0;
    BlockType Blocks::LIGHT_BLUE_CONCRETE_POWDER = 0;
    BlockType Blocks::YELLOW_CONCRETE_POWDER = 0;
    BlockType Blocks::LIME_CONCRETE_POWDER = 0;
    BlockType Blocks::PINK_CONCRETE_POWDER = 0;
    BlockType Blocks::GRAY_CONCRETE_POWDER = 0;
    BlockType Blocks::LIGHT_GRAY_CONCRETE_POWDER = 0;
    BlockType Blocks::CYAN_CONCRETE_POWDER = 0;
    BlockType Blocks::PURPLE_CONCRETE_POWDER = 0;
    BlockType Blocks::BLUE_CONCRETE_POWDER = 0;
    BlockType Blocks::BROWN_CONCRETE_POWDER = 0;
    BlockType Blocks::GREEN_CONCRETE_POWDER = 0;
    BlockType Blocks::RED_CONCRETE_POWDER = 0;
    BlockType Blocks::BLACK_CONCRETE_POWDER = 0;
    BlockType Blocks::KELP = 0;
    BlockType Blocks::KELP_PLANT = 0;
    BlockType Blocks::DRIED_KELP_BLOCK = 0;
    BlockType Blocks::TURTLE_EGG = 0;
    BlockType Blocks::SNIFFER_EGG = 0;
    BlockType Blocks::DEAD_TUBE_CORAL_BLOCK = 0;
    BlockType Blocks::DEAD_BRAIN_CORAL_BLOCK = 0;
    BlockType Blocks::DEAD_BUBBLE_CORAL_BLOCK = 0;
    BlockType Blocks::DEAD_FIRE_CORAL_BLOCK = 0;
    BlockType Blocks::DEAD_HORN_CORAL_BLOCK = 0;
    BlockType Blocks::TUBE_CORAL_BLOCK = 0;
    BlockType Blocks::BRAIN_CORAL_BLOCK = 0;
    BlockType Blocks::BUBBLE_CORAL_BLOCK = 0;
    BlockType Blocks::FIRE_CORAL_BLOCK = 0;
    BlockType Blocks::HORN_CORAL_BLOCK = 0;
    BlockType Blocks::DEAD_TUBE_CORAL = 0;
    BlockType Blocks::DEAD_BRAIN_CORAL = 0;
    BlockType Blocks::DEAD_BUBBLE_CORAL = 0;
    BlockType Blocks::DEAD_FIRE_CORAL = 0;
    BlockType Blocks::DEAD_HORN_CORAL = 0;
    BlockType Blocks::TUBE_CORAL = 0;
    BlockType Blocks::BRAIN_CORAL = 0;
    BlockType Blocks::BUBBLE_CORAL = 0;
    BlockType Blocks::FIRE_CORAL = 0;
    BlockType Blocks::HORN_CORAL = 0;
    BlockType Blocks::DEAD_TUBE_CORAL_FAN = 0;
    BlockType Blocks::DEAD_BRAIN_CORAL_FAN = 0;
    BlockType Blocks::DEAD_BUBBLE_CORAL_FAN = 0;
    BlockType Blocks::DEAD_FIRE_CORAL_FAN = 0;
    BlockType Blocks::DEAD_HORN_CORAL_FAN = 0;
    BlockType Blocks::TUBE_CORAL_FAN = 0;
    BlockType Blocks::BRAIN_CORAL_FAN = 0;
    BlockType Blocks::BUBBLE_CORAL_FAN = 0;
    BlockType Blocks::FIRE_CORAL_FAN = 0;
    BlockType Blocks::HORN_CORAL_FAN = 0;
    BlockType Blocks::DEAD_TUBE_CORAL_WALL_FAN = 0;
    BlockType Blocks::DEAD_BRAIN_CORAL_WALL_FAN = 0;
    BlockType Blocks::DEAD_BUBBLE_CORAL_WALL_FAN = 0;
    BlockType Blocks::DEAD_FIRE_CORAL_WALL_FAN = 0;
    BlockType Blocks::DEAD_HORN_CORAL_WALL_FAN = 0;
    BlockType Blocks::TUBE_CORAL_WALL_FAN = 0;
    BlockType Blocks::BRAIN_CORAL_WALL_FAN = 0;
    BlockType Blocks::BUBBLE_CORAL_WALL_FAN = 0;
    BlockType Blocks::FIRE_CORAL_WALL_FAN = 0;
    BlockType Blocks::HORN_CORAL_WALL_FAN = 0;
    BlockType Blocks::SEA_PICKLE = 0;
    BlockType Blocks::BLUE_ICE = 0;
    BlockType Blocks::CONDUIT = 0;
    BlockType Blocks::BAMBOO_SAPLING = 0;
    BlockType Blocks::BAMBOO = 0;
    BlockType Blocks::POTTED_BAMBOO = 0;
    BlockType Blocks::VOID_AIR = 0;
    BlockType Blocks::CAVE_AIR = 0;
    BlockType Blocks::BUBBLE_COLUMN = 0;
    BlockType Blocks::POLISHED_GRANITE_STAIRS = 0;
    BlockType Blocks::SMOOTH_RED_SANDSTONE_STAIRS = 0;
    BlockType Blocks::MOSSY_STONE_BRICK_STAIRS = 0;
    BlockType Blocks::POLISHED_DIORITE_STAIRS = 0;
    BlockType Blocks::MOSSY_COBBLESTONE_STAIRS = 0;
    BlockType Blocks::END_STONE_BRICK_STAIRS = 0;
    BlockType Blocks::STONE_STAIRS = 0;
    BlockType Blocks::SMOOTH_SANDSTONE_STAIRS = 0;
    BlockType Blocks::SMOOTH_QUARTZ_STAIRS = 0;
    BlockType Blocks::GRANITE_STAIRS = 0;
    BlockType Blocks::ANDESITE_STAIRS = 0;
    BlockType Blocks::RED_NETHER_BRICK_STAIRS = 0;
    BlockType Blocks::POLISHED_ANDESITE_STAIRS = 0;
    BlockType Blocks::DIORITE_STAIRS = 0;
    BlockType Blocks::POLISHED_GRANITE_SLAB = 0;
    BlockType Blocks::SMOOTH_RED_SANDSTONE_SLAB = 0;
    BlockType Blocks::MOSSY_STONE_BRICK_SLAB = 0;
    BlockType Blocks::POLISHED_DIORITE_SLAB = 0;
    BlockType Blocks::MOSSY_COBBLESTONE_SLAB = 0;
    BlockType Blocks::END_STONE_BRICK_SLAB = 0;
    BlockType Blocks::SMOOTH_SANDSTONE_SLAB = 0;
    BlockType Blocks::SMOOTH_QUARTZ_SLAB = 0;
    BlockType Blocks::GRANITE_SLAB = 0;
    BlockType Blocks::ANDESITE_SLAB = 0;
    BlockType Blocks::RED_NETHER_BRICK_SLAB = 0;
    BlockType Blocks::POLISHED_ANDESITE_SLAB = 0;
    BlockType Blocks::DIORITE_SLAB = 0;
    BlockType Blocks::BRICK_WALL = 0;
    BlockType Blocks::PRISMARINE_WALL = 0;
    BlockType Blocks::RED_SANDSTONE_WALL = 0;
    BlockType Blocks::MOSSY_STONE_BRICK_WALL = 0;
    BlockType Blocks::GRANITE_WALL = 0;
    BlockType Blocks::STONE_BRICK_WALL = 0;
    BlockType Blocks::MUD_BRICK_WALL = 0;
    BlockType Blocks::NETHER_BRICK_WALL = 0;
    BlockType Blocks::ANDESITE_WALL = 0;
    BlockType Blocks::RED_NETHER_BRICK_WALL = 0;
    BlockType Blocks::SANDSTONE_WALL = 0;
    BlockType Blocks::END_STONE_BRICK_WALL = 0;
    BlockType Blocks::DIORITE_WALL = 0;
    BlockType Blocks::SCAFFOLDING = 0;
    BlockType Blocks::LOOM = 0;
    BlockType Blocks::BARREL = 0;
    BlockType Blocks::SMOKER = 0;
    BlockType Blocks::BLAST_FURNACE = 0;
    BlockType Blocks::CARTOGRAPHY_TABLE = 0;
    BlockType Blocks::FLETCHING_TABLE = 0;
    BlockType Blocks::GRINDSTONE = 0;
    BlockType Blocks::LECTERN = 0;
    BlockType Blocks::SMITHING_TABLE = 0;
    BlockType Blocks::STONECUTTER = 0;
    BlockType Blocks::BELL = 0;
    BlockType Blocks::LANTERN = 0;
    BlockType Blocks::SOUL_LANTERN = 0;
    BlockType Blocks::CAMPFIRE = 0;
    BlockType Blocks::SOUL_CAMPFIRE = 0;
    BlockType Blocks::SWEET_BERRY_BUSH = 0;
    BlockType Blocks::WARPED_STEM = 0;
    BlockType Blocks::STRIPPED_WARPED_STEM = 0;
    BlockType Blocks::WARPED_HYPHAE = 0;
    BlockType Blocks::STRIPPED_WARPED_HYPHAE = 0;
    BlockType Blocks::WARPED_NYLIUM = 0;
    BlockType Blocks::WARPED_FUNGUS = 0;
    BlockType Blocks::WARPED_WART_BLOCK = 0;
    BlockType Blocks::WARPED_ROOTS = 0;
    BlockType Blocks::NETHER_SPROUTS = 0;
    BlockType Blocks::CRIMSON_STEM = 0;
    BlockType Blocks::STRIPPED_CRIMSON_STEM = 0;
    BlockType Blocks::CRIMSON_HYPHAE = 0;
    BlockType Blocks::STRIPPED_CRIMSON_HYPHAE = 0;
    BlockType Blocks::CRIMSON_NYLIUM = 0;
    BlockType Blocks::CRIMSON_FUNGUS = 0;
    BlockType Blocks::SHROOMLIGHT = 0;
    BlockType Blocks::WEEPING_VINES = 0;
    BlockType Blocks::WEEPING_VINES_PLANT = 0;
    BlockType Blocks::TWISTING_VINES = 0;
    BlockType Blocks::TWISTING_VINES_PLANT = 0;
    BlockType Blocks::CRIMSON_ROOTS = 0;
    BlockType Blocks::CRIMSON_PLANKS = 0;
    BlockType Blocks::WARPED_PLANKS = 0;
    BlockType Blocks::CRIMSON_SLAB = 0;
    BlockType Blocks::WARPED_SLAB = 0;
    BlockType Blocks::CRIMSON_PRESSURE_PLATE = 0;
    BlockType Blocks::WARPED_PRESSURE_PLATE = 0;
    BlockType Blocks::CRIMSON_FENCE = 0;
    BlockType Blocks::WARPED_FENCE = 0;
    BlockType Blocks::CRIMSON_TRAPDOOR = 0;
    BlockType Blocks::WARPED_TRAPDOOR = 0;
    BlockType Blocks::CRIMSON_FENCE_GATE = 0;
    BlockType Blocks::WARPED_FENCE_GATE = 0;
    BlockType Blocks::CRIMSON_STAIRS = 0;
    BlockType Blocks::WARPED_STAIRS = 0;
    BlockType Blocks::CRIMSON_BUTTON = 0;
    BlockType Blocks::WARPED_BUTTON = 0;
    BlockType Blocks::CRIMSON_DOOR = 0;
    BlockType Blocks::WARPED_DOOR = 0;
    BlockType Blocks::CRIMSON_SIGN = 0;
    BlockType Blocks::WARPED_SIGN = 0;
    BlockType Blocks::CRIMSON_WALL_SIGN = 0;
    BlockType Blocks::WARPED_WALL_SIGN = 0;
    BlockType Blocks::STRUCTURE_BLOCK = 0;
    BlockType Blocks::JIGSAW = 0;
    BlockType Blocks::TEST_BLOCK = 0;
    BlockType Blocks::TEST_INSTANCE_BLOCK = 0;
    BlockType Blocks::COMPOSTER = 0;
    BlockType Blocks::TARGET = 0;
    BlockType Blocks::BEE_NEST = 0;
    BlockType Blocks::BEEHIVE = 0;
    BlockType Blocks::HONEY_BLOCK = 0;
    BlockType Blocks::HONEYCOMB_BLOCK = 0;
    BlockType Blocks::NETHERITE_BLOCK = 0;
    BlockType Blocks::ANCIENT_DEBRIS = 0;
    BlockType Blocks::CRYING_OBSIDIAN = 0;
    BlockType Blocks::RESPAWN_ANCHOR = 0;
    BlockType Blocks::POTTED_CRIMSON_FUNGUS = 0;
    BlockType Blocks::POTTED_WARPED_FUNGUS = 0;
    BlockType Blocks::POTTED_CRIMSON_ROOTS = 0;
    BlockType Blocks::POTTED_WARPED_ROOTS = 0;
    BlockType Blocks::LODESTONE = 0;
    BlockType Blocks::BLACKSTONE = 0;
    BlockType Blocks::BLACKSTONE_STAIRS = 0;
    BlockType Blocks::BLACKSTONE_WALL = 0;
    BlockType Blocks::BLACKSTONE_SLAB = 0;
    BlockType Blocks::POLISHED_BLACKSTONE = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_BRICKS = 0;
    BlockType Blocks::CRACKED_POLISHED_BLACKSTONE_BRICKS = 0;
    BlockType Blocks::CHISELED_POLISHED_BLACKSTONE = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_BRICK_SLAB = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_BRICK_STAIRS = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_BRICK_WALL = 0;
    BlockType Blocks::GILDED_BLACKSTONE = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_STAIRS = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_SLAB = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_PRESSURE_PLATE = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_BUTTON = 0;
    BlockType Blocks::POLISHED_BLACKSTONE_WALL = 0;
    BlockType Blocks::CHISELED_NETHER_BRICKS = 0;
    BlockType Blocks::CRACKED_NETHER_BRICKS = 0;
    BlockType Blocks::QUARTZ_BRICKS = 0;
    BlockType Blocks::CANDLE = 0;
    BlockType Blocks::WHITE_CANDLE = 0;
    BlockType Blocks::ORANGE_CANDLE = 0;
    BlockType Blocks::MAGENTA_CANDLE = 0;
    BlockType Blocks::LIGHT_BLUE_CANDLE = 0;
    BlockType Blocks::YELLOW_CANDLE = 0;
    BlockType Blocks::LIME_CANDLE = 0;
    BlockType Blocks::PINK_CANDLE = 0;
    BlockType Blocks::GRAY_CANDLE = 0;
    BlockType Blocks::LIGHT_GRAY_CANDLE = 0;
    BlockType Blocks::CYAN_CANDLE = 0;
    BlockType Blocks::PURPLE_CANDLE = 0;
    BlockType Blocks::BLUE_CANDLE = 0;
    BlockType Blocks::BROWN_CANDLE = 0;
    BlockType Blocks::GREEN_CANDLE = 0;
    BlockType Blocks::RED_CANDLE = 0;
    BlockType Blocks::BLACK_CANDLE = 0;
    BlockType Blocks::CANDLE_CAKE = 0;
    BlockType Blocks::WHITE_CANDLE_CAKE = 0;
    BlockType Blocks::ORANGE_CANDLE_CAKE = 0;
    BlockType Blocks::MAGENTA_CANDLE_CAKE = 0;
    BlockType Blocks::LIGHT_BLUE_CANDLE_CAKE = 0;
    BlockType Blocks::YELLOW_CANDLE_CAKE = 0;
    BlockType Blocks::LIME_CANDLE_CAKE = 0;
    BlockType Blocks::PINK_CANDLE_CAKE = 0;
    BlockType Blocks::GRAY_CANDLE_CAKE = 0;
    BlockType Blocks::LIGHT_GRAY_CANDLE_CAKE = 0;
    BlockType Blocks::CYAN_CANDLE_CAKE = 0;
    BlockType Blocks::PURPLE_CANDLE_CAKE = 0;
    BlockType Blocks::BLUE_CANDLE_CAKE = 0;
    BlockType Blocks::BROWN_CANDLE_CAKE = 0;
    BlockType Blocks::GREEN_CANDLE_CAKE = 0;
    BlockType Blocks::RED_CANDLE_CAKE = 0;
    BlockType Blocks::BLACK_CANDLE_CAKE = 0;
    BlockType Blocks::AMETHYST_BLOCK = 0;
    BlockType Blocks::BUDDING_AMETHYST = 0;
    BlockType Blocks::AMETHYST_CLUSTER = 0;
    BlockType Blocks::LARGE_AMETHYST_BUD = 0;
    BlockType Blocks::MEDIUM_AMETHYST_BUD = 0;
    BlockType Blocks::SMALL_AMETHYST_BUD = 0;
    BlockType Blocks::TUFF = 0;
    BlockType Blocks::TUFF_SLAB = 0;
    BlockType Blocks::TUFF_STAIRS = 0;
    BlockType Blocks::TUFF_WALL = 0;
    BlockType Blocks::POLISHED_TUFF = 0;
    BlockType Blocks::POLISHED_TUFF_SLAB = 0;
    BlockType Blocks::POLISHED_TUFF_STAIRS = 0;
    BlockType Blocks::POLISHED_TUFF_WALL = 0;
    BlockType Blocks::CHISELED_TUFF = 0;
    BlockType Blocks::TUFF_BRICKS = 0;
    BlockType Blocks::TUFF_BRICK_SLAB = 0;
    BlockType Blocks::TUFF_BRICK_STAIRS = 0;
    BlockType Blocks::TUFF_BRICK_WALL = 0;
    BlockType Blocks::CHISELED_TUFF_BRICKS = 0;
    BlockType Blocks::CALCITE = 0;
    BlockType Blocks::TINTED_GLASS = 0;
    BlockType Blocks::POWDER_SNOW = 0;
    BlockType Blocks::SCULK_SENSOR = 0;
    BlockType Blocks::CALIBRATED_SCULK_SENSOR = 0;
    BlockType Blocks::SCULK = 0;
    BlockType Blocks::SCULK_VEIN = 0;
    BlockType Blocks::SCULK_CATALYST = 0;
    BlockType Blocks::SCULK_SHRIEKER = 0;
    BlockType Blocks::COPPER_BLOCK = 0;
    BlockType Blocks::EXPOSED_COPPER = 0;
    BlockType Blocks::WEATHERED_COPPER = 0;
    BlockType Blocks::OXIDIZED_COPPER = 0;
    BlockType Blocks::COPPER_ORE = 0;
    BlockType Blocks::DEEPSLATE_COPPER_ORE = 0;
    BlockType Blocks::OXIDIZED_CUT_COPPER = 0;
    BlockType Blocks::WEATHERED_CUT_COPPER = 0;
    BlockType Blocks::EXPOSED_CUT_COPPER = 0;
    BlockType Blocks::CUT_COPPER = 0;
    BlockType Blocks::OXIDIZED_CHISELED_COPPER = 0;
    BlockType Blocks::WEATHERED_CHISELED_COPPER = 0;
    BlockType Blocks::EXPOSED_CHISELED_COPPER = 0;
    BlockType Blocks::CHISELED_COPPER = 0;
    BlockType Blocks::WAXED_OXIDIZED_CHISELED_COPPER = 0;
    BlockType Blocks::WAXED_WEATHERED_CHISELED_COPPER = 0;
    BlockType Blocks::WAXED_EXPOSED_CHISELED_COPPER = 0;
    BlockType Blocks::WAXED_CHISELED_COPPER = 0;
    BlockType Blocks::OXIDIZED_CUT_COPPER_STAIRS = 0;
    BlockType Blocks::WEATHERED_CUT_COPPER_STAIRS = 0;
    BlockType Blocks::EXPOSED_CUT_COPPER_STAIRS = 0;
    BlockType Blocks::CUT_COPPER_STAIRS = 0;
    BlockType Blocks::OXIDIZED_CUT_COPPER_SLAB = 0;
    BlockType Blocks::WEATHERED_CUT_COPPER_SLAB = 0;
    BlockType Blocks::EXPOSED_CUT_COPPER_SLAB = 0;
    BlockType Blocks::CUT_COPPER_SLAB = 0;
    BlockType Blocks::WAXED_COPPER_BLOCK = 0;
    BlockType Blocks::WAXED_WEATHERED_COPPER = 0;
    BlockType Blocks::WAXED_EXPOSED_COPPER = 0;
    BlockType Blocks::WAXED_OXIDIZED_COPPER = 0;
    BlockType Blocks::WAXED_OXIDIZED_CUT_COPPER = 0;
    BlockType Blocks::WAXED_WEATHERED_CUT_COPPER = 0;
    BlockType Blocks::WAXED_EXPOSED_CUT_COPPER = 0;
    BlockType Blocks::WAXED_CUT_COPPER = 0;
    BlockType Blocks::WAXED_OXIDIZED_CUT_COPPER_STAIRS = 0;
    BlockType Blocks::WAXED_WEATHERED_CUT_COPPER_STAIRS = 0;
    BlockType Blocks::WAXED_EXPOSED_CUT_COPPER_STAIRS = 0;
    BlockType Blocks::WAXED_CUT_COPPER_STAIRS = 0;
    BlockType Blocks::WAXED_OXIDIZED_CUT_COPPER_SLAB = 0;
    BlockType Blocks::WAXED_WEATHERED_CUT_COPPER_SLAB = 0;
    BlockType Blocks::WAXED_EXPOSED_CUT_COPPER_SLAB = 0;
    BlockType Blocks::WAXED_CUT_COPPER_SLAB = 0;
    BlockType Blocks::COPPER_DOOR = 0;
    BlockType Blocks::EXPOSED_COPPER_DOOR = 0;
    BlockType Blocks::OXIDIZED_COPPER_DOOR = 0;
    BlockType Blocks::WEATHERED_COPPER_DOOR = 0;
    BlockType Blocks::WAXED_COPPER_DOOR = 0;
    BlockType Blocks::WAXED_EXPOSED_COPPER_DOOR = 0;
    BlockType Blocks::WAXED_OXIDIZED_COPPER_DOOR = 0;
    BlockType Blocks::WAXED_WEATHERED_COPPER_DOOR = 0;
    BlockType Blocks::COPPER_TRAPDOOR = 0;
    BlockType Blocks::EXPOSED_COPPER_TRAPDOOR = 0;
    BlockType Blocks::OXIDIZED_COPPER_TRAPDOOR = 0;
    BlockType Blocks::WEATHERED_COPPER_TRAPDOOR = 0;
    BlockType Blocks::WAXED_COPPER_TRAPDOOR = 0;
    BlockType Blocks::WAXED_EXPOSED_COPPER_TRAPDOOR = 0;
    BlockType Blocks::WAXED_OXIDIZED_COPPER_TRAPDOOR = 0;
    BlockType Blocks::WAXED_WEATHERED_COPPER_TRAPDOOR = 0;
    BlockType Blocks::COPPER_GRATE = 0;
    BlockType Blocks::EXPOSED_COPPER_GRATE = 0;
    BlockType Blocks::WEATHERED_COPPER_GRATE = 0;
    BlockType Blocks::OXIDIZED_COPPER_GRATE = 0;
    BlockType Blocks::WAXED_COPPER_GRATE = 0;
    BlockType Blocks::WAXED_EXPOSED_COPPER_GRATE = 0;
    BlockType Blocks::WAXED_WEATHERED_COPPER_GRATE = 0;
    BlockType Blocks::WAXED_OXIDIZED_COPPER_GRATE = 0;
    BlockType Blocks::COPPER_BULB = 0;
    BlockType Blocks::EXPOSED_COPPER_BULB = 0;
    BlockType Blocks::WEATHERED_COPPER_BULB = 0;
    BlockType Blocks::OXIDIZED_COPPER_BULB = 0;
    BlockType Blocks::WAXED_COPPER_BULB = 0;
    BlockType Blocks::WAXED_EXPOSED_COPPER_BULB = 0;
    BlockType Blocks::WAXED_WEATHERED_COPPER_BULB = 0;
    BlockType Blocks::WAXED_OXIDIZED_COPPER_BULB = 0;
    BlockType Blocks::LIGHTNING_ROD = 0;
    BlockType Blocks::POINTED_DRIPSTONE = 0;
    BlockType Blocks::DRIPSTONE_BLOCK = 0;
    BlockType Blocks::CAVE_VINES = 0;
    BlockType Blocks::CAVE_VINES_PLANT = 0;
    BlockType Blocks::SPORE_BLOSSOM = 0;
    BlockType Blocks::AZALEA = 0;
    BlockType Blocks::FLOWERING_AZALEA = 0;
    BlockType Blocks::MOSS_CARPET = 0;
    BlockType Blocks::PINK_PETALS = 0;
    BlockType Blocks::WILDFLOWERS = 0;
    BlockType Blocks::LEAF_LITTER = 0;
    BlockType Blocks::MOSS_BLOCK = 0;
    BlockType Blocks::BIG_DRIPLEAF = 0;
    BlockType Blocks::BIG_DRIPLEAF_STEM = 0;
    BlockType Blocks::SMALL_DRIPLEAF = 0;
    BlockType Blocks::HANGING_ROOTS = 0;
    BlockType Blocks::ROOTED_DIRT = 0;
    BlockType Blocks::MUD = 0;
    BlockType Blocks::DEEPSLATE = 0;
    BlockType Blocks::COBBLED_DEEPSLATE = 0;
    BlockType Blocks::COBBLED_DEEPSLATE_STAIRS = 0;
    BlockType Blocks::COBBLED_DEEPSLATE_SLAB = 0;
    BlockType Blocks::COBBLED_DEEPSLATE_WALL = 0;
    BlockType Blocks::POLISHED_DEEPSLATE = 0;
    BlockType Blocks::POLISHED_DEEPSLATE_STAIRS = 0;
    BlockType Blocks::POLISHED_DEEPSLATE_SLAB = 0;
    BlockType Blocks::POLISHED_DEEPSLATE_WALL = 0;
    BlockType Blocks::DEEPSLATE_TILES = 0;
    BlockType Blocks::DEEPSLATE_TILE_STAIRS = 0;
    BlockType Blocks::DEEPSLATE_TILE_SLAB = 0;
    BlockType Blocks::DEEPSLATE_TILE_WALL = 0;
    BlockType Blocks::DEEPSLATE_BRICKS = 0;
    BlockType Blocks::DEEPSLATE_BRICK_STAIRS = 0;
    BlockType Blocks::DEEPSLATE_BRICK_SLAB = 0;
    BlockType Blocks::DEEPSLATE_BRICK_WALL = 0;
    BlockType Blocks::CHISELED_DEEPSLATE = 0;
    BlockType Blocks::CRACKED_DEEPSLATE_BRICKS = 0;
    BlockType Blocks::CRACKED_DEEPSLATE_TILES = 0;
    BlockType Blocks::INFESTED_DEEPSLATE = 0;
    BlockType Blocks::SMOOTH_BASALT = 0;
    BlockType Blocks::RAW_IRON_BLOCK = 0;
    BlockType Blocks::RAW_COPPER_BLOCK = 0;
    BlockType Blocks::RAW_GOLD_BLOCK = 0;
    BlockType Blocks::POTTED_AZALEA_BUSH = 0;
    BlockType Blocks::POTTED_FLOWERING_AZALEA_BUSH = 0;
    BlockType Blocks::OCHRE_FROGLIGHT = 0;
    BlockType Blocks::VERDANT_FROGLIGHT = 0;
    BlockType Blocks::PEARLESCENT_FROGLIGHT = 0;
    BlockType Blocks::FROGSPAWN = 0;
    BlockType Blocks::REINFORCED_DEEPSLATE = 0;
    BlockType Blocks::DECORATED_POT = 0;
    BlockType Blocks::CRAFTER = 0;
    BlockType Blocks::TRIAL_SPAWNER = 0;
    BlockType Blocks::VAULT = 0;
    BlockType Blocks::HEAVY_CORE = 0;
    BlockType Blocks::PALE_MOSS_BLOCK = 0;
    BlockType Blocks::PALE_MOSS_CARPET = 0;
    BlockType Blocks::PALE_HANGING_MOSS = 0;
    BlockType Blocks::OPEN_EYEBLOSSOM = 0;
    BlockType Blocks::CLOSED_EYEBLOSSOM = 0;
    BlockType Blocks::POTTED_OPEN_EYEBLOSSOM = 0;
    BlockType Blocks::POTTED_CLOSED_EYEBLOSSOM = 0;
    BlockType Blocks::FIREFLY_BUSH = 0;

    BlockType Blocks::registerBlock(const std::string& id, const BlockSettings& settings,
                                    std::unique_ptr<BlockBehavior> behavior)
    {
        auto block = registerBlockInternal(id, settings, std::move(behavior));
        return block->getBlockType();
    }

    BlockDefPtr Blocks::registerBlockInternal(const std::string& id, const BlockSettings& settings,
                                              std::unique_ptr<BlockBehavior> behavior)
    {
        size_t index = blocks_.size();

        auto block = std::make_shared<BlockDefinition>(id, settings, std::move(behavior));
        block->setBlockType(static_cast<BlockType>(index));

        blocks_.push_back(block);
        idToIndex_[id] = index;

        LOG_DEBUG("Registered block '%s' with type %d", id.c_str(), static_cast<int>(index));

        return block;
    }

    BlockDefPtr Blocks::getBlock(BlockType type)
    {
        size_t index = static_cast<size_t>(type);
        if (index < blocks_.size())
        {
            return blocks_[index];
        }
        return nullptr;
    }

    BlockDefPtr Blocks::getBlock(const std::string& id)
    {
        auto it = idToIndex_.find(id);
        if (it != idToIndex_.end())
        {
            return blocks_[it->second];
        }
        return nullptr;
    }

    RenderLayer Blocks::getRenderLayer(BlockType type)
    {
        auto block = getBlock(type);
        return block ? block->getRenderLayer() : RenderLayer::OPAQUE;
    }

    const BlockBehavior* Blocks::getBehavior(BlockType type)
    {
        auto block = getBlock(type);
        return block ? block->getBehavior() : nullptr;
    }

    bool Blocks::hasCollision(BlockType type)
    {
        auto block = getBlock(type);
        return block ? block->hasCollision() : true; // Default to collision
    }

    bool Blocks::hasCollisionAt(BlockType type, const glm::vec3& position, const AABB& entityAABB)
    {
        auto block = getBlock(type);
        return block ? block->hasCollisionAt(position, entityAABB) : true; // Default to collision
    }

    void Blocks::initialize()
    {
        if (initialized_)
        {
            LOG_WARN("Blocks already initialized, skipping");
            return;
        }

        LOG_INFO("Initializing blocks system...");

        // Basic blocks
        AIR = registerBlock("air", BlockSettings::create().material(BlockMaterial::AIR));
        STONE = registerBlock("stone", BlockSettings::create().material(BlockMaterial::STONE));
        GRANITE = registerBlock("granite", BlockSettings::create().material(BlockMaterial::STONE));
        POLISHED_GRANITE = registerBlock("polished_granite", BlockSettings::create().material(BlockMaterial::STONE));
        DIORITE = registerBlock("diorite", BlockSettings::create().material(BlockMaterial::STONE));
        POLISHED_DIORITE = registerBlock("polished_diorite", BlockSettings::create().material(BlockMaterial::STONE));
        ANDESITE = registerBlock("andesite", BlockSettings::create().material(BlockMaterial::STONE));
        POLISHED_ANDESITE = registerBlock("polished_andesite", BlockSettings::create().material(BlockMaterial::STONE));
        GRASS_BLOCK = registerBlock("grass_block", BlockSettings::create().material(BlockMaterial::SOLID));
        DIRT = registerBlock("dirt", BlockSettings::create().material(BlockMaterial::SOLID));
        COARSE_DIRT = registerBlock("coarse_dirt", BlockSettings::create().material(BlockMaterial::SOLID));
        PODZOL = registerBlock("podzol", BlockSettings::create().material(BlockMaterial::SOLID));
        COBBLESTONE = registerBlock("cobblestone", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Wood planks
        OAK_PLANKS = registerBlock("oak_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        SPRUCE_PLANKS = registerBlock("spruce_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        BIRCH_PLANKS = registerBlock("birch_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        JUNGLE_PLANKS = registerBlock("jungle_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        ACACIA_PLANKS = registerBlock("acacia_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        CHERRY_PLANKS = registerBlock("cherry_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        DARK_OAK_PLANKS = registerBlock("dark_oak_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        PALE_OAK_WOOD = registerBlock("pale_oak_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        PALE_OAK_PLANKS = registerBlock("pale_oak_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        MANGROVE_PLANKS = registerBlock("mangrove_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        BAMBOO_PLANKS = registerBlock("bamboo_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        BAMBOO_MOSAIC = registerBlock("bamboo_mosaic", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Saplings
        OAK_SAPLING = registerBlock("oak_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SPRUCE_SAPLING = registerBlock("spruce_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BIRCH_SAPLING = registerBlock("birch_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        JUNGLE_SAPLING = registerBlock("jungle_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ACACIA_SAPLING = registerBlock("acacia_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CHERRY_SAPLING = registerBlock("cherry_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DARK_OAK_SAPLING = registerBlock("dark_oak_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PALE_OAK_SAPLING = registerBlock("pale_oak_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        MANGROVE_PROPAGULE = registerBlock("mangrove_propagule", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Basic terrain blocks
        BEDROCK = registerBlock("bedrock", BlockSettings::create().material(BlockMaterial::STONE));
        SAND = registerBlock("sand", BlockSettings::create().material(BlockMaterial::SOLID));
        SUSPICIOUS_SAND = registerBlock("suspicious_sand", BlockSettings::create().material(BlockMaterial::SOLID));
        RED_SAND = registerBlock("red_sand", BlockSettings::create().material(BlockMaterial::SOLID));
        GRAVEL = registerBlock("gravel", BlockSettings::create().material(BlockMaterial::SOLID));
        SUSPICIOUS_GRAVEL = registerBlock("suspicious_gravel", BlockSettings::create().material(BlockMaterial::SOLID));
        
        // Ores
        GOLD_ORE = registerBlock("gold_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DEEPSLATE_GOLD_ORE = registerBlock("deepslate_gold_ore", BlockSettings::create().material(BlockMaterial::STONE));
        IRON_ORE = registerBlock("iron_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DEEPSLATE_IRON_ORE = registerBlock("deepslate_iron_ore", BlockSettings::create().material(BlockMaterial::STONE));
        COAL_ORE =  registerBlock("coal_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DEEPSLATE_COAL_ORE = registerBlock("deepslate_coal_ore", BlockSettings::create().material(BlockMaterial::STONE));
        NETHER_GOLD_ORE = registerBlock("nether_gold_ore", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Logs
        OAK_LOG = registerBlock("oak_log", BlockSettings::create().material(BlockMaterial::WOOD));
        SPRUCE_LOG = registerBlock("spruce_log", BlockSettings::create().material(BlockMaterial::WOOD));
        BIRCH_LOG = registerBlock("birch_log", BlockSettings::create().material(BlockMaterial::WOOD));
        JUNGLE_LOG = registerBlock("jungle_log", BlockSettings::create().material(BlockMaterial::WOOD));
        ACACIA_LOG = registerBlock("acacia_log", BlockSettings::create().material(BlockMaterial::WOOD));
        CHERRY_LOG = registerBlock("cherry_log", BlockSettings::create().material(BlockMaterial::WOOD));
        DARK_OAK_LOG = registerBlock("dark_oak_log", BlockSettings::create().material(BlockMaterial::WOOD));
        PALE_OAK_LOG = registerBlock("pale_oak_log", BlockSettings::create().material(BlockMaterial::WOOD));
        MANGROVE_LOG = registerBlock("mangrove_log", BlockSettings::create().material(BlockMaterial::WOOD));
        MANGROVE_ROOTS = registerBlock("mangrove_roots", BlockSettings::create().material(BlockMaterial::WOOD));
        MUDDY_MANGROVE_ROOTS = registerBlock("muddy_mangrove_roots", BlockSettings::create().material(BlockMaterial::WOOD));
        BAMBOO_BLOCK = registerBlock("bamboo_block", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Stripped logs
        STRIPPED_SPRUCE_LOG = registerBlock("stripped_spruce_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_BIRCH_LOG = registerBlock("stripped_birch_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_JUNGLE_LOG = registerBlock("stripped_jungle_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_ACACIA_LOG = registerBlock("stripped_acacia_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_CHERRY_LOG = registerBlock("stripped_cherry_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_DARK_OAK_LOG = registerBlock("stripped_dark_oak_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_PALE_OAK_LOG = registerBlock("stripped_pale_oak_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_OAK_LOG = registerBlock("stripped_oak_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_MANGROVE_LOG = registerBlock("stripped_mangrove_log", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_BAMBOO_BLOCK = registerBlock("stripped_bamboo_block", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Wood blocks
        OAK_WOOD = registerBlock("oak_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        SPRUCE_WOOD = registerBlock("spruce_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        BIRCH_WOOD = registerBlock("birch_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        JUNGLE_WOOD = registerBlock("jungle_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        ACACIA_WOOD = registerBlock("acacia_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        CHERRY_WOOD = registerBlock("cherry_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        DARK_OAK_WOOD = registerBlock("dark_oak_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        MANGROVE_WOOD = registerBlock("mangrove_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_OAK_WOOD = registerBlock("stripped_oak_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_SPRUCE_WOOD = registerBlock("stripped_spruce_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_BIRCH_WOOD = registerBlock("stripped_birch_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_JUNGLE_WOOD = registerBlock("stripped_jungle_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_ACACIA_WOOD = registerBlock("stripped_acacia_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_CHERRY_WOOD = registerBlock("stripped_cherry_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_DARK_OAK_WOOD = registerBlock("stripped_dark_oak_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_PALE_OAK_WOOD = registerBlock("stripped_pale_oak_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        STRIPPED_MANGROVE_WOOD = registerBlock("stripped_mangrove_wood", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Leaves
        OAK_LEAVES = registerBlock("oak_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        SPRUCE_LEAVES = registerBlock("spruce_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        BIRCH_LEAVES = registerBlock("birch_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        JUNGLE_LEAVES = registerBlock("jungle_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        ACACIA_LEAVES = registerBlock("acacia_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        CHERRY_LEAVES = registerBlock("cherry_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        DARK_OAK_LEAVES = registerBlock("dark_oak_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        PALE_OAK_LEAVES = registerBlock("pale_oak_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        MANGROVE_LEAVES = registerBlock("mangrove_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        AZALEA_LEAVES = registerBlock("azalea_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        FLOWERING_AZALEA_LEAVES = registerBlock("flowering_azalea_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        
        // Utility blocks
        SPONGE = registerBlock("sponge", BlockSettings::create().material(BlockMaterial::SOLID));
        WET_SPONGE = registerBlock("wet_sponge", BlockSettings::create().material(BlockMaterial::SOLID));
        GLASS = registerBlock("glass", BlockSettings::create().material(BlockMaterial::GLASS));
        LAPIS_ORE = registerBlock("lapis_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DEEPSLATE_LAPIS_ORE = registerBlock("deepslate_lapis_ore", BlockSettings::create().material(BlockMaterial::STONE));
        LAPIS_BLOCK = registerBlock("lapis_block", BlockSettings::create().material(BlockMaterial::STONE));
        DISPENSER = registerBlock("dispenser", BlockSettings::create().material(BlockMaterial::STONE));
        SANDSTONE = registerBlock("sandstone", BlockSettings::create().material(BlockMaterial::STONE));
        CHISELED_SANDSTONE = registerBlock("chiseled_sandstone", BlockSettings::create().material(BlockMaterial::STONE));
        CUT_SANDSTONE = registerBlock("cut_sandstone", BlockSettings::create().material(BlockMaterial::STONE));
        NOTE_BLOCK = registerBlock("note_block", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Beds
        WHITE_BED = registerBlock("white_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        ORANGE_BED = registerBlock("orange_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        MAGENTA_BED = registerBlock("magenta_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        LIGHT_BLUE_BED = registerBlock("light_blue_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        YELLOW_BED = registerBlock("yellow_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        LIME_BED = registerBlock("lime_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        PINK_BED = registerBlock("pink_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        GRAY_BED = registerBlock("gray_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        LIGHT_GRAY_BED = registerBlock("light_gray_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        CYAN_BED = registerBlock("cyan_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        PURPLE_BED = registerBlock("purple_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        BLUE_BED = registerBlock("blue_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        BROWN_BED = registerBlock("brown_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        GREEN_BED = registerBlock("green_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        RED_BED = registerBlock("red_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        BLACK_BED = registerBlock("black_bed", BlockSettings::create().material(BlockMaterial::SOLID));
        
        // Rails and redstone
        POWERED_RAIL = registerBlock("powered_rail", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DETECTOR_RAIL = registerBlock("detector_rail", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        STICKY_PISTON = registerBlock("sticky_piston", BlockSettings::create().material(BlockMaterial::STONE));
        COBWEB = registerBlock("cobweb", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Plants
        SHORT_GRASS = registerBlock("short_grass", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        FERN = registerBlock("fern", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_BUSH = registerBlock("dead_bush", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BUSH = registerBlock("bush", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SHORT_DRY_GRASS = registerBlock("short_dry_grass", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        TALL_DRY_GRASS = registerBlock("tall_dry_grass", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SEAGRASS = registerBlock("seagrass", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        TALL_SEAGRASS = registerBlock("tall_seagrass", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Pistons and mechanisms
        PISTON = registerBlock("piston", BlockSettings::create().material(BlockMaterial::STONE));
        PISTON_HEAD = registerBlock("piston_head", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Wool blocks
        WHITE_WOOL = registerBlock("white_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        ORANGE_WOOL = registerBlock("orange_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        MAGENTA_WOOL = registerBlock("magenta_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        LIGHT_BLUE_WOOL = registerBlock("light_blue_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        YELLOW_WOOL = registerBlock("yellow_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        LIME_WOOL = registerBlock("lime_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        PINK_WOOL = registerBlock("pink_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        GRAY_WOOL = registerBlock("gray_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        LIGHT_GRAY_WOOL = registerBlock("light_gray_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        CYAN_WOOL = registerBlock("cyan_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        PURPLE_WOOL = registerBlock("purple_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        BLUE_WOOL = registerBlock("blue_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        BROWN_WOOL = registerBlock("brown_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        GREEN_WOOL = registerBlock("green_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        RED_WOOL = registerBlock("red_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        BLACK_WOOL = registerBlock("black_wool", BlockSettings::create().material(BlockMaterial::SOLID));
        
        // Special blocks
        MOVING_PISTON = registerBlock("moving_piston", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Flowers
        DANDELION = registerBlock("dandelion", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        TORCHFLOWER = registerBlock("torchflower", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POPPY = registerBlock("poppy", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BLUE_ORCHID = registerBlock("blue_orchid", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ALLIUM = registerBlock("allium", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        AZURE_BLUET = registerBlock("azure_bluet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        RED_TULIP = registerBlock("red_tulip", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ORANGE_TULIP = registerBlock("orange_tulip", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        WHITE_TULIP = registerBlock("white_tulip", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PINK_TULIP = registerBlock("pink_tulip", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        OXEYE_DAISY = registerBlock("oxeye_daisy", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CORNFLOWER = registerBlock("cornflower", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        WITHER_ROSE = registerBlock("wither_rose", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LILY_OF_THE_VALLEY = registerBlock("lily_of_the_valley", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BROWN_MUSHROOM = registerBlock("brown_mushroom", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        RED_MUSHROOM = registerBlock("red_mushroom", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Resource blocks
        GOLD_BLOCK = registerBlock("gold_block", BlockSettings::create().material(BlockMaterial::STONE));
        IRON_BLOCK = registerBlock("iron_block", BlockSettings::create().material(BlockMaterial::STONE));
        BRICKS = registerBlock("bricks", BlockSettings::create().material(BlockMaterial::STONE));
        TNT = registerBlock("tnt", BlockSettings::create().material(BlockMaterial::SOLID));
        BOOKSHELF = registerBlock("bookshelf", BlockSettings::create().material(BlockMaterial::WOOD));
        CHISELED_BOOKSHELF = registerBlock("chiseled_bookshelf", BlockSettings::create().material(BlockMaterial::WOOD));
        MOSSY_COBBLESTONE = registerBlock("mossy_cobblestone", BlockSettings::create().material(BlockMaterial::STONE));
        OBSIDIAN = registerBlock("obsidian", BlockSettings::create().material(BlockMaterial::STONE));
        TORCH = registerBlock("torch", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        WALL_TORCH = registerBlock("wall_torch", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        FIRE = registerBlock("fire", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SOUL_FIRE = registerBlock("soul_fire", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SPAWNER = registerBlock("spawner", BlockSettings::create().material(BlockMaterial::STONE));
        CREAKING_HEART = registerBlock("creaking_heart", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Stairs
        OAK_STAIRS = registerBlock("oak_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        
        // Utility blocks
        CHEST = registerBlock("chest", BlockSettings::create().material(BlockMaterial::WOOD));
        REDSTONE_WIRE = registerBlock("redstone_wire", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DIAMOND_ORE = registerBlock("diamond_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DEEPSLATE_DIAMOND_ORE = registerBlock("deepslate_diamond_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DIAMOND_BLOCK = registerBlock("diamond_block", BlockSettings::create().material(BlockMaterial::STONE));
        CRAFTING_TABLE = registerBlock("crafting_table", BlockSettings::create().material(BlockMaterial::WOOD));
        WHEAT = registerBlock("wheat", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        FARMLAND = registerBlock("farmland", BlockSettings::create().material(BlockMaterial::SOLID));
        FURNACE = registerBlock("furnace", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Signs
        OAK_SIGN = registerBlock("oak_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        SPRUCE_SIGN = registerBlock("spruce_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BIRCH_SIGN = registerBlock("birch_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        ACACIA_SIGN = registerBlock("acacia_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        CHERRY_SIGN = registerBlock("cherry_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        JUNGLE_SIGN = registerBlock("jungle_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        DARK_OAK_SIGN = registerBlock("dark_oak_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        PALE_OAK_SIGN = registerBlock("pale_oak_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        MANGROVE_SIGN = registerBlock("mangrove_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BAMBOO_SIGN = registerBlock("bamboo_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        
        // Doors
        OAK_DOOR = registerBlock("oak_door", BlockSettings::create().material(BlockMaterial::WOOD));
        LADDER = registerBlock("ladder", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        RAIL = registerBlock("rail", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        COBBLESTONE_STAIRS = registerBlock("cobblestone_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        
        // Wall signs
        OAK_WALL_SIGN = registerBlock("oak_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        SPRUCE_WALL_SIGN = registerBlock("spruce_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BIRCH_WALL_SIGN = registerBlock("birch_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        ACACIA_WALL_SIGN = registerBlock("acacia_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        CHERRY_WALL_SIGN = registerBlock("cherry_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        JUNGLE_WALL_SIGN = registerBlock("jungle_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        DARK_OAK_WALL_SIGN = registerBlock("dark_oak_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        PALE_OAK_WALL_SIGN = registerBlock("pale_oak_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        MANGROVE_WALL_SIGN = registerBlock("mangrove_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BAMBOO_WALL_SIGN = registerBlock("bamboo_wall_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        
        // Hanging signs
        OAK_HANGING_SIGN = registerBlock("oak_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        SPRUCE_HANGING_SIGN = registerBlock("spruce_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BIRCH_HANGING_SIGN = registerBlock("birch_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        ACACIA_HANGING_SIGN = registerBlock("acacia_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        CHERRY_HANGING_SIGN = registerBlock("cherry_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        JUNGLE_HANGING_SIGN = registerBlock("jungle_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        DARK_OAK_HANGING_SIGN = registerBlock("dark_oak_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        PALE_OAK_HANGING_SIGN = registerBlock("pale_oak_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        CRIMSON_HANGING_SIGN = registerBlock("crimson_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        WARPED_HANGING_SIGN = registerBlock("warped_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        MANGROVE_HANGING_SIGN = registerBlock("mangrove_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BAMBOO_HANGING_SIGN = registerBlock("bamboo_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        
        // Wall hanging signs
        OAK_WALL_HANGING_SIGN = registerBlock("oak_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        SPRUCE_WALL_HANGING_SIGN = registerBlock("spruce_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BIRCH_WALL_HANGING_SIGN = registerBlock("birch_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        ACACIA_WALL_HANGING_SIGN = registerBlock("acacia_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        CHERRY_WALL_HANGING_SIGN = registerBlock("cherry_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        JUNGLE_WALL_HANGING_SIGN = registerBlock("jungle_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        DARK_OAK_WALL_HANGING_SIGN = registerBlock("dark_oak_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        PALE_OAK_WALL_HANGING_SIGN = registerBlock("pale_oak_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        MANGROVE_WALL_HANGING_SIGN = registerBlock("mangrove_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        CRIMSON_WALL_HANGING_SIGN = registerBlock("crimson_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        WARPED_WALL_HANGING_SIGN = registerBlock("warped_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BAMBOO_WALL_HANGING_SIGN = registerBlock("bamboo_wall_hanging_sign", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        
        // Pressure plates and buttons
        LEVER = registerBlock("lever", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        STONE_PRESSURE_PLATE = registerBlock("stone_pressure_plate", BlockSettings::create().material(BlockMaterial::STONE).noCollision());
        IRON_DOOR = registerBlock("iron_door", BlockSettings::create().material(BlockMaterial::STONE));
        OAK_PRESSURE_PLATE = registerBlock("oak_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        SPRUCE_PRESSURE_PLATE = registerBlock("spruce_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BIRCH_PRESSURE_PLATE = registerBlock("birch_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        JUNGLE_PRESSURE_PLATE = registerBlock("jungle_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        ACACIA_PRESSURE_PLATE = registerBlock("acacia_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        CHERRY_PRESSURE_PLATE = registerBlock("cherry_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        DARK_OAK_PRESSURE_PLATE = registerBlock("dark_oak_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        PALE_OAK_PRESSURE_PLATE = registerBlock("pale_oak_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        MANGROVE_PRESSURE_PLATE = registerBlock("mangrove_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BAMBOO_PRESSURE_PLATE = registerBlock("bamboo_pressure_plate", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        
        // Redstone blocks
        REDSTONE_ORE = registerBlock("redstone_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DEEPSLATE_REDSTONE_ORE = registerBlock("deepslate_redstone_ore", BlockSettings::create().material(BlockMaterial::STONE));
        REDSTONE_TORCH = registerBlock("redstone_torch", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        REDSTONE_WALL_TORCH = registerBlock("redstone_wall_torch", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        STONE_BUTTON = registerBlock("stone_button", BlockSettings::create().material(BlockMaterial::STONE).noCollision());
        
        // Ice and snow
        SNOW = registerBlock("snow", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ICE = registerBlock("ice", BlockSettings::create().material(BlockMaterial::GLASS));
        SNOW_BLOCK = registerBlock("snow_block", BlockSettings::create().material(BlockMaterial::SOLID));
        CACTUS = registerBlock("cactus", BlockSettings::create().material(BlockMaterial::SOLID));
        CACTUS_FLOWER = registerBlock("cactus_flower", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CLAY = registerBlock("clay", BlockSettings::create().material(BlockMaterial::SOLID));
        SUGAR_CANE = registerBlock("sugar_cane", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        JUKEBOX = registerBlock("jukebox", BlockSettings::create().material(BlockMaterial::WOOD));
        OAK_FENCE = registerBlock("oak_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Nether blocks
        NETHERRACK = registerBlock("netherrack", BlockSettings::create().material(BlockMaterial::STONE));
        SOUL_SAND = registerBlock("soul_sand", BlockSettings::create().material(BlockMaterial::SOLID));
        SOUL_SOIL = registerBlock("soul_soil", BlockSettings::create().material(BlockMaterial::SOLID));
        BASALT = registerBlock("basalt", BlockSettings::create().material(BlockMaterial::STONE));
        POLISHED_BASALT = registerBlock("polished_basalt", BlockSettings::create().material(BlockMaterial::STONE));
        SOUL_TORCH = registerBlock("soul_torch", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SOUL_WALL_TORCH = registerBlock("soul_wall_torch", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        GLOWSTONE = registerBlock("glowstone", BlockSettings::create().material(BlockMaterial::SOLID).transparent());
        NETHER_PORTAL = registerBlock("nether_portal", BlockSettings::create().material(BlockMaterial::SOLID).noCollision().transparent());
        CARVED_PUMPKIN = registerBlock("carved_pumpkin", BlockSettings::create().material(BlockMaterial::SOLID));
        JACK_O_LANTERN = registerBlock("jack_o_lantern", BlockSettings::create().material(BlockMaterial::SOLID));
        CAKE = registerBlock("cake", BlockSettings::create().material(BlockMaterial::SOLID));
        REPEATER = registerBlock("repeater", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Stained glass
        WHITE_STAINED_GLASS = registerBlock("white_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        ORANGE_STAINED_GLASS = registerBlock("orange_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        MAGENTA_STAINED_GLASS = registerBlock("magenta_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        LIGHT_BLUE_STAINED_GLASS = registerBlock("light_blue_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        YELLOW_STAINED_GLASS = registerBlock("yellow_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        LIME_STAINED_GLASS = registerBlock("lime_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        PINK_STAINED_GLASS = registerBlock("pink_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        GRAY_STAINED_GLASS = registerBlock("gray_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        LIGHT_GRAY_STAINED_GLASS = registerBlock("light_gray_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        CYAN_STAINED_GLASS = registerBlock("cyan_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        PURPLE_STAINED_GLASS = registerBlock("purple_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        BLUE_STAINED_GLASS = registerBlock("blue_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        BROWN_STAINED_GLASS = registerBlock("brown_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        GREEN_STAINED_GLASS = registerBlock("green_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        RED_STAINED_GLASS = registerBlock("red_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        BLACK_STAINED_GLASS = registerBlock("black_stained_glass", BlockSettings::create().material(BlockMaterial::GLASS));
        
        // Trapdoors
        OAK_TRAPDOOR = registerBlock("oak_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        SPRUCE_TRAPDOOR = registerBlock("spruce_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        BIRCH_TRAPDOOR = registerBlock("birch_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        JUNGLE_TRAPDOOR = registerBlock("jungle_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        ACACIA_TRAPDOOR = registerBlock("acacia_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        CHERRY_TRAPDOOR = registerBlock("cherry_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        DARK_OAK_TRAPDOOR = registerBlock("dark_oak_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        PALE_OAK_TRAPDOOR = registerBlock("pale_oak_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        MANGROVE_TRAPDOOR = registerBlock("mangrove_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        BAMBOO_TRAPDOOR = registerBlock("bamboo_trapdoor", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Stone brick variants
        STONE_BRICKS = registerBlock("stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        MOSSY_STONE_BRICKS = registerBlock("mossy_stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        CRACKED_STONE_BRICKS = registerBlock("cracked_stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        CHISELED_STONE_BRICKS = registerBlock("chiseled_stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        PACKED_MUD = registerBlock("packed_mud", BlockSettings::create().material(BlockMaterial::SOLID));
        MUD_BRICKS = registerBlock("mud_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Infested blocks
        INFESTED_STONE = registerBlock("infested_stone", BlockSettings::create().material(BlockMaterial::STONE));
        INFESTED_COBBLESTONE = registerBlock("infested_cobblestone", BlockSettings::create().material(BlockMaterial::STONE));
        INFESTED_STONE_BRICKS = registerBlock("infested_stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        INFESTED_MOSSY_STONE_BRICKS = registerBlock("infested_mossy_stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        INFESTED_CRACKED_STONE_BRICKS = registerBlock("infested_cracked_stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        INFESTED_CHISELED_STONE_BRICKS = registerBlock("infested_chiseled_stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Mushroom blocks
        BROWN_MUSHROOM_BLOCK = registerBlock("brown_mushroom_block", BlockSettings::create().material(BlockMaterial::SOLID));
        RED_MUSHROOM_BLOCK = registerBlock("red_mushroom_block", BlockSettings::create().material(BlockMaterial::SOLID));
        MUSHROOM_STEM = registerBlock("mushroom_stem", BlockSettings::create().material(BlockMaterial::SOLID));
        IRON_BARS = registerBlock("iron_bars", BlockSettings::create().material(BlockMaterial::STONE));
        CHAIN = registerBlock("chain", BlockSettings::create().material(BlockMaterial::STONE));
        GLASS_PANE = registerBlock("glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        PUMPKIN = registerBlock("pumpkin", BlockSettings::create().material(BlockMaterial::SOLID));
        MELON = registerBlock("melon", BlockSettings::create().material(BlockMaterial::SOLID));
        ATTACHED_PUMPKIN_STEM = registerBlock("attached_pumpkin_stem", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ATTACHED_MELON_STEM = registerBlock("attached_melon_stem", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PUMPKIN_STEM = registerBlock("pumpkin_stem", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        MELON_STEM = registerBlock("melon_stem", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        VINE = registerBlock("vine", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        GLOW_LICHEN = registerBlock("glow_lichen", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        RESIN_CLUMP = registerBlock("resin_clump", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Fence gates
        OAK_FENCE_GATE = registerBlock("oak_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        BRICK_STAIRS = registerBlock("brick_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        STONE_BRICK_STAIRS = registerBlock("stone_brick_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        MUD_BRICK_STAIRS = registerBlock("mud_brick_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        MYCELIUM = registerBlock("mycelium", BlockSettings::create().material(BlockMaterial::SOLID));
        LILY_PAD = registerBlock("lily_pad", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Resin blocks
        RESIN_BLOCK = registerBlock("resin_block", BlockSettings::create().material(BlockMaterial::SOLID));
        RESIN_BRICKS = registerBlock("resin_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        RESIN_BRICK_STAIRS = registerBlock("resin_brick_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        RESIN_BRICK_SLAB = registerBlock("resin_brick_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        RESIN_BRICK_WALL = registerBlock("resin_brick_wall", BlockSettings::create().material(BlockMaterial::STONE));
        CHISELED_RESIN_BRICKS = registerBlock("chiseled_resin_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Nether bricks
        NETHER_BRICKS = registerBlock("nether_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        NETHER_BRICK_FENCE = registerBlock("nether_brick_fence", BlockSettings::create().material(BlockMaterial::STONE));
        NETHER_BRICK_STAIRS = registerBlock("nether_brick_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        NETHER_WART = registerBlock("nether_wart", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ENCHANTING_TABLE = registerBlock("enchanting_table", BlockSettings::create().material(BlockMaterial::STONE));
        BREWING_STAND = registerBlock("brewing_stand", BlockSettings::create().material(BlockMaterial::STONE));
        CAULDRON = registerBlock("cauldron", BlockSettings::create().material(BlockMaterial::STONE));
        WATER_CAULDRON = registerBlock("water_cauldron", BlockSettings::create().material(BlockMaterial::STONE));
        LAVA_CAULDRON = registerBlock("lava_cauldron", BlockSettings::create().material(BlockMaterial::STONE));
        POWDER_SNOW_CAULDRON = registerBlock("powder_snow_cauldron", BlockSettings::create().material(BlockMaterial::STONE));
        
        // End blocks
        END_PORTAL = registerBlock("end_portal", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        END_PORTAL_FRAME = registerBlock("end_portal_frame", BlockSettings::create().material(BlockMaterial::STONE));
        END_STONE = registerBlock("end_stone", BlockSettings::create().material(BlockMaterial::STONE));
        DRAGON_EGG = registerBlock("dragon_egg", BlockSettings::create().material(BlockMaterial::STONE));
        REDSTONE_LAMP = registerBlock("redstone_lamp", BlockSettings::create().material(BlockMaterial::STONE));
        COCOA = registerBlock("cocoa", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SANDSTONE_STAIRS = registerBlock("sandstone_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        EMERALD_ORE = registerBlock("emerald_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DEEPSLATE_EMERALD_ORE = registerBlock("deepslate_emerald_ore", BlockSettings::create().material(BlockMaterial::STONE));
        ENDER_CHEST = registerBlock("ender_chest", BlockSettings::create().material(BlockMaterial::STONE));
        TRIPWIRE_HOOK = registerBlock("tripwire_hook", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        TRIPWIRE = registerBlock("tripwire", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        EMERALD_BLOCK = registerBlock("emerald_block", BlockSettings::create().material(BlockMaterial::STONE));
        
        // More stairs
        SPRUCE_STAIRS = registerBlock("spruce_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        BIRCH_STAIRS = registerBlock("birch_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        JUNGLE_STAIRS = registerBlock("jungle_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        COMMAND_BLOCK = registerBlock("command_block", BlockSettings::create().material(BlockMaterial::STONE));
        BEACON = registerBlock("beacon", BlockSettings::create().material(BlockMaterial::GLASS));
        COBBLESTONE_WALL = registerBlock("cobblestone_wall", BlockSettings::create().material(BlockMaterial::STONE));
        MOSSY_COBBLESTONE_WALL = registerBlock("mossy_cobblestone_wall", BlockSettings::create().material(BlockMaterial::STONE));
        FLOWER_POT = registerBlock("flower_pot", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Potted plants
        POTTED_TORCHFLOWER = registerBlock("potted_torchflower", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_OAK_SAPLING = registerBlock("potted_oak_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_SPRUCE_SAPLING = registerBlock("potted_spruce_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_BIRCH_SAPLING = registerBlock("potted_birch_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_JUNGLE_SAPLING = registerBlock("potted_jungle_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_ACACIA_SAPLING = registerBlock("potted_acacia_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_CHERRY_SAPLING = registerBlock("potted_cherry_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_DARK_OAK_SAPLING = registerBlock("potted_dark_oak_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_PALE_OAK_SAPLING = registerBlock("potted_pale_oak_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_MANGROVE_PROPAGULE = registerBlock("potted_mangrove_propagule", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_FERN = registerBlock("potted_fern", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_DANDELION = registerBlock("potted_dandelion", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_POPPY = registerBlock("potted_poppy", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_BLUE_ORCHID = registerBlock("potted_blue_orchid", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_ALLIUM = registerBlock("potted_allium", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_AZURE_BLUET = registerBlock("potted_azure_bluet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_RED_TULIP = registerBlock("potted_red_tulip", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_ORANGE_TULIP = registerBlock("potted_orange_tulip", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_WHITE_TULIP = registerBlock("potted_white_tulip", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_PINK_TULIP = registerBlock("potted_pink_tulip", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_OXEYE_DAISY = registerBlock("potted_oxeye_daisy", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_CORNFLOWER = registerBlock("potted_cornflower", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_LILY_OF_THE_VALLEY = registerBlock("potted_lily_of_the_valley", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_WITHER_ROSE = registerBlock("potted_wither_rose", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_RED_MUSHROOM = registerBlock("potted_red_mushroom", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_BROWN_MUSHROOM = registerBlock("potted_brown_mushroom", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_DEAD_BUSH = registerBlock("potted_dead_bush", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_CACTUS = registerBlock("potted_cactus", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Crops
        CARROTS = registerBlock("carrots", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTATOES = registerBlock("potatoes", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Wood buttons
        OAK_BUTTON = registerBlock("oak_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        SPRUCE_BUTTON = registerBlock("spruce_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BIRCH_BUTTON = registerBlock("birch_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        JUNGLE_BUTTON = registerBlock("jungle_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        ACACIA_BUTTON = registerBlock("acacia_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        CHERRY_BUTTON = registerBlock("cherry_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        DARK_OAK_BUTTON = registerBlock("dark_oak_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        PALE_OAK_BUTTON = registerBlock("pale_oak_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        MANGROVE_BUTTON = registerBlock("mangrove_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        BAMBOO_BUTTON = registerBlock("bamboo_button", BlockSettings::create().material(BlockMaterial::WOOD).noCollision());
        
        // Skulls and heads
        SKELETON_SKULL = registerBlock("skeleton_skull", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SKELETON_WALL_SKULL = registerBlock("skeleton_wall_skull", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        WITHER_SKELETON_SKULL = registerBlock("wither_skeleton_skull", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        WITHER_SKELETON_WALL_SKULL = registerBlock("wither_skeleton_wall_skull", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ZOMBIE_HEAD = registerBlock("zombie_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ZOMBIE_WALL_HEAD = registerBlock("zombie_wall_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PLAYER_HEAD = registerBlock("player_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PLAYER_WALL_HEAD = registerBlock("player_wall_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CREEPER_HEAD = registerBlock("creeper_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CREEPER_WALL_HEAD = registerBlock("creeper_wall_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DRAGON_HEAD = registerBlock("dragon_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DRAGON_WALL_HEAD = registerBlock("dragon_wall_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PIGLIN_HEAD = registerBlock("piglin_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PIGLIN_WALL_HEAD = registerBlock("piglin_wall_head", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Anvils
        ANVIL = registerBlock("anvil", BlockSettings::create().material(BlockMaterial::STONE));
        CHIPPED_ANVIL = registerBlock("chipped_anvil", BlockSettings::create().material(BlockMaterial::STONE));
        DAMAGED_ANVIL = registerBlock("damaged_anvil", BlockSettings::create().material(BlockMaterial::STONE));
        TRAPPED_CHEST = registerBlock("trapped_chest", BlockSettings::create().material(BlockMaterial::WOOD));
        LIGHT_WEIGHTED_PRESSURE_PLATE = registerBlock("light_weighted_pressure_plate", BlockSettings::create().material(BlockMaterial::STONE).noCollision());
        HEAVY_WEIGHTED_PRESSURE_PLATE = registerBlock("heavy_weighted_pressure_plate", BlockSettings::create().material(BlockMaterial::STONE).noCollision());
        COMPARATOR = registerBlock("comparator", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DAYLIGHT_DETECTOR = registerBlock("daylight_detector", BlockSettings::create().material(BlockMaterial::WOOD));
        REDSTONE_BLOCK = registerBlock("redstone_block", BlockSettings::create().material(BlockMaterial::STONE));
        NETHER_QUARTZ_ORE = registerBlock("nether_quartz_ore", BlockSettings::create().material(BlockMaterial::STONE));
        HOPPER = registerBlock("hopper", BlockSettings::create().material(BlockMaterial::STONE));
        QUARTZ_BLOCK = registerBlock("quartz_block", BlockSettings::create().material(BlockMaterial::STONE));
        CHISELED_QUARTZ_BLOCK = registerBlock("chiseled_quartz_block", BlockSettings::create().material(BlockMaterial::STONE));
        QUARTZ_PILLAR = registerBlock("quartz_pillar", BlockSettings::create().material(BlockMaterial::STONE));
        QUARTZ_STAIRS = registerBlock("quartz_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        ACTIVATOR_RAIL = registerBlock("activator_rail", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DROPPER = registerBlock("dropper", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Terracotta
        WHITE_TERRACOTTA = registerBlock("white_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        ORANGE_TERRACOTTA = registerBlock("orange_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        MAGENTA_TERRACOTTA = registerBlock("magenta_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        LIGHT_BLUE_TERRACOTTA = registerBlock("light_blue_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        YELLOW_TERRACOTTA = registerBlock("yellow_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        LIME_TERRACOTTA = registerBlock("lime_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        PINK_TERRACOTTA = registerBlock("pink_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        GRAY_TERRACOTTA = registerBlock("gray_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        LIGHT_GRAY_TERRACOTTA = registerBlock("light_gray_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        CYAN_TERRACOTTA = registerBlock("cyan_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        PURPLE_TERRACOTTA = registerBlock("purple_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        BLUE_TERRACOTTA = registerBlock("blue_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        BROWN_TERRACOTTA = registerBlock("brown_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        GREEN_TERRACOTTA = registerBlock("green_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        RED_TERRACOTTA = registerBlock("red_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        BLACK_TERRACOTTA = registerBlock("black_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Stained glass panes
        WHITE_STAINED_GLASS_PANE = registerBlock("white_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        ORANGE_STAINED_GLASS_PANE = registerBlock("orange_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        MAGENTA_STAINED_GLASS_PANE = registerBlock("magenta_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        LIGHT_BLUE_STAINED_GLASS_PANE = registerBlock("light_blue_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        YELLOW_STAINED_GLASS_PANE = registerBlock("yellow_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        LIME_STAINED_GLASS_PANE = registerBlock("lime_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        PINK_STAINED_GLASS_PANE = registerBlock("pink_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        GRAY_STAINED_GLASS_PANE = registerBlock("gray_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        LIGHT_GRAY_STAINED_GLASS_PANE = registerBlock("light_gray_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        CYAN_STAINED_GLASS_PANE = registerBlock("cyan_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        PURPLE_STAINED_GLASS_PANE = registerBlock("purple_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        BLUE_STAINED_GLASS_PANE = registerBlock("blue_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        BROWN_STAINED_GLASS_PANE = registerBlock("brown_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        GREEN_STAINED_GLASS_PANE = registerBlock("green_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        RED_STAINED_GLASS_PANE = registerBlock("red_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        BLACK_STAINED_GLASS_PANE = registerBlock("black_stained_glass_pane", BlockSettings::create().material(BlockMaterial::GLASS));
        
        // More stairs
        ACACIA_STAIRS = registerBlock("acacia_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        CHERRY_STAIRS = registerBlock("cherry_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        DARK_OAK_STAIRS = registerBlock("dark_oak_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        PALE_OAK_STAIRS = registerBlock("pale_oak_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        MANGROVE_STAIRS = registerBlock("mangrove_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        BAMBOO_STAIRS = registerBlock("bamboo_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        BAMBOO_MOSAIC_STAIRS = registerBlock("bamboo_mosaic_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        SLIME_BLOCK = registerBlock("slime_block", BlockSettings::create().material(BlockMaterial::SOLID));
        BARRIER = registerBlock("barrier", BlockSettings::create().material(BlockMaterial::SOLID).noCollision().transparent());
        LIGHT = registerBlock("light", BlockSettings::create().material(BlockMaterial::SOLID).noCollision().transparent());
        IRON_TRAPDOOR = registerBlock("iron_trapdoor", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Prismarine blocks
        PRISMARINE = registerBlock("prismarine", BlockSettings::create().material(BlockMaterial::STONE));
        PRISMARINE_BRICKS = registerBlock("prismarine_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        DARK_PRISMARINE = registerBlock("dark_prismarine", BlockSettings::create().material(BlockMaterial::STONE));
        PRISMARINE_STAIRS = registerBlock("prismarine_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        PRISMARINE_BRICK_STAIRS = registerBlock("prismarine_brick_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        DARK_PRISMARINE_STAIRS = registerBlock("dark_prismarine_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        PRISMARINE_SLAB = registerBlock("prismarine_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        PRISMARINE_BRICK_SLAB = registerBlock("prismarine_brick_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        DARK_PRISMARINE_SLAB = registerBlock("dark_prismarine_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        SEA_LANTERN = registerBlock("sea_lantern", BlockSettings::create().material(BlockMaterial::GLASS));
        HAY_BLOCK = registerBlock("hay_block", BlockSettings::create().material(BlockMaterial::SOLID));
        
        // Carpets
        WHITE_CARPET = registerBlock("white_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ORANGE_CARPET = registerBlock("orange_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        MAGENTA_CARPET = registerBlock("magenta_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIGHT_BLUE_CARPET = registerBlock("light_blue_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        YELLOW_CARPET = registerBlock("yellow_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIME_CARPET = registerBlock("lime_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PINK_CARPET = registerBlock("pink_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        GRAY_CARPET = registerBlock("gray_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIGHT_GRAY_CARPET = registerBlock("light_gray_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CYAN_CARPET = registerBlock("cyan_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PURPLE_CARPET = registerBlock("purple_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BLUE_CARPET = registerBlock("blue_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BROWN_CARPET = registerBlock("brown_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        GREEN_CARPET = registerBlock("green_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        RED_CARPET = registerBlock("red_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BLACK_CARPET = registerBlock("black_carpet", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        TERRACOTTA = registerBlock("terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        COAL_BLOCK = registerBlock("coal_block", BlockSettings::create().material(BlockMaterial::STONE));
        PACKED_ICE = registerBlock("packed_ice", BlockSettings::create().material(BlockMaterial::GLASS));
        
        // Large plants
        SUNFLOWER = registerBlock("sunflower", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LILAC = registerBlock("lilac", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ROSE_BUSH = registerBlock("rose_bush", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PEONY = registerBlock("peony", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        TALL_GRASS = registerBlock("tall_grass", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LARGE_FERN = registerBlock("large_fern", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Banners
        WHITE_BANNER = registerBlock("white_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ORANGE_BANNER = registerBlock("orange_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        MAGENTA_BANNER = registerBlock("magenta_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIGHT_BLUE_BANNER = registerBlock("light_blue_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        YELLOW_BANNER = registerBlock("yellow_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIME_BANNER = registerBlock("lime_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PINK_BANNER = registerBlock("pink_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        GRAY_BANNER = registerBlock("gray_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIGHT_GRAY_BANNER = registerBlock("light_gray_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CYAN_BANNER = registerBlock("cyan_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PURPLE_BANNER = registerBlock("purple_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BLUE_BANNER = registerBlock("blue_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BROWN_BANNER = registerBlock("brown_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        GREEN_BANNER = registerBlock("green_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        RED_BANNER = registerBlock("red_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BLACK_BANNER = registerBlock("black_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Wall banners
        WHITE_WALL_BANNER = registerBlock("white_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        ORANGE_WALL_BANNER = registerBlock("orange_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        MAGENTA_WALL_BANNER = registerBlock("magenta_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIGHT_BLUE_WALL_BANNER = registerBlock("light_blue_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        YELLOW_WALL_BANNER = registerBlock("yellow_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIME_WALL_BANNER = registerBlock("lime_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PINK_WALL_BANNER = registerBlock("pink_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        GRAY_WALL_BANNER = registerBlock("gray_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        LIGHT_GRAY_WALL_BANNER = registerBlock("light_gray_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CYAN_WALL_BANNER = registerBlock("cyan_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PURPLE_WALL_BANNER = registerBlock("purple_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BLUE_WALL_BANNER = registerBlock("blue_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BROWN_WALL_BANNER = registerBlock("brown_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        GREEN_WALL_BANNER = registerBlock("green_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        RED_WALL_BANNER = registerBlock("red_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BLACK_WALL_BANNER = registerBlock("black_wall_banner", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Red sandstone
        RED_SANDSTONE = registerBlock("red_sandstone", BlockSettings::create().material(BlockMaterial::STONE));
        CHISELED_RED_SANDSTONE = registerBlock("chiseled_red_sandstone", BlockSettings::create().material(BlockMaterial::STONE));
        CUT_RED_SANDSTONE = registerBlock("cut_red_sandstone", BlockSettings::create().material(BlockMaterial::STONE));
        RED_SANDSTONE_STAIRS = registerBlock("red_sandstone_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        
        // Slabs
        OAK_SLAB = registerBlock("oak_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        SPRUCE_SLAB = registerBlock("spruce_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        BIRCH_SLAB = registerBlock("birch_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        JUNGLE_SLAB = registerBlock("jungle_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        ACACIA_SLAB = registerBlock("acacia_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        CHERRY_SLAB = registerBlock("cherry_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        DARK_OAK_SLAB = registerBlock("dark_oak_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        PALE_OAK_SLAB = registerBlock("pale_oak_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        MANGROVE_SLAB = registerBlock("mangrove_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        BAMBOO_SLAB = registerBlock("bamboo_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        BAMBOO_MOSAIC_SLAB = registerBlock("bamboo_mosaic_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        STONE_SLAB = registerBlock("stone_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        SMOOTH_STONE_SLAB = registerBlock("smooth_stone_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        SANDSTONE_SLAB = registerBlock("sandstone_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        CUT_SANDSTONE_SLAB = registerBlock("cut_sandstone_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        PETRIFIED_OAK_SLAB = registerBlock("petrified_oak_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        COBBLESTONE_SLAB = registerBlock("cobblestone_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        BRICK_SLAB = registerBlock("brick_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        STONE_BRICK_SLAB = registerBlock("stone_brick_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        MUD_BRICK_SLAB = registerBlock("mud_brick_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        NETHER_BRICK_SLAB = registerBlock("nether_brick_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        QUARTZ_SLAB = registerBlock("quartz_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        RED_SANDSTONE_SLAB = registerBlock("red_sandstone_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        CUT_RED_SANDSTONE_SLAB = registerBlock("cut_red_sandstone_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        PURPUR_SLAB = registerBlock("purpur_slab", BlockSettings::create().material(BlockMaterial::STONE).slab());
        
        // Smooth blocks
        SMOOTH_STONE = registerBlock("smooth_stone", BlockSettings::create().material(BlockMaterial::STONE));
        SMOOTH_SANDSTONE = registerBlock("smooth_sandstone", BlockSettings::create().material(BlockMaterial::STONE));
        SMOOTH_QUARTZ = registerBlock("smooth_quartz", BlockSettings::create().material(BlockMaterial::STONE));
        SMOOTH_RED_SANDSTONE = registerBlock("smooth_red_sandstone", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Fence gates
        SPRUCE_FENCE_GATE = registerBlock("spruce_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        BIRCH_FENCE_GATE = registerBlock("birch_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        JUNGLE_FENCE_GATE = registerBlock("jungle_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        ACACIA_FENCE_GATE = registerBlock("acacia_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        CHERRY_FENCE_GATE = registerBlock("cherry_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        DARK_OAK_FENCE_GATE = registerBlock("dark_oak_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        PALE_OAK_FENCE_GATE = registerBlock("pale_oak_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        MANGROVE_FENCE_GATE = registerBlock("mangrove_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        BAMBOO_FENCE_GATE = registerBlock("bamboo_fence_gate", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Fences
        SPRUCE_FENCE = registerBlock("spruce_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        BIRCH_FENCE = registerBlock("birch_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        JUNGLE_FENCE = registerBlock("jungle_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        ACACIA_FENCE = registerBlock("acacia_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        CHERRY_FENCE = registerBlock("cherry_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        DARK_OAK_FENCE = registerBlock("dark_oak_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        PALE_OAK_FENCE = registerBlock("pale_oak_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        MANGROVE_FENCE = registerBlock("mangrove_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        BAMBOO_FENCE = registerBlock("bamboo_fence", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // Doors
        SPRUCE_DOOR = registerBlock("spruce_door", BlockSettings::create().material(BlockMaterial::WOOD));
        BIRCH_DOOR = registerBlock("birch_door", BlockSettings::create().material(BlockMaterial::WOOD));
        JUNGLE_DOOR = registerBlock("jungle_door", BlockSettings::create().material(BlockMaterial::WOOD));
        ACACIA_DOOR = registerBlock("acacia_door", BlockSettings::create().material(BlockMaterial::WOOD));
        CHERRY_DOOR = registerBlock("cherry_door", BlockSettings::create().material(BlockMaterial::WOOD));
        DARK_OAK_DOOR = registerBlock("dark_oak_door", BlockSettings::create().material(BlockMaterial::WOOD));
        PALE_OAK_DOOR = registerBlock("pale_oak_door", BlockSettings::create().material(BlockMaterial::WOOD));
        MANGROVE_DOOR = registerBlock("mangrove_door", BlockSettings::create().material(BlockMaterial::WOOD));
        BAMBOO_DOOR = registerBlock("bamboo_door", BlockSettings::create().material(BlockMaterial::WOOD));
        
        // End blocks
        END_ROD = registerBlock("end_rod", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        CHORUS_PLANT = registerBlock("chorus_plant", BlockSettings::create().material(BlockMaterial::SOLID));
        CHORUS_FLOWER = registerBlock("chorus_flower", BlockSettings::create().material(BlockMaterial::SOLID));
        PURPUR_BLOCK = registerBlock("purpur_block", BlockSettings::create().material(BlockMaterial::STONE));
        PURPUR_PILLAR = registerBlock("purpur_pillar", BlockSettings::create().material(BlockMaterial::STONE));
        PURPUR_STAIRS = registerBlock("purpur_stairs", BlockSettings::create().material(BlockMaterial::STONE).stairs());
        END_STONE_BRICKS = registerBlock("end_stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Crops and farm blocks
        TORCHFLOWER_CROP = registerBlock("torchflower_crop", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PITCHER_CROP = registerBlock("pitcher_crop", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        PITCHER_PLANT = registerBlock("pitcher_plant", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BEETROOTS = registerBlock("beetroots", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DIRT_PATH = registerBlock("dirt_path", BlockSettings::create().material(BlockMaterial::SOLID));
        
        // More command blocks
        END_GATEWAY = registerBlock("end_gateway", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        REPEATING_COMMAND_BLOCK = registerBlock("repeating_command_block", BlockSettings::create().material(BlockMaterial::STONE));
        CHAIN_COMMAND_BLOCK = registerBlock("chain_command_block", BlockSettings::create().material(BlockMaterial::STONE));
        FROSTED_ICE = registerBlock("frosted_ice", BlockSettings::create().material(BlockMaterial::GLASS));
        MAGMA_BLOCK = registerBlock("magma_block", BlockSettings::create().material(BlockMaterial::STONE));
        NETHER_WART_BLOCK = registerBlock("nether_wart_block", BlockSettings::create().material(BlockMaterial::SOLID));
        RED_NETHER_BRICKS = registerBlock("red_nether_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        BONE_BLOCK = registerBlock("bone_block", BlockSettings::create().material(BlockMaterial::SOLID));
        STRUCTURE_VOID = registerBlock("structure_void", BlockSettings::create().material(BlockMaterial::SOLID).noCollision().transparent());
        OBSERVER = registerBlock("observer", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Shulker boxes
        SHULKER_BOX = registerBlock("shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        WHITE_SHULKER_BOX = registerBlock("white_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        ORANGE_SHULKER_BOX = registerBlock("orange_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        MAGENTA_SHULKER_BOX = registerBlock("magenta_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        LIGHT_BLUE_SHULKER_BOX = registerBlock("light_blue_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        YELLOW_SHULKER_BOX = registerBlock("yellow_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        LIME_SHULKER_BOX = registerBlock("lime_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        PINK_SHULKER_BOX = registerBlock("pink_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        GRAY_SHULKER_BOX = registerBlock("gray_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        LIGHT_GRAY_SHULKER_BOX = registerBlock("light_gray_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        CYAN_SHULKER_BOX = registerBlock("cyan_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        PURPLE_SHULKER_BOX = registerBlock("purple_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        BLUE_SHULKER_BOX = registerBlock("blue_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        BROWN_SHULKER_BOX = registerBlock("brown_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        GREEN_SHULKER_BOX = registerBlock("green_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        RED_SHULKER_BOX = registerBlock("red_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        BLACK_SHULKER_BOX = registerBlock("black_shulker_box", BlockSettings::create().material(BlockMaterial::SOLID));
        
        // Glazed terracotta
        WHITE_GLAZED_TERRACOTTA = registerBlock("white_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        ORANGE_GLAZED_TERRACOTTA = registerBlock("orange_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        MAGENTA_GLAZED_TERRACOTTA = registerBlock("magenta_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        LIGHT_BLUE_GLAZED_TERRACOTTA = registerBlock("light_blue_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        YELLOW_GLAZED_TERRACOTTA = registerBlock("yellow_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        LIME_GLAZED_TERRACOTTA = registerBlock("lime_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        PINK_GLAZED_TERRACOTTA = registerBlock("pink_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        GRAY_GLAZED_TERRACOTTA = registerBlock("gray_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        LIGHT_GRAY_GLAZED_TERRACOTTA = registerBlock("light_gray_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        CYAN_GLAZED_TERRACOTTA = registerBlock("cyan_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        PURPLE_GLAZED_TERRACOTTA = registerBlock("purple_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        BLUE_GLAZED_TERRACOTTA = registerBlock("blue_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        BROWN_GLAZED_TERRACOTTA = registerBlock("brown_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        GREEN_GLAZED_TERRACOTTA = registerBlock("green_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        RED_GLAZED_TERRACOTTA = registerBlock("red_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        BLACK_GLAZED_TERRACOTTA = registerBlock("black_glazed_terracotta", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Concrete blocks
        WHITE_CONCRETE = registerBlock("white_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        ORANGE_CONCRETE = registerBlock("orange_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        MAGENTA_CONCRETE = registerBlock("magenta_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        LIGHT_BLUE_CONCRETE = registerBlock("light_blue_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        YELLOW_CONCRETE = registerBlock("yellow_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        LIME_CONCRETE = registerBlock("lime_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        PINK_CONCRETE = registerBlock("pink_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        GRAY_CONCRETE = registerBlock("gray_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        LIGHT_GRAY_CONCRETE = registerBlock("light_gray_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        CYAN_CONCRETE = registerBlock("cyan_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        PURPLE_CONCRETE = registerBlock("purple_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        BLUE_CONCRETE = registerBlock("blue_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        BROWN_CONCRETE = registerBlock("brown_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        GREEN_CONCRETE = registerBlock("green_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        RED_CONCRETE = registerBlock("red_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        BLACK_CONCRETE = registerBlock("black_concrete", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Concrete powder
        WHITE_CONCRETE_POWDER = registerBlock("white_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        ORANGE_CONCRETE_POWDER = registerBlock("orange_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        MAGENTA_CONCRETE_POWDER = registerBlock("magenta_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        LIGHT_BLUE_CONCRETE_POWDER = registerBlock("light_blue_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        YELLOW_CONCRETE_POWDER = registerBlock("yellow_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        LIME_CONCRETE_POWDER = registerBlock("lime_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        PINK_CONCRETE_POWDER = registerBlock("pink_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        GRAY_CONCRETE_POWDER = registerBlock("gray_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        LIGHT_GRAY_CONCRETE_POWDER = registerBlock("light_gray_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        CYAN_CONCRETE_POWDER = registerBlock("cyan_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        PURPLE_CONCRETE_POWDER = registerBlock("purple_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        BLUE_CONCRETE_POWDER = registerBlock("blue_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        BROWN_CONCRETE_POWDER = registerBlock("brown_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        GREEN_CONCRETE_POWDER = registerBlock("green_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        RED_CONCRETE_POWDER = registerBlock("red_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        BLACK_CONCRETE_POWDER = registerBlock("black_concrete_powder", BlockSettings::create().material(BlockMaterial::SOLID));
        
        // Kelp and ocean blocks
        KELP = registerBlock("kelp", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        KELP_PLANT = registerBlock("kelp_plant", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DRIED_KELP_BLOCK = registerBlock("dried_kelp_block", BlockSettings::create().material(BlockMaterial::SOLID));
        TURTLE_EGG = registerBlock("turtle_egg", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        SNIFFER_EGG = registerBlock("sniffer_egg", BlockSettings::create().material(BlockMaterial::SOLID));
        
        // Dead coral blocks
        DEAD_TUBE_CORAL_BLOCK = registerBlock("dead_tube_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        DEAD_BRAIN_CORAL_BLOCK = registerBlock("dead_brain_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        DEAD_BUBBLE_CORAL_BLOCK = registerBlock("dead_bubble_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        DEAD_FIRE_CORAL_BLOCK = registerBlock("dead_fire_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        DEAD_HORN_CORAL_BLOCK = registerBlock("dead_horn_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Live coral blocks
        TUBE_CORAL_BLOCK = registerBlock("tube_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        BRAIN_CORAL_BLOCK = registerBlock("brain_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        BUBBLE_CORAL_BLOCK = registerBlock("bubble_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        FIRE_CORAL_BLOCK = registerBlock("fire_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        HORN_CORAL_BLOCK = registerBlock("horn_coral_block", BlockSettings::create().material(BlockMaterial::STONE));
        
        // Dead coral
        DEAD_TUBE_CORAL = registerBlock("dead_tube_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_BRAIN_CORAL = registerBlock("dead_brain_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_BUBBLE_CORAL = registerBlock("dead_bubble_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_FIRE_CORAL = registerBlock("dead_fire_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_HORN_CORAL = registerBlock("dead_horn_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Live coral
        TUBE_CORAL = registerBlock("tube_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BRAIN_CORAL = registerBlock("brain_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BUBBLE_CORAL = registerBlock("bubble_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        FIRE_CORAL = registerBlock("fire_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        HORN_CORAL = registerBlock("horn_coral", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Dead coral fans
        DEAD_TUBE_CORAL_FAN = registerBlock("dead_tube_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_BRAIN_CORAL_FAN = registerBlock("dead_brain_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_BUBBLE_CORAL_FAN = registerBlock("dead_bubble_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_FIRE_CORAL_FAN = registerBlock("dead_fire_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_HORN_CORAL_FAN = registerBlock("dead_horn_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Live coral fans
        TUBE_CORAL_FAN = registerBlock("tube_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BRAIN_CORAL_FAN = registerBlock("brain_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BUBBLE_CORAL_FAN = registerBlock("bubble_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        FIRE_CORAL_FAN = registerBlock("fire_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        HORN_CORAL_FAN = registerBlock("horn_coral_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Dead coral wall fans
        DEAD_TUBE_CORAL_WALL_FAN = registerBlock("dead_tube_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_BRAIN_CORAL_WALL_FAN = registerBlock("dead_brain_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_BUBBLE_CORAL_WALL_FAN = registerBlock("dead_bubble_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_FIRE_CORAL_WALL_FAN = registerBlock("dead_fire_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        DEAD_HORN_CORAL_WALL_FAN = registerBlock("dead_horn_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Live coral wall fans
        TUBE_CORAL_WALL_FAN = registerBlock("tube_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BRAIN_CORAL_WALL_FAN = registerBlock("brain_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BUBBLE_CORAL_WALL_FAN = registerBlock("bubble_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        FIRE_CORAL_WALL_FAN = registerBlock("fire_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        HORN_CORAL_WALL_FAN = registerBlock("horn_coral_wall_fan", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Sea blocks
        SEA_PICKLE = registerBlock("sea_pickle", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BLUE_ICE = registerBlock("blue_ice", BlockSettings::create().material(BlockMaterial::GLASS));
        CONDUIT = registerBlock("conduit", BlockSettings::create().material(BlockMaterial::GLASS));
        
        // Bamboo
        BAMBOO_SAPLING = registerBlock("bamboo_sapling", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        BAMBOO = registerBlock("bamboo", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        POTTED_BAMBOO = registerBlock("potted_bamboo", BlockSettings::create().material(BlockMaterial::SOLID).noCollision());
        
        // Air variants
        VOID_AIR = registerBlock("void_air", BlockSettings::create().material(BlockMaterial::AIR));
        CAVE_AIR = registerBlock("cave_air", BlockSettings::create().material(BlockMaterial::AIR));
        BUBBLE_COLUMN = registerBlock("bubble_column", BlockSettings::create().material(BlockMaterial::LIQUID).noCollision());
        
        // Water with custom fluid behavior  
        auto waterSettings = BlockSettings::create().material(BlockMaterial::LIQUID);
        auto waterBehavior = std::make_unique<FluidBlock>(waterSettings);
        WATER = registerBlock("water", waterSettings, std::move(waterBehavior));
        
        // Lava with custom fluid behavior
        auto lavaSettings = BlockSettings::create().material(BlockMaterial::LIQUID);
        auto lavaBehavior = std::make_unique<FluidBlock>(lavaSettings);
        LAVA = registerBlock("lava", lavaSettings, std::move(lavaBehavior));

        initialized_ = true;
        LOG_INFO("Blocks system initialized with %zu blocks", blocks_.size());
    }
} // namespace Zerith
