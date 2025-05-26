#pragma once

#include "chunk.h"
#include "chunk_mesh_generator.h"
#include "blockbench_instance_generator.h"
#include <unordered_map>
#include <memory>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

namespace Zerith {

class ChunkManager {
public:
    ChunkManager();
    
    // Update which chunks are loaded based on player position
    void updateLoadedChunks(const glm::vec3& playerPosition);
    
    // Get all face instances for rendering (returns reference to avoid copying)
    const std::vector<BlockbenchInstanceGenerator::FaceInstance>& getAllFaceInstances() const { return m_allFaceInstances; }
    
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

private:
    // Convert world position to chunk position
    glm::ivec3 worldToChunkPos(const glm::vec3& worldPos) const;
    
    // Load a chunk at the given chunk position
    void loadChunk(const glm::ivec3& chunkPos);
    
    // Unload a chunk
    void unloadChunk(const glm::ivec3& chunkPos);
    
    // Generate terrain for a chunk (placeholder for now)
    void generateTerrain(Chunk& chunk);
    
    // Check if a chunk position is within render distance
    bool isChunkInRange(const glm::ivec3& chunkPos, const glm::ivec3& centerChunkPos) const;
    
    // Regenerate mesh for a chunk
    void regenerateChunkMesh(const glm::ivec3& chunkPos);

private:
    // Chunk storage - key is chunk position
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> m_chunks;
    
    // Face instances for each chunk
    std::unordered_map<glm::ivec3, std::vector<BlockbenchInstanceGenerator::FaceInstance>> m_chunkMeshes;
    
    // Mesh generator
    std::unique_ptr<ChunkMeshGenerator> m_meshGenerator;
    
    // Render distance in chunks
    int m_renderDistance = 2;
    
    // Last player chunk position to detect when to update loaded chunks
    glm::ivec3 m_lastPlayerChunkPos = glm::ivec3(INT_MAX);
    
    // Combined face instances for all chunks (updated only when chunks change)
    std::vector<BlockbenchInstanceGenerator::FaceInstance> m_allFaceInstances;
    
    // Flag to track if face instances need to be rebuilt
    bool m_needsRebuild = true;
    
    // Rebuild the combined face instance vector
    void rebuildAllFaceInstances();
};

} // namespace Zerith