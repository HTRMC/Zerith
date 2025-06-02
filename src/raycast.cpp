#include "raycast.h"
#include "chunk_manager.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace Zerith {

std::optional<RaycastHit> Raycast::cast(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    ChunkManager* chunkManager
) {
    if (!chunkManager) {
        return std::nullopt;
    }

    glm::vec3 normalizedDir = glm::normalize(direction);
    
    // Use octree to get chunks along the ray (optimization)
    auto chunksAlongRay = chunkManager->getChunksAlongRay(origin, normalizedDir, maxDistance);
    
    // If no chunks found along the ray, return early
    if (chunksAlongRay.empty()) {
        return std::nullopt;
    }
    
    // Current position in the grid
    glm::ivec3 current(
        static_cast<int>(std::floor(origin.x)),
        static_cast<int>(std::floor(origin.y)),
        static_cast<int>(std::floor(origin.z))
    );
    
    // Direction to step in (1, 0, or -1 for each axis)
    glm::ivec3 step = sign(normalizedDir);
    
    // Calculate tMax - distance to next grid line
    glm::vec3 tMax;
    if (normalizedDir.x != 0) {
        tMax.x = (normalizedDir.x > 0) ? 
            (std::floor(origin.x) + 1.0f - origin.x) / normalizedDir.x :
            (origin.x - std::floor(origin.x)) / -normalizedDir.x;
    } else {
        tMax.x = std::numeric_limits<float>::max();
    }
    
    if (normalizedDir.y != 0) {
        tMax.y = (normalizedDir.y > 0) ? 
            (std::floor(origin.y) + 1.0f - origin.y) / normalizedDir.y :
            (origin.y - std::floor(origin.y)) / -normalizedDir.y;
    } else {
        tMax.y = std::numeric_limits<float>::max();
    }
    
    if (normalizedDir.z != 0) {
        tMax.z = (normalizedDir.z > 0) ? 
            (std::floor(origin.z) + 1.0f - origin.z) / normalizedDir.z :
            (origin.z - std::floor(origin.z)) / -normalizedDir.z;
    } else {
        tMax.z = std::numeric_limits<float>::max();
    }
    
    // Calculate tDelta - distance between grid lines
    glm::vec3 tDelta;
    tDelta.x = (normalizedDir.x != 0) ? std::abs(1.0f / normalizedDir.x) : std::numeric_limits<float>::max();
    tDelta.y = (normalizedDir.y != 0) ? std::abs(1.0f / normalizedDir.y) : std::numeric_limits<float>::max();
    tDelta.z = (normalizedDir.z != 0) ? std::abs(1.0f / normalizedDir.z) : std::numeric_limits<float>::max();
    
    glm::ivec3 normal(0);
    
    float distance = 0.0f;
    
    // Step through the grid, but only checking blocks in chunks from the octree
    while (distance < maxDistance) {
        // Check if the current position is in one of the chunks returned by the octree
        bool inLoadedChunk = false;
        for (const auto* chunk : chunksAlongRay) {
            // Convert world position to chunk-local coordinates
            glm::ivec3 chunkPos = chunk->getChunkPosition();
            glm::ivec3 localPos = current - chunkPos * Chunk::CHUNK_SIZE;
            
            // Check if within this chunk's bounds
            if (localPos.x >= 0 && localPos.x < Chunk::CHUNK_SIZE &&
                localPos.y >= 0 && localPos.y < Chunk::CHUNK_SIZE &&
                localPos.z >= 0 && localPos.z < Chunk::CHUNK_SIZE) {
                inLoadedChunk = true;
                
                // Get block directly from chunk for better performance
                BlockType blockType = chunk->getBlock(localPos.x, localPos.y, localPos.z);
                
                if (blockType != BlockType::AIR) {
                    RaycastHit hit;
                    hit.blockPos = current;
                    hit.previousPos = current - normal;
                    hit.normal = normal;
                    hit.distance = distance;
                    hit.blockType = blockType;
                    hit.hitPoint = origin + normalizedDir * distance;
                    
                    return hit;
                }
                
                break; // Found the correct chunk, no need to check others
            }
        }
        
        // If current position is not in any loaded chunk, fall back to regular getBlock
        // This handles the case where chunks might not be properly loaded in the octree yet
        if (!inLoadedChunk) {
            BlockType blockType = chunkManager->getBlock(glm::vec3(current));
            
            if (blockType != BlockType::AIR) {
                RaycastHit hit;
                hit.blockPos = current;
                hit.previousPos = current - normal;
                hit.normal = normal;
                hit.distance = distance;
                hit.blockType = blockType;
                hit.hitPoint = origin + normalizedDir * distance;
                
                return hit;
            }
        }
        
        // Move to next grid cell
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            // X axis
            distance = tMax.x;
            tMax.x += tDelta.x;
            current.x += step.x;
            normal = glm::ivec3(-step.x, 0, 0);
        } else if (tMax.y < tMax.z) {
            // Y axis
            distance = tMax.y;
            tMax.y += tDelta.y;
            current.y += step.y;
            normal = glm::ivec3(0, -step.y, 0);
        } else {
            // Z axis
            distance = tMax.z;
            tMax.z += tDelta.z;
            current.z += step.z;
            normal = glm::ivec3(0, 0, -step.z);
        }
    }
    
    return std::nullopt;
}

glm::ivec3 Raycast::sign(const glm::vec3& v) {
    return glm::ivec3(
        v.x > 0 ? 1 : (v.x < 0 ? -1 : 0),
        v.y > 0 ? 1 : (v.y < 0 ? -1 : 0),
        v.z > 0 ? 1 : (v.z < 0 ? -1 : 0)
    );
}


} // namespace Zerith