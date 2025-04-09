#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

enum class BlockRenderLayer {
    LAYER_OPAQUE,      // Solid blocks (stone, dirt, etc.)
    LAYER_CUTOUT,      // Blocks with parts that are fully transparent (glass, leaves)
    LAYER_TRANSLUCENT    // Blocks with partially transparent parts (colored glass, water)
};

// Block registry to manage block types, IDs, and their model paths
class BlockRegistry {
public:
    BlockRegistry() = default;
    ~BlockRegistry() = default;

    // Register a new block type
    void registerBlock(uint16_t id, const std::string& name, BlockRenderLayer renderLayer = BlockRenderLayer::LAYER_OPAQUE);
    
    // Get the name for a block ID
    std::string getBlockName(uint16_t id) const;
    
    // Check if a block ID is valid
    bool isValidBlock(uint16_t id) const;
    
    // Check if a block ID is transparent (e.g., air, glass)
    bool isBlockTransparent(uint16_t id) const;
    
    // Get the model path for a block
    std::string getModelPath(uint16_t id) const;

    // Get the render layer for a block
    BlockRenderLayer getBlockRenderLayer(uint16_t id) const;

    // Get the number of registered blocks
    size_t getBlockCount() const {
        return blockNames.size();
    }

private:
    std::unordered_map<uint16_t, std::string> blockNames;
    std::unordered_map<uint16_t, bool> blockTransparency;
    std::unordered_map<uint16_t, BlockRenderLayer> blockRenderLayers;
};