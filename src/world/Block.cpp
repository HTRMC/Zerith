#include "Block.hpp"

void BlockRegistry::registerBlock(uint16_t id, const std::string& name, BlockRenderLayer renderLayer) {
    blockNames[id] = name;

    // Set transparency based on block type
    // Default to non-transparent (solid)
    bool transparent = false;

    // Define transparent blocks
    if (id == 0 || name == "air" || name == "glass" || renderLayer == BlockRenderLayer::LAYER_TRANSLUCENT) {
        transparent = true;
    }

    blockTransparency[id] = transparent;

    // Store the render layer
    blockRenderLayers[id] = renderLayer;
}

std::string BlockRegistry::getBlockName(uint16_t id) const {
    auto it = blockNames.find(id);
    if (it != blockNames.end()) {
        return it->second;
    }
    return "unknown"; // Default unknown block
}

bool BlockRegistry::isValidBlock(uint16_t id) const {
    return blockNames.find(id) != blockNames.end();
}

bool BlockRegistry::isBlockTransparent(uint16_t id) const {
    auto it = blockTransparency.find(id);
    if (it != blockTransparency.end()) {
        return it->second;
    }
    return false; // Default to solid if unknown
}

BlockRenderLayer BlockRegistry::getBlockRenderLayer(uint16_t id) const {
    auto it = blockRenderLayers.find(id);
    if (it != blockRenderLayers.end()) {
        return it->second;
    }
    return BlockRenderLayer::LAYER_OPAQUE; // Default to opaque if unknown
}

std::string BlockRegistry::getModelPath(uint16_t id) const {
    std::string name = getBlockName(id);
    if (name == "unknown" || name == "air") {
        return "assets/minecraft/models/block/stone.json"; // Fallback model
    }

    // Format as "assets/minecraft/models/block/<name>.json"
    return "assets/minecraft/models/block/" + name + ".json";
}