#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "chunk.h"
#include "block_properties.h"

namespace Zerith {

// Forward declarations
class BlockDefinition;
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

// Block settings builder
class BlockSettings {
private:
    std::string modelName_;
    BlockMaterial material_ = BlockMaterial::SOLID;
    bool isTransparent_ = false;
    bool renderAllFaces_ = false;
    
    // Culling properties
    std::array<CullFace, 6> faceCulling_ = {
        CullFace::FULL, CullFace::FULL, CullFace::FULL,
        CullFace::FULL, CullFace::FULL, CullFace::FULL
    };

public:
    static BlockSettings create() { return BlockSettings(); }
    
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
    
    BlockSettings& noFaceCulling() {
        std::fill(faceCulling_.begin(), faceCulling_.end(), CullFace::NONE);
        return *this;
    }
    
    BlockSettings& slab() {
        // Top face doesn't cull, bottom face does
        faceCulling_[0] = CullFace::FULL; // Bottom
        faceCulling_[1] = CullFace::NONE; // Top
        return *this;
    }
    
    BlockSettings& stairs() {
        // Complex culling for stairs - simplified for now
        faceCulling_[1] = CullFace::NONE; // Top face varies
        return *this;
    }
    
    friend class BlockDefinition;
};

// Block definition
class BlockDefinition {
private:
    std::string id_;
    std::string displayName_;
    BlockSettings settings_;
    BlockType blockType_;
    
public:
    BlockDefinition(const std::string& id, const std::string& displayName, const BlockSettings& settings)
        : id_(id), displayName_(displayName), settings_(settings) {}
    
    const std::string& getId() const { return id_; }
    const std::string& getDisplayName() const { return displayName_; }
    const std::string& getModelName() const { return settings_.modelName_; }
    
    BlockCullingProperties getCullingProperties() const {
        return {
            settings_.faceCulling_,
            settings_.isTransparent_,
            !settings_.renderAllFaces_
        };
    }
    
    void setBlockType(BlockType type) { blockType_ = type; }
    BlockType getBlockType() const { return blockType_; }
};

// Block registry singleton
class BlockRegistry {
private:
    std::vector<BlockDefPtr> blocks_;
    std::unordered_map<std::string, size_t> idToIndex_;
    static BlockRegistry* instance_;
    
    BlockRegistry() = default;

public:
    static BlockRegistry& getInstance() {
        if (!instance_) {
            instance_ = new BlockRegistry();
        }
        return *instance_;
    }
    
    // Quick access to common block types
    static BlockType AIR_TYPE() { return 0; }
    
    // Register a block and return its type
    static BlockDefPtr registerBlock(const std::string& id, BlockDefPtr block) {
        auto& registry = getInstance();
        size_t index = registry.blocks_.size();
        
        block->setBlockType(static_cast<BlockType>(index));
        registry.blocks_.push_back(block);
        registry.idToIndex_[id] = index;
        
        return block;
    }
    
    // Get block by type
    BlockDefPtr getBlock(BlockType type) const {
        size_t index = static_cast<size_t>(type);
        if (index < blocks_.size()) {
            return blocks_[index];
        }
        return nullptr;
    }
    
    // Get block by ID
    BlockDefPtr getBlock(const std::string& id) const {
        auto it = idToIndex_.find(id);
        if (it != idToIndex_.end()) {
            return blocks_[it->second];
        }
        return nullptr;
    }
    
    // Get all blocks
    const std::vector<BlockDefPtr>& getAllBlocks() const {
        return blocks_;
    }
    
    size_t getBlockCount() const {
        return blocks_.size();
    }
};

// Main Blocks class with all block definitions
class Blocks {
public:
    // Basic blocks
    static const BlockDefPtr AIR;
    static const BlockDefPtr STONE;
    static const BlockDefPtr DIRT;
    static const BlockDefPtr GRASS_BLOCK;
    static const BlockDefPtr COBBLESTONE;
    static const BlockDefPtr SAND;
    static const BlockDefPtr GRAVEL;
    
    // Wood blocks
    static const BlockDefPtr OAK_LOG;
    static const BlockDefPtr OAK_PLANKS;
    static const BlockDefPtr OAK_LEAVES;
    static const BlockDefPtr OAK_SLAB;
    static const BlockDefPtr OAK_STAIRS;
    
    // Stone variants
    static const BlockDefPtr STONE_BRICKS;
    static const BlockDefPtr BRICKS;
    
    // Ores
    static const BlockDefPtr COAL_ORE;
    static const BlockDefPtr IRON_ORE;
    static const BlockDefPtr DIAMOND_ORE;
    
    // Special blocks
    static const BlockDefPtr GLASS;
    static const BlockDefPtr GLOWSTONE;
    static const BlockDefPtr WATER;
    static const BlockDefPtr CRAFTING_TABLE;
    
    // Initialize all blocks
    static void initialize();
    
private:
    static BlockDefPtr registerBlock(const std::string& id, const std::string& displayName, const BlockSettings& settings) {
        return BlockRegistry::registerBlock(id, 
            std::make_shared<BlockDefinition>(id, displayName, settings));
    }
};

} // namespace Zerith