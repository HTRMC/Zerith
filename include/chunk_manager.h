#pragma once

#include "chunk.h"
#include "chunk_mesh_generator.h"
#include "terrain_generator.h"
#include "blockbench_instance_generator.h"
#include "indirect_draw.h"
#include "chunk_octree.h"
#include "thread_pool.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <future>
#include <functional>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace Zerith {

struct ChunkLoadRequest {
    glm::ivec3 chunkPos;
    int priority;
    
    bool operator<(const ChunkLoadRequest& other) const {
        return priority < other.priority; // Higher priority first
    }
};

struct MeshGenerationRequest {
    glm::ivec3 chunkPos;
    int priority;
    
    bool operator<(const MeshGenerationRequest& other) const {
        return priority < other.priority; // Higher priority first
    }
};

struct CompletedMesh {
    glm::ivec3 chunkPos;
    std::vector<BlockbenchInstanceGenerator::FaceInstance> faces;
};

struct ChunkData {
    std::unique_ptr<Chunk> chunk;
    std::vector<BlockbenchInstanceGenerator::FaceInstance> faces;
    std::atomic<bool> ready{false};
    
    // Move constructor and assignment
    ChunkData() = default;
    ChunkData(ChunkData&& other) noexcept 
        : chunk(std::move(other.chunk)), 
          faces(std::move(other.faces)), 
          ready(other.ready.load()) {}
    
    ChunkData& operator=(ChunkData&& other) noexcept {
        if (this != &other) {
            chunk = std::move(other.chunk);
            faces = std::move(other.faces);
            ready = other.ready.load();
        }
        return *this;
    }
    
    // Disable copy operations
    ChunkData(const ChunkData&) = delete;
    ChunkData& operator=(const ChunkData&) = delete;
};

class ChunkManager {
public:
    ChunkManager();
    ~ChunkManager();
    
    // Set timing callback functions
    void setTimingCallbacks(std::function<void(float)> chunkGenCallback, std::function<void(float)> meshGenCallback) {
        m_chunkGenCallback = chunkGenCallback;
        m_meshGenCallback = meshGenCallback;
    }
    
    // Update which chunks are loaded based on player position
    void updateLoadedChunks(const glm::vec3& playerPosition);
    
    // Get all face instances for rendering (returns reference to avoid copying)
    const std::vector<BlockbenchInstanceGenerator::FaceInstance>& getAllFaceInstances() const { return m_allFaceInstances; }
    
    // Get face instances for rendering when changed (avoids unnecessary operations)
    std::vector<BlockbenchInstanceGenerator::FaceInstance> getFaceInstancesWhenChanged() { 
        m_needsRebuild = false;
        return m_allFaceInstances; // Copy, but only when actually changed
    }
    
    // Check if face instances have been updated since last move
    bool hasFaceInstancesChanged() const { return m_needsRebuild; }
    
    // Get a specific chunk (returns nullptr if not loaded)
    Chunk* getChunk(const glm::ivec3& chunkPos);
    
    // Get block at world position
    BlockType getBlock(const glm::vec3& worldPos) const;
    
    // Set block at world position
    void setBlock(const glm::vec3& worldPos, BlockType type);
    
    // Get chunks in a region using octree for fast spatial queries
    std::vector<Chunk*> getChunksInRegion(const AABB& region) const;
    
    // Get chunks along a ray using octree for fast raycasting
    std::vector<Chunk*> getChunksAlongRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = std::numeric_limits<float>::max()) const;
    
    // Get render distance in chunks
    int getRenderDistance() const { return m_renderDistance; }
    void setRenderDistance(int distance) { 
        m_renderDistance = std::min(std::max(distance, 1), 32); // Clamp between 1 and 32
    }
    
    // Get statistics
    size_t getLoadedChunkCount() const { return m_chunks.size(); }
    size_t getTotalFaceCount() const;
    
    // Get mesh generator for texture array access
    const ChunkMeshGenerator* getMeshGenerator() const { return m_meshGenerator.get(); }
    
    // Process completed chunk loading tasks
    void processCompletedChunks();
    
    // Get indirect draw manager
    const IndirectDrawManager& getIndirectDrawManager() const { return m_indirectDrawManager; }

private:
    // Convert world position to chunk position
    glm::ivec3 worldToChunkPos(const glm::vec3& worldPos) const;
    
    // Load a chunk at the given chunk position (synchronous)
    void loadChunk(const glm::ivec3& chunkPos);
    
    // Load a chunk asynchronously
    void loadChunkAsync(const glm::ivec3& chunkPos, int priority = 0);
    
    // Unload a chunk
    void unloadChunk(const glm::ivec3& chunkPos);
    
    // Generate terrain for a chunk (placeholder for now)
    void generateTerrain(Chunk& chunk);
    
    // Check if a chunk position is within render distance
    bool isChunkInRange(const glm::ivec3& chunkPos, const glm::ivec3& centerChunkPos) const;
    
    // Regenerate mesh for a chunk (e.g., when neighbors change)
    void regenerateChunkMesh(const glm::ivec3& chunkPos);
    
    
    // Background chunk loading function
    std::unique_ptr<ChunkData> loadChunkBackground(const glm::ivec3& chunkPos);
    
    // Background mesh generation function
    std::vector<BlockbenchInstanceGenerator::FaceInstance> generateMeshForChunk(const glm::ivec3& chunkPos, Chunk* chunk);
    
    // Queue mesh generation for a chunk
    void queueMeshGeneration(const glm::ivec3& chunkPos, int priority);

private:
    // Chunk storage - key is chunk position
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> m_chunks;
    
    // Face instances for each chunk
    std::unordered_map<glm::ivec3, std::vector<BlockbenchInstanceGenerator::FaceInstance>> m_chunkMeshes;
    
    // Mesh generator
    std::unique_ptr<ChunkMeshGenerator> m_meshGenerator;
    
    // Terrain generator
    std::unique_ptr<TerrainGenerator> m_terrainGenerator;
    
    // Render distance in chunks
    int m_renderDistance = 8;
    
    // Last player chunk position to detect when to update loaded chunks
    glm::ivec3 m_lastPlayerChunkPos = glm::ivec3(INT_MAX);
    
    // Combined face instances for all chunks (updated only when chunks change)
    std::vector<BlockbenchInstanceGenerator::FaceInstance> m_allFaceInstances;
    
    // Flag to track if face instances need to be rebuilt
    bool m_needsRebuild = true;
    
    // Threading components
    std::unordered_map<glm::ivec3, Task::TaskId> m_loadingChunks;
    std::unordered_map<glm::ivec3, Task::TaskId> m_meshingChunks;
    
    // Thread synchronization
    mutable std::shared_mutex m_chunksMutex;  // Reader-writer lock for chunk map
    mutable std::mutex m_loadingMutex;
    mutable std::mutex m_meshingMutex;
    
    // Per-chunk fine-grained locking
    mutable std::unordered_map<glm::ivec3, std::shared_ptr<std::mutex>> m_chunkMutexes;
    mutable std::mutex m_chunkMutexesMutex;
    
    // Completed chunks ready to be integrated
    std::queue<std::pair<glm::ivec3, std::unique_ptr<ChunkData>>> m_completedChunks;
    std::mutex m_completedMutex;
    
    // Completed meshes ready to be integrated
    std::queue<CompletedMesh> m_completedMeshes;
    std::mutex m_completedMeshMutex;
    
    // Rebuild the combined face instance vector
    void rebuildAllFaceInstances();
    
    // Rebuild indirect draw commands
    void rebuildIndirectCommands();
    
    // Per-chunk locking helpers
    std::shared_ptr<std::mutex> getChunkMutex(const glm::ivec3& chunkPos) const;
    void removeChunkMutex(const glm::ivec3& chunkPos);
    
    // Indirect draw manager
    IndirectDrawManager m_indirectDrawManager;
    
    // Chunk octree for spatial queries
    std::unique_ptr<ChunkOctree> m_chunkOctree;
    
    // Timing callbacks
    std::function<void(float)> m_chunkGenCallback;
    std::function<void(float)> m_meshGenCallback;
};

} // namespace Zerith