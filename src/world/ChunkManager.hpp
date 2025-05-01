#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <future>
#include <unordered_set>

#include "Block.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/TextureLoader.hpp"
#include "Chunk.hpp"
#include "core/ThreadPool.hpp"

// Structure to hold mesh data for a specific render layer across all chunks
struct LayerRenderData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    bool dirty = true;
};

// Hash function for glm::ivec3 to use in unordered_map
struct IVec3Hash {
    size_t operator()(const glm::ivec3& vec) const {
        // Simple hash function for ivec3
        return std::hash<int>()(vec.x) ^
               (std::hash<int>()(vec.y) << 1) ^
               (std::hash<int>()(vec.z) << 2);
    }
};

// Equality operator for glm::ivec3 to use in unordered_map
struct IVec3Equal {
    bool operator()(const glm::ivec3& lhs, const glm::ivec3& rhs) const {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }
};

// Class to manage chunks and block registry
class ChunkManager {
public:
    ChunkManager();
    ~ChunkManager();
    
    // Load chunks around the given player position
    void updateLoadedChunks(const glm::vec3& playerPosition);
    
    // Update chunk meshes that need regeneration
    void updateChunkMeshes(ModelLoader& modelLoader, TextureLoader& textureLoader);
    
    // Get mesh data for each render layer
    bool getLayerMeshData(BlockRenderLayer layer, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) const;

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

    // Get a specific chunk by position
    Chunk* getChunk(const glm::ivec3& position);

    // Set device-related resources for buffer creation
    void setVulkanResources(VkDevice device, VkPhysicalDevice physicalDevice,
                          VkCommandPool commandPool, VkQueue graphicsQueue);

    // Get block at a specific world position
    uint16_t getBlockAt(const glm::vec3& worldPos) const;

    // Set block at a specific world position
    void setBlockAt(const glm::vec3& worldPos, uint16_t blockId);

    // Convert world position to chunk position
    static glm::ivec3 worldToChunkPos(const glm::vec3& worldPos);

    // Convert world position to local position within a chunk
    static glm::ivec3 worldToLocalPos(const glm::vec3& worldPos);

    // Configuration
    void setChunkLoadRadius(int radius) { chunkLoadRadius = radius; }
    int getChunkLoadRadius() const { return chunkLoadRadius; }

    void setMaxChunksPerFrame(int count) { maxChunksPerFrame = count; }
    int getMaxChunksPerFrame() const { return maxChunksPerFrame; }

    // Get the total number of loaded chunks
    size_t getLoadedChunkCount() const;

    // Preload common block models to improve performance
    void preloadBlockModels(ModelLoader& modelLoader);

    // Wait for all pending chunk operations to complete
    void waitForPendingOperations();

private:
    // Block registry to manage block types and properties
    BlockRegistry blockRegistry;
    
    // Container for all chunks (mapped by chunk position)
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash, IVec3Equal> chunks;

    // Render data for each layer
    std::map<BlockRenderLayer, LayerRenderData> layerRenderData;

    // Thread pool for chunk loading/generation
    std::shared_ptr<Zerith::ThreadPool> threadPool;

    // Mutex for thread-safe access to chunks map
    mutable std::mutex chunksMutex;

    // Map to track pending async operations
    std::unordered_map<glm::ivec3, std::future<void>, IVec3Hash, IVec3Equal> pendingOperations;
    std::mutex pendingOpsMutex;

    // Chunk that needs to be loaded
    struct ChunkLoadRequest {
        glm::ivec3 position;
        int priority; // Lower number = higher priority
    };

    // Queue for chunks to load
    std::queue<ChunkLoadRequest> chunkLoadQueue;
    std::mutex queueMutex;

    // Set of chunk positions currently in the load queue
    std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal> queuedChunks;
    std::mutex queuedChunksMutex;

    // Last known player chunk position
    glm::ivec3 lastPlayerChunkPos;
    std::mutex playerPosMutex;

    // Configuration
    int chunkLoadRadius = 8; // How many chunks to load in each direction
    int maxChunksPerFrame = 2; // Maximum number of chunks to load per frame

    // Vulkan resources for buffer creation
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    // Helper methods
    void loadChunk(const glm::ivec3& position);
    void unloadChunk(const glm::ivec3& position);
    bool isChunkInRange(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos, int radius) const;
    void processChunkQueue(ModelLoader& modelLoader);
    void generateChunkMeshes(ModelLoader& modelLoader, TextureLoader& textureLoader);
};