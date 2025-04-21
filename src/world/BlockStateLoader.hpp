#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include "BlockState.hpp"

class BlockStateLoader {
public:
    BlockStateLoader() = default;
    ~BlockStateLoader() = default;

    // Load a blockstate from a file, using cache if available
    std::optional<std::reference_wrapper<const BlockState>> loadBlockState(const std::string& blockId);

    // Get a blockstate from the cache (the blockstate must already exist in the cache)
    const BlockState& getCachedBlockState(const std::string& blockId) const;

    // Check if a blockstate is in the cache
    bool hasBlockState(const std::string& blockId) const;

    // Clear the blockstate cache
    void clearCache();

    // Get stats about the blockstate cache
    size_t getCacheSize() const { return blockStateCache.size(); }
    size_t getCacheHits() const { return cacheHits; }
    size_t getCacheMisses() const { return cacheMisses; }

    // Resolve a blockstate path from a block ID (e.g., "stone" -> "assets/minecraft/blockstates/stone.json")
    static std::string resolveBlockStatePath(const std::string& blockId);

private:
    // Blockstate cache
    std::unordered_map<std::string, BlockState> blockStateCache;
    size_t cacheHits = 0;    // For stats
    size_t cacheMisses = 0;  // For stats
};