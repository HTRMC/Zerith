#pragma once

#include "chunk.h"
#include "blocks/block_behavior.h"
#include "block_properties.h"
#include "translation_manager.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Zerith {

// Forward declarations
class BlockDefinition;
class FluidBlock;
using BlockDefPtr = std::shared_ptr<BlockDefinition>;

// Block material types for common properties
enum class BlockMaterial {
    AIR,
    SOLID,
    WOOD,
    STONE,
    GLASS,
    LEAVES,
    LIQUID
};

// Render layer types for transparency handling
enum class RenderLayer {
    OPAQUE,      // Solid blocks (stone, dirt, wood) - render first with depth writing
    CUTOUT,      // Blocks with binary alpha (leaves) - render second with alpha testing
    TRANSLUCENT  // Transparent blocks (glass, water) - render last with alpha blending
};

// Enhanced block settings that combines rendering and behavior settings
class BlockSettings {
private:
    std::string modelName_;
    BlockMaterial material_ = BlockMaterial::SOLID;
    bool isTransparent_ = false;
    bool renderAllFaces_ = false;
    
    // Behavior settings
    bool hasCollision_ = true;
    bool isLiquid_ = false;
    bool isWalkThrough_ = false;
    float hardness_ = 1.0f;
    
    // Culling properties
    std::array<CullFace, 6> faceCulling_ = {
        CullFace::FULL, CullFace::FULL, CullFace::FULL,
        CullFace::FULL, CullFace::FULL, CullFace::FULL
    };

public:
    static BlockSettings create() { return BlockSettings(); }
    
    // Model and rendering
    BlockSettings& model(const std::string& modelName) {
        modelName_ = modelName;
        return *this;
    }
    
    BlockSettings& material(BlockMaterial mat) {
        material_ = mat;
        // Apply material presets
        switch (mat) {
            case BlockMaterial::AIR:
                isTransparent_ = true;
                renderAllFaces_ = false;
                hasCollision_ = false;
                isWalkThrough_ = true;
                std::fill(faceCulling_.begin(), faceCulling_.end(), CullFace::NONE);
                break;
            case BlockMaterial::GLASS:
            case BlockMaterial::LEAVES:
                isTransparent_ = true;
                renderAllFaces_ = true;
                break;
            case BlockMaterial::LIQUID:
                isTransparent_ = true;
                renderAllFaces_ = false;
                hasCollision_ = false;
                isLiquid_ = true;
                isWalkThrough_ = true;
                faceCulling_[1] = CullFace::NONE; // Top face doesn't cull
                break;
            default:
                break;
        }
        return *this;
    }
    
    BlockSettings& transparent() {
        isTransparent_ = true;
        return *this;
    }
    
    BlockSettings& renderAllFaces() {
        renderAllFaces_ = true;
        return *this;
    }
    
    // Behavior settings (similar to new block_settings.h)
    BlockSettings& noCollision() { 
        hasCollision_ = false; 
        return *this; 
    }
    
    BlockSettings& collision() { 
        hasCollision_ = true; 
        return *this; 
    }
    
    BlockSettings& liquid() { 
        isLiquid_ = true; 
        hasCollision_ = false;
        return *this; 
    }
    
    BlockSettings& walkThrough() { 
        isWalkThrough_ = true; 
        return *this; 
    }
    
    BlockSettings& strength(float hardness) { 
        hardness_ = hardness; 
        return *this; 
    }
    
    // Culling settings
    BlockSettings& noFaceCulling() {
        std::fill(faceCulling_.begin(), faceCulling_.end(), CullFace::NONE);
        return *this;
    }
    
    BlockSettings& slab() {
        faceCulling_[0] = CullFace::FULL; // Bottom
        faceCulling_[1] = CullFace::NONE; // Top
        return *this;
    }
    
    BlockSettings& stairs() {
        faceCulling_[1] = CullFace::NONE; // Top face varies
        return *this;
    }
    
    // Getters
    const std::string& getModelName() const { return modelName_; }
    BlockMaterial getMaterial() const { return material_; }
    bool isTransparent() const { return isTransparent_; }
    bool shouldRenderAllFaces() const { return renderAllFaces_; }
    bool hasCollision() const { return hasCollision_; }
    bool isLiquid() const { return isLiquid_; }
    bool isWalkThrough() const { return isWalkThrough_; }
    float getHardness() const { return hardness_; }
    const std::array<CullFace, 6>& getFaceCulling() const { return faceCulling_; }
    
    friend class BlockDefinition;
};

// Block definition that combines everything
class BlockDefinition {
private:
    std::string id_;
    BlockSettings settings_;
    BlockType blockType_;
    std::unique_ptr<BlockBehavior> behavior_;
    
public:
    BlockDefinition(const std::string& id, const BlockSettings& settings, std::unique_ptr<BlockBehavior> behavior = nullptr)
        : id_(id), settings_(settings), behavior_(std::move(behavior)) {}
    
    const std::string& getId() const { return id_; }
    
    std::string getDisplayName() const { 
        std::string translationKey = TranslationManager::getTranslationKey(id_);
        return TranslationManager::getInstance().translate(translationKey);
    }
    
    const std::string& getModelName() const { 
        return settings_.modelName_.empty() ? id_ : settings_.modelName_; 
    }
    
    BlockCullingProperties getCullingProperties() const {
        return {
            settings_.faceCulling_,
            settings_.isTransparent_,
            !settings_.renderAllFaces_
        };
    }
    
    RenderLayer getRenderLayer() const {
        switch (settings_.material_) {
            case BlockMaterial::AIR:
                return RenderLayer::OPAQUE; // Air doesn't render, but default to opaque
            case BlockMaterial::GLASS:
            case BlockMaterial::LIQUID:
                return RenderLayer::TRANSLUCENT;
            case BlockMaterial::LEAVES:
                return RenderLayer::CUTOUT;
            case BlockMaterial::SOLID:
            case BlockMaterial::WOOD:
            case BlockMaterial::STONE:
            default:
                return RenderLayer::OPAQUE;
        }
    }
    
    // Block behavior access
    const BlockBehavior* getBehavior() const {
        return behavior_ ? behavior_.get() : nullptr;
    }
    
    // Convenience methods that delegate to behavior or settings
    bool hasCollision() const {
        if (behavior_) return behavior_->hasCollision();
        return settings_.hasCollision_;
    }
    
    bool hasCollisionAt(const glm::vec3& position, const AABB& entityAABB) const {
        if (behavior_) return behavior_->hasCollisionAt(position, entityAABB);
        return hasCollision();
    }
    
    bool isWalkThrough() const {
        if (behavior_) return behavior_->isWalkThrough();
        return settings_.isWalkThrough_;
    }
    
    void onPlayerEnter(const glm::vec3& playerPos) const {
        if (behavior_) behavior_->onPlayerEnter(playerPos);
    }
    
    void onPlayerExit(const glm::vec3& playerPos) const {
        if (behavior_) behavior_->onPlayerExit(playerPos);
    }
    
    void setBlockType(BlockType type) { blockType_ = type; }
    BlockType getBlockType() const { return blockType_; }
};

// Unified Blocks registry and constants
class Blocks {
private:
    static std::vector<BlockDefPtr> blocks_;
    static std::unordered_map<std::string, size_t> idToIndex_;
    static bool initialized_;

public:
    // Block type constants
    static BlockType AIR;
    static BlockType STONE;
    static BlockType GRANITE;
    static BlockType POLISHED_GRANITE;
    static BlockType DIORITE;
    static BlockType POLISHED_DIORITE;
    static BlockType ANDESITE;
    static BlockType POLISHED_ANDESITE;
    static BlockType GRASS_BLOCK;
    static BlockType DIRT;
    static BlockType COARSE_DIRT;
    static BlockType PODZOL;
    static BlockType COBBLESTONE;
    static BlockType OAK_PLANKS;
    static BlockType SPRUCE_PLANKS;
    static BlockType BIRCH_PLANKS;
    static BlockType JUNGLE_PLANKS;
    static BlockType ACACIA_PLANKS;
    static BlockType CHERRY_PLANKS;
    static BlockType DARK_OAK_PLANKS;
    static BlockType PALE_OAK_WOOD;
    static BlockType PALE_OAK_PLANKS;
    static BlockType MANGROVE_PLANKS;
    static BlockType BAMBOO_PLANKS;
    static BlockType BAMBOO_MOSAIC;
    static BlockType OAK_SAPLING;
    static BlockType SPRUCE_SAPLING;
    static BlockType BIRCH_SAPLING;
    static BlockType JUNGLE_SAPLING;
    static BlockType ACACIA_SAPLING;
    static BlockType CHERRY_SAPLING;
    static BlockType DARK_OAK_SAPLING;
    static BlockType PALE_OAK_SAPLING;
    static BlockType MANGROVE_PROPAGULE;
    static BlockType BEDROCK;
    static BlockType WATER;
    static BlockType LAVA;
    static BlockType SAND;
    static BlockType SUSPICIOUS_SAND;
    static BlockType RED_SAND;
    static BlockType GRAVEL;
    static BlockType SUSPICIOUS_GRAVEL;
    static BlockType GOLD_ORE;
    static BlockType DEEPSLATE_GOLD_ORE;
    static BlockType IRON_ORE;
    static BlockType DEEPSLATE_IRON_ORE;
    static BlockType COAL_ORE;
    static BlockType DEEPSLATE_COAL_ORE;
    static BlockType NETHER_GOLD_ORE;
    static BlockType OAK_LOG;
    static BlockType SPRUCE_LOG;
    static BlockType BIRCH_LOG;
    static BlockType JUNGLE_LOG;
    static BlockType ACACIA_LOG;
    static BlockType CHERRY_LOG;
    static BlockType DARK_OAK_LOG;
    static BlockType PALE_OAK_LOG;
    static BlockType MANGROVE_LOG;
    static BlockType MANGROVE_ROOTS;
    static BlockType MUDDY_MANGROVE_ROOTS;
    static BlockType BAMBOO_BLOCK;
    static BlockType STRIPPED_SPRUCE_LOG;
    static BlockType STRIPPED_BIRCH_LOG;
    static BlockType STRIPPED_JUNGLE_LOG;
    static BlockType STRIPPED_ACACIA_LOG;
    static BlockType STRIPPED_CHERRY_LOG;
    static BlockType STRIPPED_DARK_OAK_LOG;
    static BlockType STRIPPED_PALE_OAK_LOG;
    static BlockType STRIPPED_OAK_LOG;
    static BlockType STRIPPED_MANGROVE_LOG;
    static BlockType STRIPPED_BAMBOO_BLOCK;
    static BlockType OAK_WOOD;
    static BlockType SPRUCE_WOOD;
    static BlockType BIRCH_WOOD;
    static BlockType JUNGLE_WOOD;
    static BlockType ACACIA_WOOD;
    static BlockType CHERRY_WOOD;
    static BlockType DARK_OAK_WOOD;
    static BlockType MANGROVE_WOOD;
    static BlockType STRIPPED_OAK_WOOD;
    static BlockType STRIPPED_SPRUCE_WOOD;
    static BlockType STRIPPED_BIRCH_WOOD;
    static BlockType STRIPPED_JUNGLE_WOOD;
    static BlockType STRIPPED_ACACIA_WOOD;
    static BlockType STRIPPED_CHERRY_WOOD;
    static BlockType STRIPPED_DARK_OAK_WOOD;
    static BlockType STRIPPED_PALE_OAK_WOOD;
    static BlockType STRIPPED_MANGROVE_WOOD;
    static BlockType OAK_LEAVES;
    static BlockType SPRUCE_LEAVES;
    static BlockType BIRCH_LEAVES;
    static BlockType JUNGLE_LEAVES;
    static BlockType ACACIA_LEAVES;
    static BlockType CHERRY_LEAVES;
    static BlockType DARK_OAK_LEAVES;
    static BlockType PALE_OAK_LEAVES;
    static BlockType MANGROVE_LEAVES;
    static BlockType AZALEA_LEAVES;
    static BlockType FLOWERING_AZALEA_LEAVES;
    static BlockType SPONGE;
    static BlockType WET_SPONGE;
    static BlockType GLASS;
    static BlockType LAPIS_ORE;
    static BlockType DEEPSLATE_LAPIS_ORE;
    static BlockType LAPIS_BLOCK;
    static BlockType DISPENSER;
    static BlockType SANDSTONE;
    static BlockType CHISELED_SANDSTONE;
    static BlockType CUT_SANDSTONE;
    static BlockType NOTE_BLOCK;
    static BlockType WHITE_BED;
    static BlockType ORANGE_BED;
    static BlockType MAGENTA_BED;
    static BlockType LIGHT_BLUE_BED;
    static BlockType YELLOW_BED;
    static BlockType LIME_BED;
    static BlockType PINK_BED;
    static BlockType GRAY_BED;
    static BlockType LIGHT_GRAY_BED;
    static BlockType CYAN_BED;
    static BlockType PURPLE_BED;
    static BlockType BLUE_BED;
    static BlockType BROWN_BED;
    static BlockType GREEN_BED;
    static BlockType RED_BED;
    static BlockType BLACK_BED;
    static BlockType POWERED_RAIL;
    static BlockType DETECTOR_RAIL;
    static BlockType STICKY_PISTON;
    static BlockType COBWEB;
    static BlockType SHORT_GRASS;
    static BlockType FERN;
    static BlockType DEAD_BUSH;
    static BlockType BUSH;
    static BlockType SHORT_DRY_GRASS;
    static BlockType TALL_DRY_GRASS;
    static BlockType SEAGRASS;
    static BlockType TALL_SEAGRASS;
    static BlockType PISTON;
    static BlockType PISTON_HEAD;
    static BlockType WHITE_WOOL;
    static BlockType ORANGE_WOOL;
    static BlockType MAGENTA_WOOL;
    static BlockType LIGHT_BLUE_WOOL;
    static BlockType YELLOW_WOOL;
    static BlockType LIME_WOOL;
    static BlockType PINK_WOOL;
    static BlockType GRAY_WOOL;
    static BlockType LIGHT_GRAY_WOOL;
    static BlockType CYAN_WOOL;
    static BlockType PURPLE_WOOL;
    static BlockType BLUE_WOOL;
    static BlockType BROWN_WOOL;
    static BlockType GREEN_WOOL;
    static BlockType RED_WOOL;
    static BlockType BLACK_WOOL;
    static BlockType MOVING_PISTON;
    static BlockType DANDELION;
    static BlockType TORCHFLOWER;
    static BlockType POPPY;
    static BlockType BLUE_ORCHID;
    static BlockType ALLIUM;
    static BlockType AZURE_BLUET;
    static BlockType RED_TULIP;
    static BlockType ORANGE_TULIP;
    static BlockType WHITE_TULIP;
    static BlockType PINK_TULIP;
    static BlockType OXEYE_DAISY;
    static BlockType CORNFLOWER;
    static BlockType WITHER_ROSE;
    static BlockType LILY_OF_THE_VALLEY;
    static BlockType BROWN_MUSHROOM;
    static BlockType RED_MUSHROOM;
    static BlockType GOLD_BLOCK;
    static BlockType IRON_BLOCK;
    static BlockType BRICKS;
    static BlockType TNT;
    static BlockType BOOKSHELF;
    static BlockType CHISELED_BOOKSHELF;
    static BlockType MOSSY_COBBLESTONE;
    static BlockType OBSIDIAN;
    static BlockType TORCH;
    static BlockType WALL_TORCH;
    static BlockType FIRE;
    static BlockType SOUL_FIRE;
    static BlockType SPAWNER;
    static BlockType CREAKING_HEART;
    static BlockType OAK_STAIRS;
    static BlockType CHEST;
    static BlockType REDSTONE_WIRE;
    static BlockType DIAMOND_ORE;
    static BlockType DEEPSLATE_DIAMOND_ORE;
    static BlockType DIAMOND_BLOCK;
    static BlockType CRAFTING_TABLE;
    static BlockType WHEAT;
    static BlockType FARMLAND;
    static BlockType FURNACE;
    static BlockType OAK_SIGN;
    static BlockType SPRUCE_SIGN;
    static BlockType BIRCH_SIGN;
    static BlockType ACACIA_SIGN;
    static BlockType CHERRY_SIGN;
    static BlockType JUNGLE_SIGN;
    static BlockType DARK_OAK_SIGN;
    static BlockType PALE_OAK_SIGN;
    static BlockType MANGROVE_SIGN;
    static BlockType BAMBOO_SIGN;
    static BlockType OAK_DOOR;
    static BlockType LADDER;
    static BlockType RAIL;
    static BlockType COBBLESTONE_STAIRS;
    static BlockType OAK_WALL_SIGN;
    static BlockType SPRUCE_WALL_SIGN;
    static BlockType BIRCH_WALL_SIGN;
    static BlockType ACACIA_WALL_SIGN;
    static BlockType CHERRY_WALL_SIGN;
    static BlockType JUNGLE_WALL_SIGN;
    static BlockType DARK_OAK_WALL_SIGN;
    static BlockType PALE_OAK_WALL_SIGN;
    static BlockType MANGROVE_WALL_SIGN;
    static BlockType BAMBOO_WALL_SIGN;
    static BlockType OAK_HANGING_SIGN;
    static BlockType SPRUCE_HANGING_SIGN;
    static BlockType BIRCH_HANGING_SIGN;
    static BlockType ACACIA_HANGING_SIGN;
    static BlockType CHERRY_HANGING_SIGN;
    static BlockType JUNGLE_HANGING_SIGN;
    static BlockType DARK_OAK_HANGING_SIGN;
    static BlockType PALE_OAK_HANGING_SIGN;
    static BlockType CRIMSON_HANGING_SIGN;
    static BlockType WARPED_HANGING_SIGN;
    static BlockType MANGROVE_HANGING_SIGN;
    static BlockType BAMBOO_HANGING_SIGN;
    static BlockType OAK_WALL_HANGING_SIGN;
    static BlockType SPRUCE_WALL_HANGING_SIGN;
    static BlockType BIRCH_WALL_HANGING_SIGN;
    static BlockType ACACIA_WALL_HANGING_SIGN;
    static BlockType CHERRY_WALL_HANGING_SIGN;
    static BlockType JUNGLE_WALL_HANGING_SIGN;
    static BlockType DARK_OAK_WALL_HANGING_SIGN;
    static BlockType PALE_OAK_WALL_HANGING_SIGN;
    static BlockType MANGROVE_WALL_HANGING_SIGN;
    static BlockType CRIMSON_WALL_HANGING_SIGN;
    static BlockType WARPED_WALL_HANGING_SIGN;
    static BlockType BAMBOO_WALL_HANGING_SIGN;
    static BlockType LEVER;
    static BlockType STONE_PRESSURE_PLATE;
    static BlockType IRON_DOOR;
    static BlockType OAK_PRESSURE_PLATE;
    static BlockType SPRUCE_PRESSURE_PLATE;
    static BlockType BIRCH_PRESSURE_PLATE;
    static BlockType JUNGLE_PRESSURE_PLATE;
    static BlockType ACACIA_PRESSURE_PLATE;
    static BlockType CHERRY_PRESSURE_PLATE;
    static BlockType DARK_OAK_PRESSURE_PLATE;
    static BlockType PALE_OAK_PRESSURE_PLATE;
    static BlockType MANGROVE_PRESSURE_PLATE;
    static BlockType BAMBOO_PRESSURE_PLATE;
    static BlockType REDSTONE_ORE;
    static BlockType DEEPSLATE_REDSTONE_ORE;
    static BlockType REDSTONE_TORCH;
    static BlockType REDSTONE_WALL_TORCH;
    static BlockType STONE_BUTTON;
    static BlockType SNOW;
    static BlockType ICE;
    static BlockType SNOW_BLOCK;
    static BlockType CACTUS;
    static BlockType CACTUS_FLOWER;
    static BlockType CLAY;
    static BlockType SUGAR_CANE;
    static BlockType JUKEBOX;
    static BlockType OAK_FENCE;
    static BlockType NETHERRACK;
    static BlockType SOUL_SAND;
    static BlockType SOUL_SOIL;
    static BlockType BASALT;
    static BlockType POLISHED_BASALT;
    static BlockType SOUL_TORCH;
    static BlockType SOUL_WALL_TORCH;
    static BlockType GLOWSTONE;
    static BlockType NETHER_PORTAL;
    static BlockType CARVED_PUMPKIN;
    static BlockType JACK_O_LANTERN;
    static BlockType CAKE;
    static BlockType REPEATER;
    static BlockType WHITE_STAINED_GLASS;
    static BlockType ORANGE_STAINED_GLASS;
    static BlockType MAGENTA_STAINED_GLASS;
    static BlockType LIGHT_BLUE_STAINED_GLASS;
    static BlockType YELLOW_STAINED_GLASS;
    static BlockType LIME_STAINED_GLASS;
    static BlockType PINK_STAINED_GLASS;
    static BlockType GRAY_STAINED_GLASS;
    static BlockType LIGHT_GRAY_STAINED_GLASS;
    static BlockType CYAN_STAINED_GLASS;
    static BlockType PURPLE_STAINED_GLASS;
    static BlockType BLUE_STAINED_GLASS;
    static BlockType BROWN_STAINED_GLASS;
    static BlockType GREEN_STAINED_GLASS;
    static BlockType RED_STAINED_GLASS;
    static BlockType BLACK_STAINED_GLASS;
    static BlockType OAK_TRAPDOOR;
    static BlockType SPRUCE_TRAPDOOR;
    static BlockType BIRCH_TRAPDOOR;
    static BlockType JUNGLE_TRAPDOOR;
    static BlockType ACACIA_TRAPDOOR;
    static BlockType CHERRY_TRAPDOOR;
    static BlockType DARK_OAK_TRAPDOOR;
    static BlockType PALE_OAK_TRAPDOOR;
    static BlockType MANGROVE_TRAPDOOR;
    static BlockType BAMBOO_TRAPDOOR;
    static BlockType STONE_BRICKS;
    static BlockType MOSSY_STONE_BRICKS;
    static BlockType CRACKED_STONE_BRICKS;
    static BlockType CHISELED_STONE_BRICKS;
    static BlockType PACKED_MUD;
    static BlockType MUD_BRICKS;
    static BlockType INFESTED_STONE;
    static BlockType INFESTED_COBBLESTONE;
    static BlockType INFESTED_STONE_BRICKS;
    static BlockType INFESTED_MOSSY_STONE_BRICKS;
    static BlockType INFESTED_CRACKED_STONE_BRICKS;
    static BlockType INFESTED_CHISELED_STONE_BRICKS;
    static BlockType BROWN_MUSHROOM_BLOCK;
    static BlockType RED_MUSHROOM_BLOCK;
    static BlockType MUSHROOM_STEM;
    static BlockType IRON_BARS;
    static BlockType CHAIN;
    static BlockType GLASS_PANE;
    static BlockType PUMPKIN;
    static BlockType MELON;
    static BlockType ATTACHED_PUMPKIN_STEM;
    static BlockType ATTACHED_MELON_STEM;
    static BlockType PUMPKIN_STEM;
    static BlockType MELON_STEM;
    static BlockType VINE;
    static BlockType GLOW_LICHEN;
    static BlockType RESIN_CLUMP;
    static BlockType OAK_FENCE_GATE;
    static BlockType BRICK_STAIRS;
    static BlockType STONE_BRICK_STAIRS;
    static BlockType MUD_BRICK_STAIRS;
    static BlockType MYCELIUM;
    static BlockType LILY_PAD;
    static BlockType RESIN_BLOCK;
    static BlockType RESIN_BRICKS;
    static BlockType RESIN_BRICK_STAIRS;
    static BlockType RESIN_BRICK_SLAB;
    static BlockType RESIN_BRICK_WALL;
    static BlockType CHISELED_RESIN_BRICKS;
    static BlockType NETHER_BRICKS;
    static BlockType NETHER_BRICK_FENCE;
    static BlockType NETHER_BRICK_STAIRS;
    static BlockType NETHER_WART;
    static BlockType ENCHANTING_TABLE;
    static BlockType BREWING_STAND;
    static BlockType CAULDRON;
    static BlockType WATER_CAULDRON;
    static BlockType LAVA_CAULDRON;
    static BlockType POWDER_SNOW_CAULDRON;
    static BlockType END_PORTAL;
    static BlockType END_PORTAL_FRAME;
    static BlockType END_STONE;
    static BlockType DRAGON_EGG;
    static BlockType REDSTONE_LAMP;
    static BlockType COCOA;
    static BlockType SANDSTONE_STAIRS;
    static BlockType EMERALD_ORE;
    static BlockType DEEPSLATE_EMERALD_ORE;
    static BlockType ENDER_CHEST;
    static BlockType TRIPWIRE_HOOK;
    static BlockType TRIPWIRE;
    static BlockType EMERALD_BLOCK;
    static BlockType SPRUCE_STAIRS;
    static BlockType BIRCH_STAIRS;
    static BlockType JUNGLE_STAIRS;
    static BlockType COMMAND_BLOCK;
    static BlockType BEACON;
    static BlockType COBBLESTONE_WALL;
    static BlockType MOSSY_COBBLESTONE_WALL;
    static BlockType FLOWER_POT;
    static BlockType POTTED_TORCHFLOWER;
    static BlockType POTTED_OAK_SAPLING;
    static BlockType POTTED_SPRUCE_SAPLING;
    static BlockType POTTED_BIRCH_SAPLING;
    static BlockType POTTED_JUNGLE_SAPLING;
    static BlockType POTTED_ACACIA_SAPLING;
    static BlockType POTTED_CHERRY_SAPLING;
    static BlockType POTTED_DARK_OAK_SAPLING;
    static BlockType POTTED_PALE_OAK_SAPLING;
    static BlockType POTTED_MANGROVE_PROPAGULE;
    static BlockType POTTED_FERN;
    static BlockType POTTED_DANDELION;
    static BlockType POTTED_POPPY;
    static BlockType POTTED_BLUE_ORCHID;
    static BlockType POTTED_ALLIUM;
    static BlockType POTTED_AZURE_BLUET;
    static BlockType POTTED_RED_TULIP;
    static BlockType POTTED_ORANGE_TULIP;
    static BlockType POTTED_WHITE_TULIP;
    static BlockType POTTED_PINK_TULIP;
    static BlockType POTTED_OXEYE_DAISY;
    static BlockType POTTED_CORNFLOWER;
    static BlockType POTTED_LILY_OF_THE_VALLEY;
    static BlockType POTTED_WITHER_ROSE;
    static BlockType POTTED_RED_MUSHROOM;
    static BlockType POTTED_BROWN_MUSHROOM;
    static BlockType POTTED_DEAD_BUSH;
    static BlockType POTTED_CACTUS;
    static BlockType CARROTS;
    static BlockType POTATOES;
    static BlockType OAK_BUTTON;
    static BlockType SPRUCE_BUTTON;
    static BlockType BIRCH_BUTTON;
    static BlockType JUNGLE_BUTTON;
    static BlockType ACACIA_BUTTON;
    static BlockType CHERRY_BUTTON;
    static BlockType DARK_OAK_BUTTON;
    static BlockType PALE_OAK_BUTTON;
    static BlockType MANGROVE_BUTTON;
    static BlockType BAMBOO_BUTTON;
    static BlockType SKELETON_SKULL;
    static BlockType SKELETON_WALL_SKULL;
    static BlockType WITHER_SKELETON_SKULL;
    static BlockType WITHER_SKELETON_WALL_SKULL;
    static BlockType ZOMBIE_HEAD;
    static BlockType ZOMBIE_WALL_HEAD;
    static BlockType PLAYER_HEAD;
    static BlockType PLAYER_WALL_HEAD;
    static BlockType CREEPER_HEAD;
    static BlockType CREEPER_WALL_HEAD;
    static BlockType DRAGON_HEAD;
    static BlockType DRAGON_WALL_HEAD;
    static BlockType PIGLIN_HEAD;
    static BlockType PIGLIN_WALL_HEAD;
    static BlockType ANVIL;
    static BlockType CHIPPED_ANVIL;
    static BlockType DAMAGED_ANVIL;
    static BlockType TRAPPED_CHEST;
    static BlockType LIGHT_WEIGHTED_PRESSURE_PLATE;
    static BlockType HEAVY_WEIGHTED_PRESSURE_PLATE;
    static BlockType COMPARATOR;
    static BlockType DAYLIGHT_DETECTOR;
    static BlockType REDSTONE_BLOCK;
    static BlockType NETHER_QUARTZ_ORE;
    static BlockType HOPPER;
    static BlockType QUARTZ_BLOCK;
    static BlockType CHISELED_QUARTZ_BLOCK;
    static BlockType QUARTZ_PILLAR;
    static BlockType QUARTZ_STAIRS;
    static BlockType ACTIVATOR_RAIL;
    static BlockType DROPPER;
    static BlockType WHITE_TERRACOTTA;
    static BlockType ORANGE_TERRACOTTA;
    static BlockType MAGENTA_TERRACOTTA;
    static BlockType LIGHT_BLUE_TERRACOTTA;
    static BlockType YELLOW_TERRACOTTA;
    static BlockType LIME_TERRACOTTA;
    static BlockType PINK_TERRACOTTA;
    static BlockType GRAY_TERRACOTTA;
    static BlockType LIGHT_GRAY_TERRACOTTA;
    static BlockType CYAN_TERRACOTTA;
    static BlockType PURPLE_TERRACOTTA;
    static BlockType BLUE_TERRACOTTA;
    static BlockType BROWN_TERRACOTTA;
    static BlockType GREEN_TERRACOTTA;
    static BlockType RED_TERRACOTTA;
    static BlockType BLACK_TERRACOTTA;
    static BlockType WHITE_STAINED_GLASS_PANE;
    static BlockType ORANGE_STAINED_GLASS_PANE;
    static BlockType MAGENTA_STAINED_GLASS_PANE;
    static BlockType LIGHT_BLUE_STAINED_GLASS_PANE;
    static BlockType YELLOW_STAINED_GLASS_PANE;
    static BlockType LIME_STAINED_GLASS_PANE;
    static BlockType PINK_STAINED_GLASS_PANE;
    static BlockType GRAY_STAINED_GLASS_PANE;
    static BlockType LIGHT_GRAY_STAINED_GLASS_PANE;
    static BlockType CYAN_STAINED_GLASS_PANE;
    static BlockType PURPLE_STAINED_GLASS_PANE;
    static BlockType BLUE_STAINED_GLASS_PANE;
    static BlockType BROWN_STAINED_GLASS_PANE;
    static BlockType GREEN_STAINED_GLASS_PANE;
    static BlockType RED_STAINED_GLASS_PANE;
    static BlockType BLACK_STAINED_GLASS_PANE;
    static BlockType ACACIA_STAIRS;
    static BlockType CHERRY_STAIRS;
    static BlockType DARK_OAK_STAIRS;
    static BlockType PALE_OAK_STAIRS;
    static BlockType MANGROVE_STAIRS;
    static BlockType BAMBOO_STAIRS;
    static BlockType BAMBOO_MOSAIC_STAIRS;
    static BlockType SLIME_BLOCK;
    static BlockType BARRIER;
    static BlockType LIGHT;
    static BlockType IRON_TRAPDOOR;
    static BlockType PRISMARINE;
    static BlockType PRISMARINE_BRICKS;
    static BlockType DARK_PRISMARINE;
    static BlockType PRISMARINE_STAIRS;
    static BlockType PRISMARINE_BRICK_STAIRS;
    static BlockType DARK_PRISMARINE_STAIRS;
    static BlockType PRISMARINE_SLAB;
    static BlockType PRISMARINE_BRICK_SLAB;
    static BlockType DARK_PRISMARINE_SLAB;
    static BlockType SEA_LANTERN;
    static BlockType HAY_BLOCK;
    static BlockType WHITE_CARPET;
    static BlockType ORANGE_CARPET;
    static BlockType MAGENTA_CARPET;
    static BlockType LIGHT_BLUE_CARPET;
    static BlockType YELLOW_CARPET;
    static BlockType LIME_CARPET;
    static BlockType PINK_CARPET;
    static BlockType GRAY_CARPET;
    static BlockType LIGHT_GRAY_CARPET;
    static BlockType CYAN_CARPET;
    static BlockType PURPLE_CARPET;
    static BlockType BLUE_CARPET;
    static BlockType BROWN_CARPET;
    static BlockType GREEN_CARPET;
    static BlockType RED_CARPET;
    static BlockType BLACK_CARPET;
    static BlockType TERRACOTTA;
    static BlockType COAL_BLOCK;
    static BlockType PACKED_ICE;
    static BlockType SUNFLOWER;
    static BlockType LILAC;
    static BlockType ROSE_BUSH;
    static BlockType PEONY;
    static BlockType TALL_GRASS;
    static BlockType LARGE_FERN;
    static BlockType WHITE_BANNER;
    static BlockType ORANGE_BANNER;
    static BlockType MAGENTA_BANNER;
    static BlockType LIGHT_BLUE_BANNER;
    static BlockType YELLOW_BANNER;
    static BlockType LIME_BANNER;
    static BlockType PINK_BANNER;
    static BlockType GRAY_BANNER;
    static BlockType LIGHT_GRAY_BANNER;
    static BlockType CYAN_BANNER;
    static BlockType PURPLE_BANNER;
    static BlockType BLUE_BANNER;
    static BlockType BROWN_BANNER;
    static BlockType GREEN_BANNER;
    static BlockType RED_BANNER;
    static BlockType BLACK_BANNER;
    static BlockType WHITE_WALL_BANNER;
    static BlockType ORANGE_WALL_BANNER;
    static BlockType MAGENTA_WALL_BANNER;
    static BlockType LIGHT_BLUE_WALL_BANNER;
    static BlockType YELLOW_WALL_BANNER;
    static BlockType LIME_WALL_BANNER;
    static BlockType PINK_WALL_BANNER;
    static BlockType GRAY_WALL_BANNER;
    static BlockType LIGHT_GRAY_WALL_BANNER;
    static BlockType CYAN_WALL_BANNER;
    static BlockType PURPLE_WALL_BANNER;
    static BlockType BLUE_WALL_BANNER;
    static BlockType BROWN_WALL_BANNER;
    static BlockType GREEN_WALL_BANNER;
    static BlockType RED_WALL_BANNER;
    static BlockType BLACK_WALL_BANNER;
    static BlockType RED_SANDSTONE;
    static BlockType CHISELED_RED_SANDSTONE;
    static BlockType CUT_RED_SANDSTONE;
    static BlockType RED_SANDSTONE_STAIRS;
    static BlockType OAK_SLAB;
    static BlockType SPRUCE_SLAB;
    static BlockType BIRCH_SLAB;
    static BlockType JUNGLE_SLAB;
    static BlockType ACACIA_SLAB;
    static BlockType CHERRY_SLAB;
    static BlockType DARK_OAK_SLAB;
    static BlockType PALE_OAK_SLAB;
    static BlockType MANGROVE_SLAB;
    static BlockType BAMBOO_SLAB;
    static BlockType BAMBOO_MOSAIC_SLAB;
    static BlockType STONE_SLAB;
    static BlockType SMOOTH_STONE_SLAB;
    static BlockType SANDSTONE_SLAB;
    static BlockType CUT_SANDSTONE_SLAB;
    static BlockType PETRIFIED_OAK_SLAB;
    static BlockType COBBLESTONE_SLAB;
    static BlockType BRICK_SLAB;
    static BlockType STONE_BRICK_SLAB;
    static BlockType MUD_BRICK_SLAB;
    static BlockType NETHER_BRICK_SLAB;
    static BlockType QUARTZ_SLAB;
    static BlockType RED_SANDSTONE_SLAB;
    static BlockType CUT_RED_SANDSTONE_SLAB;
    static BlockType PURPUR_SLAB;
    static BlockType SMOOTH_STONE;
    static BlockType SMOOTH_SANDSTONE;
    static BlockType SMOOTH_QUARTZ;
    static BlockType SMOOTH_RED_SANDSTONE;
    static BlockType SPRUCE_FENCE_GATE;
    static BlockType BIRCH_FENCE_GATE;
    static BlockType JUNGLE_FENCE_GATE;
    static BlockType ACACIA_FENCE_GATE;
    static BlockType CHERRY_FENCE_GATE;
    static BlockType DARK_OAK_FENCE_GATE;
    static BlockType PALE_OAK_FENCE_GATE;
    static BlockType MANGROVE_FENCE_GATE;
    static BlockType BAMBOO_FENCE_GATE;
    static BlockType SPRUCE_FENCE;
    static BlockType BIRCH_FENCE;
    static BlockType JUNGLE_FENCE;
    static BlockType ACACIA_FENCE;
    static BlockType CHERRY_FENCE;
    static BlockType DARK_OAK_FENCE;
    static BlockType PALE_OAK_FENCE;
    static BlockType MANGROVE_FENCE;
    static BlockType BAMBOO_FENCE;
    static BlockType SPRUCE_DOOR;
    static BlockType BIRCH_DOOR;
    static BlockType JUNGLE_DOOR;
    static BlockType ACACIA_DOOR;
    static BlockType CHERRY_DOOR;
    static BlockType DARK_OAK_DOOR;
    static BlockType PALE_OAK_DOOR;
    static BlockType MANGROVE_DOOR;
    static BlockType BAMBOO_DOOR;
    static BlockType END_ROD;
    static BlockType CHORUS_PLANT;
    static BlockType CHORUS_FLOWER;
    static BlockType PURPUR_BLOCK;
    static BlockType PURPUR_PILLAR;
    static BlockType PURPUR_STAIRS;
    static BlockType END_STONE_BRICKS;
    static BlockType TORCHFLOWER_CROP;
    static BlockType PITCHER_CROP;
    static BlockType PITCHER_PLANT;
    static BlockType BEETROOTS;
    static BlockType DIRT_PATH;
    static BlockType END_GATEWAY;
    static BlockType REPEATING_COMMAND_BLOCK;
    static BlockType CHAIN_COMMAND_BLOCK;
    static BlockType FROSTED_ICE;
    static BlockType MAGMA_BLOCK;
    static BlockType NETHER_WART_BLOCK;
    static BlockType RED_NETHER_BRICKS;
    static BlockType BONE_BLOCK;
    static BlockType STRUCTURE_VOID;
    static BlockType OBSERVER;
    static BlockType SHULKER_BOX;
    static BlockType WHITE_SHULKER_BOX;
    static BlockType ORANGE_SHULKER_BOX;
    static BlockType MAGENTA_SHULKER_BOX;
    static BlockType LIGHT_BLUE_SHULKER_BOX;
    static BlockType YELLOW_SHULKER_BOX;
    static BlockType LIME_SHULKER_BOX;
    static BlockType PINK_SHULKER_BOX;
    static BlockType GRAY_SHULKER_BOX;
    static BlockType LIGHT_GRAY_SHULKER_BOX;
    static BlockType CYAN_SHULKER_BOX;
    static BlockType PURPLE_SHULKER_BOX;
    static BlockType BLUE_SHULKER_BOX;
    static BlockType BROWN_SHULKER_BOX;
    static BlockType GREEN_SHULKER_BOX;
    static BlockType RED_SHULKER_BOX;
    static BlockType BLACK_SHULKER_BOX;
    static BlockType WHITE_GLAZED_TERRACOTTA;
    static BlockType ORANGE_GLAZED_TERRACOTTA;
    static BlockType MAGENTA_GLAZED_TERRACOTTA;
    static BlockType LIGHT_BLUE_GLAZED_TERRACOTTA;
    static BlockType YELLOW_GLAZED_TERRACOTTA;
    static BlockType LIME_GLAZED_TERRACOTTA;
    static BlockType PINK_GLAZED_TERRACOTTA;
    static BlockType GRAY_GLAZED_TERRACOTTA;
    static BlockType LIGHT_GRAY_GLAZED_TERRACOTTA;
    static BlockType CYAN_GLAZED_TERRACOTTA;
    static BlockType PURPLE_GLAZED_TERRACOTTA;
    static BlockType BLUE_GLAZED_TERRACOTTA;
    static BlockType BROWN_GLAZED_TERRACOTTA;
    static BlockType GREEN_GLAZED_TERRACOTTA;
    static BlockType RED_GLAZED_TERRACOTTA;
    static BlockType BLACK_GLAZED_TERRACOTTA;
    static BlockType WHITE_CONCRETE;
    static BlockType ORANGE_CONCRETE;
    static BlockType MAGENTA_CONCRETE;
    static BlockType LIGHT_BLUE_CONCRETE;
    static BlockType YELLOW_CONCRETE;
    static BlockType LIME_CONCRETE;
    static BlockType PINK_CONCRETE;
    static BlockType GRAY_CONCRETE;
    static BlockType LIGHT_GRAY_CONCRETE;
    static BlockType CYAN_CONCRETE;
    static BlockType PURPLE_CONCRETE;
    static BlockType BLUE_CONCRETE;
    static BlockType BROWN_CONCRETE;
    static BlockType GREEN_CONCRETE;
    static BlockType RED_CONCRETE;
    static BlockType BLACK_CONCRETE;
    static BlockType WHITE_CONCRETE_POWDER;
    static BlockType ORANGE_CONCRETE_POWDER;
    static BlockType MAGENTA_CONCRETE_POWDER;
    static BlockType LIGHT_BLUE_CONCRETE_POWDER;
    static BlockType YELLOW_CONCRETE_POWDER;
    static BlockType LIME_CONCRETE_POWDER;
    static BlockType PINK_CONCRETE_POWDER;
    static BlockType GRAY_CONCRETE_POWDER;
    static BlockType LIGHT_GRAY_CONCRETE_POWDER;
    static BlockType CYAN_CONCRETE_POWDER;
    static BlockType PURPLE_CONCRETE_POWDER;
    static BlockType BLUE_CONCRETE_POWDER;
    static BlockType BROWN_CONCRETE_POWDER;
    static BlockType GREEN_CONCRETE_POWDER;
    static BlockType RED_CONCRETE_POWDER;
    static BlockType BLACK_CONCRETE_POWDER;
    static BlockType KELP;
    static BlockType KELP_PLANT;
    static BlockType DRIED_KELP_BLOCK;
    static BlockType TURTLE_EGG;
    static BlockType SNIFFER_EGG;
    static BlockType DEAD_TUBE_CORAL_BLOCK;
    static BlockType DEAD_BRAIN_CORAL_BLOCK;
    static BlockType DEAD_BUBBLE_CORAL_BLOCK;
    static BlockType DEAD_FIRE_CORAL_BLOCK;
    static BlockType DEAD_HORN_CORAL_BLOCK;
    static BlockType TUBE_CORAL_BLOCK;
    static BlockType BRAIN_CORAL_BLOCK;
    static BlockType BUBBLE_CORAL_BLOCK;
    static BlockType FIRE_CORAL_BLOCK;
    static BlockType HORN_CORAL_BLOCK;
    static BlockType DEAD_TUBE_CORAL;
    static BlockType DEAD_BRAIN_CORAL;
    static BlockType DEAD_BUBBLE_CORAL;
    static BlockType DEAD_FIRE_CORAL;
    static BlockType DEAD_HORN_CORAL;
    static BlockType TUBE_CORAL;
    static BlockType BRAIN_CORAL;
    static BlockType BUBBLE_CORAL;
    static BlockType FIRE_CORAL;
    static BlockType HORN_CORAL;
    static BlockType DEAD_TUBE_CORAL_FAN;
    static BlockType DEAD_BRAIN_CORAL_FAN;
    static BlockType DEAD_BUBBLE_CORAL_FAN;
    static BlockType DEAD_FIRE_CORAL_FAN;
    static BlockType DEAD_HORN_CORAL_FAN;
    static BlockType TUBE_CORAL_FAN;
    static BlockType BRAIN_CORAL_FAN;
    static BlockType BUBBLE_CORAL_FAN;
    static BlockType FIRE_CORAL_FAN;
    static BlockType HORN_CORAL_FAN;
    static BlockType DEAD_TUBE_CORAL_WALL_FAN;
    static BlockType DEAD_BRAIN_CORAL_WALL_FAN;
    static BlockType DEAD_BUBBLE_CORAL_WALL_FAN;
    static BlockType DEAD_FIRE_CORAL_WALL_FAN;
    static BlockType DEAD_HORN_CORAL_WALL_FAN;
    static BlockType TUBE_CORAL_WALL_FAN;
    static BlockType BRAIN_CORAL_WALL_FAN;
    static BlockType BUBBLE_CORAL_WALL_FAN;
    static BlockType FIRE_CORAL_WALL_FAN;
    static BlockType HORN_CORAL_WALL_FAN;
    static BlockType SEA_PICKLE;
    static BlockType BLUE_ICE;
    static BlockType CONDUIT;
    static BlockType BAMBOO_SAPLING;
    static BlockType BAMBOO;
    static BlockType POTTED_BAMBOO;
    static BlockType VOID_AIR;
    static BlockType CAVE_AIR;
    static BlockType BUBBLE_COLUMN;
    static BlockType POLISHED_GRANITE_STAIRS;
    static BlockType SMOOTH_RED_SANDSTONE_STAIRS;
    static BlockType MOSSY_STONE_BRICK_STAIRS;
    static BlockType POLISHED_DIORITE_STAIRS;
    static BlockType MOSSY_COBBLESTONE_STAIRS;
    static BlockType END_STONE_BRICK_STAIRS;
    static BlockType STONE_STAIRS;
    static BlockType SMOOTH_SANDSTONE_STAIRS;
    static BlockType SMOOTH_QUARTZ_STAIRS;
    static BlockType GRANITE_STAIRS;
    static BlockType ANDESITE_STAIRS;
    static BlockType RED_NETHER_BRICK_STAIRS;
    static BlockType POLISHED_ANDESITE_STAIRS;
    static BlockType DIORITE_STAIRS;
    static BlockType POLISHED_GRANITE_SLAB;
    static BlockType SMOOTH_RED_SANDSTONE_SLAB;
    static BlockType MOSSY_STONE_BRICK_SLAB;
    static BlockType POLISHED_DIORITE_SLAB;
    static BlockType MOSSY_COBBLESTONE_SLAB;
    static BlockType END_STONE_BRICK_SLAB;
    static BlockType SMOOTH_SANDSTONE_SLAB;
    static BlockType SMOOTH_QUARTZ_SLAB;
    static BlockType GRANITE_SLAB;
    static BlockType ANDESITE_SLAB;
    static BlockType RED_NETHER_BRICK_SLAB;
    static BlockType POLISHED_ANDESITE_SLAB;
    static BlockType DIORITE_SLAB;
    static BlockType BRICK_WALL;
    static BlockType PRISMARINE_WALL;
    static BlockType RED_SANDSTONE_WALL;
    static BlockType MOSSY_STONE_BRICK_WALL;
    static BlockType GRANITE_WALL;
    static BlockType STONE_BRICK_WALL;
    static BlockType MUD_BRICK_WALL;
    static BlockType NETHER_BRICK_WALL;
    static BlockType ANDESITE_WALL;
    static BlockType RED_NETHER_BRICK_WALL;
    static BlockType SANDSTONE_WALL;
    static BlockType END_STONE_BRICK_WALL;
    static BlockType DIORITE_WALL;
    static BlockType SCAFFOLDING;
    static BlockType LOOM;
    static BlockType BARREL;
    static BlockType SMOKER;
    static BlockType BLAST_FURNACE;
    static BlockType CARTOGRAPHY_TABLE;
    static BlockType FLETCHING_TABLE;
    static BlockType GRINDSTONE;
    static BlockType LECTERN;
    static BlockType SMITHING_TABLE;
    static BlockType STONECUTTER;
    static BlockType BELL;
    static BlockType LANTERN;
    static BlockType SOUL_LANTERN;
    static BlockType CAMPFIRE;
    static BlockType SOUL_CAMPFIRE;
    static BlockType SWEET_BERRY_BUSH;
    static BlockType WARPED_STEM;
    static BlockType STRIPPED_WARPED_STEM;
    static BlockType WARPED_HYPHAE;
    static BlockType STRIPPED_WARPED_HYPHAE;
    static BlockType WARPED_NYLIUM;
    static BlockType WARPED_FUNGUS;
    static BlockType WARPED_WART_BLOCK;
    static BlockType WARPED_ROOTS;
    static BlockType NETHER_SPROUTS;
    static BlockType CRIMSON_STEM;
    static BlockType STRIPPED_CRIMSON_STEM;
    static BlockType CRIMSON_HYPHAE;
    static BlockType STRIPPED_CRIMSON_HYPHAE;
    static BlockType CRIMSON_NYLIUM;
    static BlockType CRIMSON_FUNGUS;
    static BlockType SHROOMLIGHT;
    static BlockType WEEPING_VINES;
    static BlockType WEEPING_VINES_PLANT;
    static BlockType TWISTING_VINES;
    static BlockType TWISTING_VINES_PLANT;
    static BlockType CRIMSON_ROOTS;
    static BlockType CRIMSON_PLANKS;
    static BlockType WARPED_PLANKS;
    static BlockType CRIMSON_SLAB;
    static BlockType WARPED_SLAB;
    static BlockType CRIMSON_PRESSURE_PLATE;
    static BlockType WARPED_PRESSURE_PLATE;
    static BlockType CRIMSON_FENCE;
    static BlockType WARPED_FENCE;
    static BlockType CRIMSON_TRAPDOOR;
    static BlockType WARPED_TRAPDOOR;
    static BlockType CRIMSON_FENCE_GATE;
    static BlockType WARPED_FENCE_GATE;
    static BlockType CRIMSON_STAIRS;
    static BlockType WARPED_STAIRS;
    static BlockType CRIMSON_BUTTON;
    static BlockType WARPED_BUTTON;
    static BlockType CRIMSON_DOOR;
    static BlockType WARPED_DOOR;
    static BlockType CRIMSON_SIGN;
    static BlockType WARPED_SIGN;
    static BlockType CRIMSON_WALL_SIGN;
    static BlockType WARPED_WALL_SIGN;
    static BlockType STRUCTURE_BLOCK;
    static BlockType JIGSAW;
    static BlockType TEST_BLOCK;
    static BlockType TEST_INSTANCE_BLOCK;
    static BlockType COMPOSTER;
    static BlockType TARGET;
    static BlockType BEE_NEST;
    static BlockType BEEHIVE;
    static BlockType HONEY_BLOCK;
    static BlockType HONEYCOMB_BLOCK;
    static BlockType NETHERITE_BLOCK;
    static BlockType ANCIENT_DEBRIS;
    static BlockType CRYING_OBSIDIAN;
    static BlockType RESPAWN_ANCHOR;
    static BlockType POTTED_CRIMSON_FUNGUS;
    static BlockType POTTED_WARPED_FUNGUS;
    static BlockType POTTED_CRIMSON_ROOTS;
    static BlockType POTTED_WARPED_ROOTS;
    static BlockType LODESTONE;
    static BlockType BLACKSTONE;
    static BlockType BLACKSTONE_STAIRS;
    static BlockType BLACKSTONE_WALL;
    static BlockType BLACKSTONE_SLAB;
    static BlockType POLISHED_BLACKSTONE;
    static BlockType POLISHED_BLACKSTONE_BRICKS;
    static BlockType CRACKED_POLISHED_BLACKSTONE_BRICKS;
    static BlockType CHISELED_POLISHED_BLACKSTONE;
    static BlockType POLISHED_BLACKSTONE_BRICK_SLAB;
    static BlockType POLISHED_BLACKSTONE_BRICK_STAIRS;
    static BlockType POLISHED_BLACKSTONE_BRICK_WALL;
    static BlockType GILDED_BLACKSTONE;
    static BlockType POLISHED_BLACKSTONE_STAIRS;
    static BlockType POLISHED_BLACKSTONE_SLAB;
    static BlockType POLISHED_BLACKSTONE_PRESSURE_PLATE;
    static BlockType POLISHED_BLACKSTONE_BUTTON;
    static BlockType POLISHED_BLACKSTONE_WALL;
    static BlockType CHISELED_NETHER_BRICKS;
    static BlockType CRACKED_NETHER_BRICKS;
    static BlockType QUARTZ_BRICKS;
    static BlockType CANDLE;
    static BlockType WHITE_CANDLE;
    static BlockType ORANGE_CANDLE;
    static BlockType MAGENTA_CANDLE;
    static BlockType LIGHT_BLUE_CANDLE;
    static BlockType YELLOW_CANDLE;
    static BlockType LIME_CANDLE;
    static BlockType PINK_CANDLE;
    static BlockType GRAY_CANDLE;
    static BlockType LIGHT_GRAY_CANDLE;
    static BlockType CYAN_CANDLE;
    static BlockType PURPLE_CANDLE;
    static BlockType BLUE_CANDLE;
    static BlockType BROWN_CANDLE;
    static BlockType GREEN_CANDLE;
    static BlockType RED_CANDLE;
    static BlockType BLACK_CANDLE;
    static BlockType CANDLE_CAKE;
    static BlockType WHITE_CANDLE_CAKE;
    static BlockType ORANGE_CANDLE_CAKE;
    static BlockType MAGENTA_CANDLE_CAKE;
    static BlockType LIGHT_BLUE_CANDLE_CAKE;
    static BlockType YELLOW_CANDLE_CAKE;
    static BlockType LIME_CANDLE_CAKE;
    static BlockType PINK_CANDLE_CAKE;
    static BlockType GRAY_CANDLE_CAKE;
    static BlockType LIGHT_GRAY_CANDLE_CAKE;
    static BlockType CYAN_CANDLE_CAKE;
    static BlockType PURPLE_CANDLE_CAKE;
    static BlockType BLUE_CANDLE_CAKE;
    static BlockType BROWN_CANDLE_CAKE;
    static BlockType GREEN_CANDLE_CAKE;
    static BlockType RED_CANDLE_CAKE;
    static BlockType BLACK_CANDLE_CAKE;
    static BlockType AMETHYST_BLOCK;
    static BlockType BUDDING_AMETHYST;
    static BlockType AMETHYST_CLUSTER;
    static BlockType LARGE_AMETHYST_BUD;
    static BlockType MEDIUM_AMETHYST_BUD;
    static BlockType SMALL_AMETHYST_BUD;
    static BlockType TUFF;
    static BlockType TUFF_SLAB;
    static BlockType TUFF_STAIRS;
    static BlockType TUFF_WALL;
    static BlockType POLISHED_TUFF;
    static BlockType POLISHED_TUFF_SLAB;
    static BlockType POLISHED_TUFF_STAIRS;
    static BlockType POLISHED_TUFF_WALL;
    static BlockType CHISELED_TUFF;
    static BlockType TUFF_BRICKS;
    static BlockType TUFF_BRICK_SLAB;
    static BlockType TUFF_BRICK_STAIRS;
    static BlockType TUFF_BRICK_WALL;
    static BlockType CHISELED_TUFF_BRICKS;
    static BlockType CALCITE;
    static BlockType TINTED_GLASS;
    static BlockType POWDER_SNOW;
    static BlockType SCULK_SENSOR;
    static BlockType CALIBRATED_SCULK_SENSOR;
    static BlockType SCULK;
    static BlockType SCULK_VEIN;
    static BlockType SCULK_CATALYST;
    static BlockType SCULK_SHRIEKER;
    static BlockType COPPER_BLOCK;
    static BlockType EXPOSED_COPPER;
    static BlockType WEATHERED_COPPER;
    static BlockType OXIDIZED_COPPER;
    static BlockType COPPER_ORE;
    static BlockType DEEPSLATE_COPPER_ORE;
    static BlockType OXIDIZED_CUT_COPPER;
    static BlockType WEATHERED_CUT_COPPER;
    static BlockType EXPOSED_CUT_COPPER;
    static BlockType CUT_COPPER;
    static BlockType OXIDIZED_CHISELED_COPPER;
    static BlockType WEATHERED_CHISELED_COPPER;
    static BlockType EXPOSED_CHISELED_COPPER;
    static BlockType CHISELED_COPPER;
    static BlockType WAXED_OXIDIZED_CHISELED_COPPER;
    static BlockType WAXED_WEATHERED_CHISELED_COPPER;
    static BlockType WAXED_EXPOSED_CHISELED_COPPER;
    static BlockType WAXED_CHISELED_COPPER;
    static BlockType OXIDIZED_CUT_COPPER_STAIRS;
    static BlockType WEATHERED_CUT_COPPER_STAIRS;
    static BlockType EXPOSED_CUT_COPPER_STAIRS;
    static BlockType CUT_COPPER_STAIRS;
    static BlockType OXIDIZED_CUT_COPPER_SLAB;
    static BlockType WEATHERED_CUT_COPPER_SLAB;
    static BlockType EXPOSED_CUT_COPPER_SLAB;
    static BlockType CUT_COPPER_SLAB;
    static BlockType WAXED_COPPER_BLOCK;
    static BlockType WAXED_WEATHERED_COPPER;
    static BlockType WAXED_EXPOSED_COPPER;
    static BlockType WAXED_OXIDIZED_COPPER;
    static BlockType WAXED_OXIDIZED_CUT_COPPER;
    static BlockType WAXED_WEATHERED_CUT_COPPER;
    static BlockType WAXED_EXPOSED_CUT_COPPER;
    static BlockType WAXED_CUT_COPPER;
    static BlockType WAXED_OXIDIZED_CUT_COPPER_STAIRS;
    static BlockType WAXED_WEATHERED_CUT_COPPER_STAIRS;
    static BlockType WAXED_EXPOSED_CUT_COPPER_STAIRS;
    static BlockType WAXED_CUT_COPPER_STAIRS;
    static BlockType WAXED_OXIDIZED_CUT_COPPER_SLAB;
    static BlockType WAXED_WEATHERED_CUT_COPPER_SLAB;
    static BlockType WAXED_EXPOSED_CUT_COPPER_SLAB;
    static BlockType WAXED_CUT_COPPER_SLAB;
    static BlockType COPPER_DOOR;
    static BlockType EXPOSED_COPPER_DOOR;
    static BlockType OXIDIZED_COPPER_DOOR;
    static BlockType WEATHERED_COPPER_DOOR;
    static BlockType WAXED_COPPER_DOOR;
    static BlockType WAXED_EXPOSED_COPPER_DOOR;
    static BlockType WAXED_OXIDIZED_COPPER_DOOR;
    static BlockType WAXED_WEATHERED_COPPER_DOOR;
    static BlockType COPPER_TRAPDOOR;
    static BlockType EXPOSED_COPPER_TRAPDOOR;
    static BlockType OXIDIZED_COPPER_TRAPDOOR;
    static BlockType WEATHERED_COPPER_TRAPDOOR;
    static BlockType WAXED_COPPER_TRAPDOOR;
    static BlockType WAXED_EXPOSED_COPPER_TRAPDOOR;
    static BlockType WAXED_OXIDIZED_COPPER_TRAPDOOR;
    static BlockType WAXED_WEATHERED_COPPER_TRAPDOOR;
    static BlockType COPPER_GRATE;
    static BlockType EXPOSED_COPPER_GRATE;
    static BlockType WEATHERED_COPPER_GRATE;
    static BlockType OXIDIZED_COPPER_GRATE;
    static BlockType WAXED_COPPER_GRATE;
    static BlockType WAXED_EXPOSED_COPPER_GRATE;
    static BlockType WAXED_WEATHERED_COPPER_GRATE;
    static BlockType WAXED_OXIDIZED_COPPER_GRATE;
    static BlockType COPPER_BULB;
    static BlockType EXPOSED_COPPER_BULB;
    static BlockType WEATHERED_COPPER_BULB;
    static BlockType OXIDIZED_COPPER_BULB;
    static BlockType WAXED_COPPER_BULB;
    static BlockType WAXED_EXPOSED_COPPER_BULB;
    static BlockType WAXED_WEATHERED_COPPER_BULB;
    static BlockType WAXED_OXIDIZED_COPPER_BULB;
    static BlockType LIGHTNING_ROD;
    static BlockType POINTED_DRIPSTONE;
    static BlockType DRIPSTONE_BLOCK;
    static BlockType CAVE_VINES;
    static BlockType CAVE_VINES_PLANT;
    static BlockType SPORE_BLOSSOM;
    static BlockType AZALEA;
    static BlockType FLOWERING_AZALEA;
    static BlockType MOSS_CARPET;
    static BlockType PINK_PETALS;
    static BlockType WILDFLOWERS;
    static BlockType LEAF_LITTER;
    static BlockType MOSS_BLOCK;
    static BlockType BIG_DRIPLEAF;
    static BlockType BIG_DRIPLEAF_STEM;
    static BlockType SMALL_DRIPLEAF;
    static BlockType HANGING_ROOTS;
    static BlockType ROOTED_DIRT;
    static BlockType MUD;
    static BlockType DEEPSLATE;
    static BlockType COBBLED_DEEPSLATE;
    static BlockType COBBLED_DEEPSLATE_STAIRS;
    static BlockType COBBLED_DEEPSLATE_SLAB;
    static BlockType COBBLED_DEEPSLATE_WALL;
    static BlockType POLISHED_DEEPSLATE;
    static BlockType POLISHED_DEEPSLATE_STAIRS;
    static BlockType POLISHED_DEEPSLATE_SLAB;
    static BlockType POLISHED_DEEPSLATE_WALL;
    static BlockType DEEPSLATE_TILES;
    static BlockType DEEPSLATE_TILE_STAIRS;
    static BlockType DEEPSLATE_TILE_SLAB;
    static BlockType DEEPSLATE_TILE_WALL;
    static BlockType DEEPSLATE_BRICKS;
    static BlockType DEEPSLATE_BRICK_STAIRS;
    static BlockType DEEPSLATE_BRICK_SLAB;
    static BlockType DEEPSLATE_BRICK_WALL;
    static BlockType CHISELED_DEEPSLATE;
    static BlockType CRACKED_DEEPSLATE_BRICKS;
    static BlockType CRACKED_DEEPSLATE_TILES;
    static BlockType INFESTED_DEEPSLATE;
    static BlockType SMOOTH_BASALT;
    static BlockType RAW_IRON_BLOCK;
    static BlockType RAW_COPPER_BLOCK;
    static BlockType RAW_GOLD_BLOCK;
    static BlockType POTTED_AZALEA_BUSH;
    static BlockType POTTED_FLOWERING_AZALEA_BUSH;
    static BlockType OCHRE_FROGLIGHT;
    static BlockType VERDANT_FROGLIGHT;
    static BlockType PEARLESCENT_FROGLIGHT;
    static BlockType FROGSPAWN;
    static BlockType REINFORCED_DEEPSLATE;
    static BlockType DECORATED_POT;
    static BlockType CRAFTER;
    static BlockType TRIAL_SPAWNER;
    static BlockType VAULT;
    static BlockType HEAVY_CORE;
    static BlockType PALE_MOSS_BLOCK;
    static BlockType PALE_MOSS_CARPET;
    static BlockType PALE_HANGING_MOSS;
    static BlockType OPEN_EYEBLOSSOM;
    static BlockType CLOSED_EYEBLOSSOM;
    static BlockType POTTED_OPEN_EYEBLOSSOM;
    static BlockType POTTED_CLOSED_EYEBLOSSOM;
    static BlockType FIREFLY_BUSH;
    
    // Registry methods
    static BlockType registerBlock(const std::string& id, const BlockSettings& settings, std::unique_ptr<BlockBehavior> behavior = nullptr);
    
    // Get block by type
    static BlockDefPtr getBlock(BlockType type);
    
    // Get block by ID
    static BlockDefPtr getBlock(const std::string& id);
    
    // Get all blocks
    static const std::vector<BlockDefPtr>& getAllBlocks() { return blocks_; }
    
    // Get render layer for a block type
    static RenderLayer getRenderLayer(BlockType type);
    
    // Behavior access
    static const BlockBehavior* getBehavior(BlockType type);
    static bool hasCollision(BlockType type);
    static bool hasCollisionAt(BlockType type, const glm::vec3& position, const AABB& entityAABB);
    
    static size_t getBlockCount() { return blocks_.size(); }
    
    // Initialize all blocks and behaviors
    static void initialize();
    
private:
    static BlockDefPtr registerBlockInternal(const std::string& id, const BlockSettings& settings, std::unique_ptr<BlockBehavior> behavior = nullptr);
};

} // namespace Zerith