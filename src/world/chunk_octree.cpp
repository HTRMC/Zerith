#include "chunk_octree.h"
#include "coordinate_conversion.h"
#include "logger.h"

namespace Zerith {

ChunkOctree::ChunkOctree(const AABB& worldBounds)
    : octree(worldBounds), chunkCount(0)
{
    LOG_INFO("ChunkOctree initialized with bounds: (%f, %f, %f) to (%f, %f, %f)",
               worldBounds.min.x, worldBounds.min.y, worldBounds.min.z,
               worldBounds.max.x, worldBounds.max.y, worldBounds.max.z);
}

void ChunkOctree::addChunk(Chunk* chunk) {
    if (!chunk) {
        LOG_WARN("ChunkOctree: Attempted to add null chunk");
        return;
    }
    
    AABB chunkAABB = getChunkAABB(chunk->getChunkPosition());
    octree.insert(chunkAABB, chunk);
    chunkCount++;
}

void ChunkOctree::removeChunk(Chunk* chunk) {
    if (!chunk) {
        LOG_WARN("ChunkOctree: Attempted to remove null chunk");
        return;
    }
    
    AABB chunkAABB = getChunkAABB(chunk->getChunkPosition());
    if (octree.remove(chunkAABB, chunk)) {
        chunkCount--;
    }
}

void ChunkOctree::updateChunk(Chunk* chunk, const glm::ivec3& oldPos, const glm::ivec3& newPos) {
    if (!chunk) {
        LOG_WARN("ChunkOctree: Attempted to update null chunk");
        return;
    }
    
    AABB oldAABB = getChunkAABB(oldPos);
    AABB newAABB = getChunkAABB(newPos);
    octree.update(oldAABB, newAABB, chunk);
}

std::vector<Chunk*> ChunkOctree::getChunksInRegion(const AABB& region) const {
    std::vector<Chunk*> chunks;
    auto results = octree.queryRegion(region);
    
    chunks.reserve(results.size());
    for (const auto& [bounds, chunk] : results) {
        chunks.push_back(chunk);
    }
    
    return chunks;
}

std::vector<Chunk*> ChunkOctree::getChunksAlongRay(const glm::vec3& origin, 
                                               const glm::vec3& direction, 
                                               float maxDistance) const {
    std::vector<Chunk*> chunks;
    auto results = octree.queryRay(origin, direction, maxDistance);
    
    chunks.reserve(results.size());
    for (const auto& [bounds, chunk] : results) {
        chunks.push_back(chunk);
    }
    
    return chunks;
}

void ChunkOctree::clear() {
    octree.clear();
    chunkCount = 0;
}

AABB ChunkOctree::getChunkAABB(const glm::ivec3& chunkPos) const {
    // Convert chunk position to world coordinates
    glm::vec3 worldMin = CoordinateConversion::chunkToWorld(chunkPos);
    glm::vec3 worldMax = worldMin + glm::vec3(Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE, Chunk::CHUNK_SIZE);
    
    return AABB(worldMin, worldMax);
}

} // namespace Zerith