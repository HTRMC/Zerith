#include "BlockStateLoader.hpp"
#include "Logger.hpp"
#include <filesystem>

namespace fs = std::filesystem;

std::optional<std::reference_wrapper<const BlockState>> BlockStateLoader::loadBlockState(const std::string& blockId) {
    // First, check if the blockstate is already in the cache
    auto it = blockStateCache.find(blockId);
    if (it != blockStateCache.end()) {
        // Found in cache
        cacheHits++;
        return std::reference_wrapper<const BlockState>(it->second);
    }

    // Not in cache, need to load it
    cacheMisses++;

    // Resolve the full path
    std::string fullPath = resolveBlockStatePath(blockId);

    LOG_DEBUG("Loading blockstate: %s -> %s", blockId.c_str(), fullPath.c_str());

    // Create blockstate and load from file
    BlockState blockState;
    if (!blockState.loadFromFile(fullPath)) {
        LOG_ERROR("Failed to load blockstate: %s", fullPath.c_str());
        return std::nullopt;
    }

    // Add to cache
    auto [insertIt, success] = blockStateCache.emplace(blockId, std::move(blockState));
    if (!success) {
        LOG_ERROR("Failed to add blockstate to cache: %s", blockId.c_str());
        return std::nullopt;
    }

    // Return a reference to the cached blockstate
    return std::reference_wrapper<const BlockState>(insertIt->second);
}

const BlockState& BlockStateLoader::getCachedBlockState(const std::string& blockId) const {
    auto it = blockStateCache.find(blockId);
    if (it != blockStateCache.end()) {
        return it->second;
    }

    // This should not happen if the caller checks hasBlockState() first
    LOG_ERROR("Requested blockstate not found in cache: %s", blockId.c_str());
    static BlockState emptyBlockState;
    return emptyBlockState;
}

bool BlockStateLoader::hasBlockState(const std::string& blockId) const {
    return blockStateCache.find(blockId) != blockStateCache.end();
}

void BlockStateLoader::clearCache() {
    blockStateCache.clear();
    cacheHits = 0;
    cacheMisses = 0;
}

std::string BlockStateLoader::resolveBlockStatePath(const std::string& blockId) {
    // Split namespace and path if the blockId has a namespace (e.g., "minecraft:stone")
    std::string namespace_ = "minecraft"; // Default namespace
    std::string path = blockId;

    size_t colonPos = blockId.find(':');
    if (colonPos != std::string::npos) {
        namespace_ = blockId.substr(0, colonPos);
        path = blockId.substr(colonPos + 1);
    }

    // Construct the path to the blockstate file
    std::string fullPath = "assets/" + namespace_ + "/blockstates/" + path + ".json";

    return fullPath;
}