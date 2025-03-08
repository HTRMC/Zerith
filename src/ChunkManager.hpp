#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Block.hpp"
#include "ModelLoader.hpp"
#include "Chunk.hpp"
#include "TextureLoader.hpp"

// Structure to hold mesh data for a specific render layer across all chunks
struct LayerRenderData {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    bool dirty = true;
};

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
    
    // Get mesh data for each render layer
    bool getLayerMeshData(BlockRenderLayer layer, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices) const;

    // Get the complete render data for a layer
    const LayerRenderData& getLayerRenderData(BlockRenderLayer layer) const;

    // Mark a layer's render data as dirty
    void markLayerDirty(BlockRenderLayer layer);

    // Check if a layer's render data is dirty
    bool isLayerDirty(BlockRenderLayer layer) const;

    // Create GPU buffers for a layer
    void createLayerBuffers(BlockRenderLayer layer, VkDevice device, VkPhysicalDevice physicalDevice,
                           VkCommandPool commandPool, VkQueue graphicsQueue);

    // Cleanup layer buffers
    void cleanupLayerBuffers(VkDevice device);
    
    // Load textures for chunks as a texture array
    VkDescriptorImageInfo loadChunkTextures(TextureLoader& textureLoader) const;
    
    // Get the block registry
    const BlockRegistry& getBlockRegistry() const { return blockRegistry; }

    // Get chunks vector
    std::vector<std::unique_ptr<Chunk>>& getChunks() { return chunks; }

private:
    // Block registry to manage block types and properties
    BlockRegistry blockRegistry;
    
    // Container for all chunks
    std::vector<std::unique_ptr<Chunk>> chunks;

    // Render data for each layer
    std::map<BlockRenderLayer, LayerRenderData> layerRenderData;
};