#pragma once

#include "octree.h"
#include "chunk.h"
#include <memory>

namespace Zerith {

class ChunkOctree {
public:
    ChunkOctree(const AABB& worldBounds);
    ~ChunkOctree() = default;

    // Add a chunk to the octree
    void addChunk(Chunk* chunk);
    
    // Remove a chunk from the octree
    void removeChunk(Chunk* chunk);
    
    // Update a chunk's position (rarely needed, but provided for completeness)
    void updateChunk(Chunk* chunk, const glm::ivec3& oldPos, const glm::ivec3& newPos);
    
    // Query chunks within a region
    std::vector<Chunk*> getChunksInRegion(const AABB& region) const;
    
    // Query chunks that intersect with a ray
    std::vector<Chunk*> getChunksAlongRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = std::numeric_limits<float>::max()) const;
    
    // Clear the octree
    void clear();
    
    // Get the number of chunks in the octree
    size_t getChunkCount() const { return chunkCount; }

private:
    Octree<Chunk*> octree;
    size_t chunkCount;
    
    // Helper function to calculate AABB for a chunk position
    AABB getChunkAABB(const glm::ivec3& chunkPos) const;
};

} // namespace Zerith