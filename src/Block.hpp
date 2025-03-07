#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// Block registry to manage block types, IDs, and their model paths
class BlockRegistry {
public:
    BlockRegistry() = default;
    ~BlockRegistry() = default;

    // Register a new block type
    void registerBlock(uint16_t id, const std::string& name);
    
    // Get the name for a block ID
    std::string getBlockName(uint16_t id) const;
    
    // Check if a block ID is valid
    bool isValidBlock(uint16_t id) const;
    
    // Check if a block ID is transparent (e.g., air, glass)
    bool isBlockTransparent(uint16_t id) const;
    
    // Get the model path for a block
    std::string getModelPath(uint16_t id) const;

private:
    std::unordered_map<uint16_t, std::string> blockNames;
    std::unordered_map<uint16_t, bool> blockTransparency;
};