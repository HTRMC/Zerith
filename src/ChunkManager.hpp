#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Block.hpp"
#include "ModelLoader.hpp"
#include "Chunk.hpp"
#include "TextureLoader.hpp"

// Class to manage chunks and block registry
class ChunkManager {
public:
    ChunkManager();
    ~ChunkManager() = default;

    // Initialize the block registry with block types
    void initializeBlockRegistry();
    
    // Create and manage chunks
    void createChunks();
    
    // Update chunk meshes that need regeneration
    void updateChunkMeshes(ModelLoader& modelLoader);
    
    // Get the first chunk's mesh data (for rendering)
    bool getFirstChunkMeshData(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices) const;
    
    // Load textures for chunks
    uint32_t loadChunkTextures(TextureLoader& textureLoader) const;
    
    // Get the block registry
    const BlockRegistry& getBlockRegistry() const { return blockRegistry; }

private:
    // Block registry to manage block types and properties
    BlockRegistry blockRegistry;
    
    // Container for all chunks
    std::vector<std::unique_ptr<Chunk>> chunks;
};