#include "Block.hpp"
#include "BlockStateLoader.hpp"
#include "Logger.hpp"

BlockRegistry::BlockRegistry() : blockStateLoader(nullptr) {
    // Initialize with no blockstate loader, must be set later
}

void BlockRegistry::registerBlock(uint16_t id, const std::string& name, BlockRenderLayer renderLayer) {
    blockNames[id] = name;

    // Set blockstate ID (e.g., "minecraft:stone")
    blockStateIds[id] = "minecraft:" + name;

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

    LOG_DEBUG("Registered block: id=%d, name=%s, renderLayer=%d", id, name.c_str(), static_cast<int>(renderLayer));
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

std::string BlockRegistry::getBlockStateId(uint16_t id) const {
    auto it = blockStateIds.find(id);
    if (it != blockStateIds.end()) {
        return it->second;
    }

    // Fallback to constructing from name
    return "minecraft:" + getBlockName(id);
}

std::string BlockRegistry::getModelPath(uint16_t id) const {
    // Get the block's name for logging
    std::string name = getBlockName(id);

    // If the block is air or unknown, return fallback model
    if (id == 0 || name == "unknown" || name == "air") {
        return "assets/minecraft/models/block/stone.json"; // Fallback model
    }

    // If we have a blockstate loader, use it to get the blockstate and model
    if (blockStateLoader) {
        std::string blockStateId = getBlockStateId(id);
        auto blockStateOpt = blockStateLoader->loadBlockState(name); // Use block name directly as the ID

        if (blockStateOpt.has_value()) {
            // Get a random variant from the blockstate
            const auto& variant = blockStateOpt->get().getRandomVariant();

            // Get the model path from the variant
            std::string modelPath = variant.modelPath;

            // If the model path doesn't end with .json, add it
            if (modelPath.find(".json") == std::string::npos) {
                modelPath += ".json";
            }

            // If the model path doesn't start with assets/, convert it to a full path
            if (modelPath.find("assets/") != 0) {
                // Extract namespace if present
                std::string namespace_ = "minecraft"; // Default namespace
                std::string path = modelPath;

                size_t colonPos = modelPath.find(':');
                if (colonPos != std::string::npos) {
                    namespace_ = modelPath.substr(0, colonPos);
                    path = modelPath.substr(colonPos + 1);
                }

                // Construct the full path
                modelPath = "assets/" + namespace_ + "/models/" + path;

                // Make sure it ends with .json
                if (modelPath.find(".json") == std::string::npos) {
                    modelPath += ".json";
                }
            }

            LOG_DEBUG("Using model path from blockstate for %s: %s", name.c_str(), modelPath.c_str());
            return modelPath;
        } else {
            LOG_WARN("Could not load blockstate for %s, falling back to default model path", name.c_str());
        }
    }

    // Fallback to legacy behavior: direct model path
    LOG_DEBUG("Using fallback model path for %s: assets/minecraft/models/block/%s.json", name.c_str(), name.c_str());
    return "assets/minecraft/models/block/" + name + ".json";
}