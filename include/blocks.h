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