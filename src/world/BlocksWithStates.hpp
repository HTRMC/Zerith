#pragma once

#include "Block.hpp"
#include "BlockStateLoader.hpp"
#include <memory>

// Forward declaration
class BlockRegistry;

// Class to extend the block system with blockstate support
class BlocksWithStates {
public:
    // Use the existing Blocks class to register all blocks, then initialize blockstates
    static void registerAllBlocks(BlockRegistry& registry);

    // Initialize the blockstate system for already registered blocks
    static void initBlockStates(BlockRegistry& registry);

private:
    // Shared instance of the blockstate loader
    static std::shared_ptr<BlockStateLoader> blockStateLoader;
};