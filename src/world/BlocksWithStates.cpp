#include "BlocksWithStates.hpp"

#include "Blocks.hpp"
#include "Logger.hpp"
#include <filesystem>

namespace fs = std::filesystem;

// Initialize static member
std::shared_ptr<BlockStateLoader> BlocksWithStates::blockStateLoader = nullptr;

void BlocksWithStates::registerAllBlocks(BlockRegistry& registry) {
    LOG_INFO("Registering blocks with blockstate support...");

    // Use the existing Blocks class to register all blocks
    Blocks::registerAllBlocks(registry);

    // Initialize the blockstate system for the registered blocks
    initBlockStates(registry);
}

void BlocksWithStates::initBlockStates(BlockRegistry& registry) {
    // Create the blockstate loader if it doesn't exist
    if (!blockStateLoader) {
        blockStateLoader = std::make_shared<BlockStateLoader>();
    }
    
    // Set the blockstate loader in the registry
    registry.setBlockStateLoader(blockStateLoader);
    
    // Check if the blockstates directory exists
    std::string blockstatesDir = "assets/minecraft/blockstates";
    if (!fs::exists(blockstatesDir) || !fs::is_directory(blockstatesDir)) {
        LOG_WARN("Blockstates directory not found: %s", blockstatesDir.c_str());
        return;
    }

    // Count blockstate files
    size_t fileCount = 0;
    for (const auto& entry : fs::directory_iterator(blockstatesDir)) {
        if (entry.path().extension() == ".json") {
            fileCount++;
        }
    }
    LOG_INFO("Found %zu blockstate files in directory %s", fileCount, blockstatesDir.c_str());

    // Pre-load blockstates for all registered blocks
    LOG_INFO("Pre-loading blockstates for all registered blocks...");
    int loadedCount = 0;
    int failedCount = 0;
    int totalCount = 0;

    // Get the total number of blocks in the registry
    size_t blockCount = registry.getBlockCount();

    // Try to load blockstates for all blocks
    for (uint16_t id = 0; id < blockCount; id++) {
        // Skip air (block ID 0)
        if (id == 0) continue;

        std::string blockName = registry.getBlockName(id);
        if (blockName == "unknown") continue;

        totalCount++;

        // Try to load blockstate for this block
        auto blockStateOpt = blockStateLoader->loadBlockState(blockName);
        if (blockStateOpt.has_value()) {
            // Success
            loadedCount++;

            // Log the variants we found
            const auto& blockState = blockStateOpt->get();
            LOG_DEBUG("Loaded blockstate for %s (ID %d) with %zu variants",
                     blockName.c_str(), id, blockState.getVariantCount());
        } else {
            // Failed
            failedCount++;
            LOG_DEBUG("No blockstate found for %s (ID %d)", blockName.c_str(), id);
        }
    }

    LOG_INFO("Pre-loaded %d/%d blockstates (%d failed)", loadedCount, totalCount, failedCount);

    // If we have too many failures, log a warning
    if (failedCount > loadedCount) {
        LOG_WARN("More than half of blockstates failed to load. Check your blockstate files and directory structure.");
    }
}