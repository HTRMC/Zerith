#pragma once

#include "chunk.h"
#include "chunk_mesh_generator.h"
#include "terrain_generator.h"
#include "blockbench_instance_generator.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <future>

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

struct ChunkData {
    std::unique_ptr<Chunk> chunk;
    std::vector<BlockbenchInstanceGenerator::FaceInstance> faces;
    std::atomic<bool> ready{false};
};

class ChunkManager {
public:
    ChunkManager();
    ~ChunkManager();
    
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
    
    // Get render distance in chunks
    int getRenderDistance() const { return m_renderDistance; }
    void setRenderDistance(int distance) { m_renderDistance = distance; }
    
    // Get statistics
    size_t getLoadedChunkCount() const { return m_chunks.size(); }
    size_t getTotalFaceCount() const;
    
    // Get mesh generator for texture array access
    const ChunkMeshGenerator* getMeshGenerator() const { return m_meshGenerator.get(); }
    
    // Process completed chunk loading tasks
    void processCompletedChunks();

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
    
    // Regenerate mesh for a chunk
    void regenerateChunkMesh(const glm::ivec3& chunkPos);
    
    // Worker thread function
    void chunkWorkerThread();
    
    // Background chunk loading function
    std::unique_ptr<ChunkData> loadChunkBackground(const glm::ivec3& chunkPos);

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
    int m_renderDistance = 2;
    
    // Last player chunk position to detect when to update loaded chunks
    glm::ivec3 m_lastPlayerChunkPos = glm::ivec3(INT_MAX);
    
    // Combined face instances for all chunks (updated only when chunks change)
    std::vector<BlockbenchInstanceGenerator::FaceInstance> m_allFaceInstances;
    
    // Flag to track if face instances need to be rebuilt
    bool m_needsRebuild = true;
    
    // Threading components
    std::vector<std::thread> m_workerThreads;
    std::priority_queue<ChunkLoadRequest> m_loadQueue;
    std::unordered_map<glm::ivec3, std::future<std::unique_ptr<ChunkData>>> m_loadingChunks;
    
    // Thread synchronization
    mutable std::mutex m_chunksMutex;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::atomic<bool> m_shutdown{false};
    
    // Completed chunks ready to be integrated
    std::queue<std::pair<glm::ivec3, std::unique_ptr<ChunkData>>> m_completedChunks;
    std::mutex m_completedMutex;
    
    // Rebuild the combined face instance vector
    void rebuildAllFaceInstances();
};

} // namespace Zerith