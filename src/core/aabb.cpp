#include "aabb.h"
#include "chunk_manager.h"
#include "chunk.h"
#include "blocks.h"
#include <algorithm>
#include <cmath>

namespace Zerith {

// Constexpr implementations moved to header file

CollisionSystem::CollisionResult CollisionSystem::checkAABBCollision(const AABB& a, const AABB& b) {
    CollisionResult result;
    
    if (!a.intersects(b)) {
        return result;
    }
    
    result.hasCollision = true;
    
    glm::vec3 overlapMin = glm::max(a.min, b.min);
    glm::vec3 overlapMax = glm::min(a.max, b.max);
    glm::vec3 overlap = overlapMax - overlapMin;
    
    if (overlap.x <= overlap.y && overlap.x <= overlap.z) {
        result.penetration.x = overlap.x;
        result.normal.x = (a.getCenter().x < b.getCenter().x) ? -1.0f : 1.0f;
    } else if (overlap.y <= overlap.z) {
        result.penetration.y = overlap.y;
        result.normal.y = (a.getCenter().y < b.getCenter().y) ? -1.0f : 1.0f;
    } else {
        result.penetration.z = overlap.z;
        result.normal.z = (a.getCenter().z < b.getCenter().z) ? -1.0f : 1.0f;
    }
    
    return result;
}

glm::vec3 CollisionSystem::resolveCollision(const AABB& movingBox, const AABB& staticBox, const glm::vec3& velocity) {
    CollisionResult collision = checkAABBCollision(movingBox, staticBox);
    
    if (!collision.hasCollision) {
        return velocity;
    }
    
    glm::vec3 resolvedVelocity = velocity;
    
    if (collision.normal.x != 0.0f) {
        resolvedVelocity.x = 0.0f;
    }
    if (collision.normal.y != 0.0f) {
        resolvedVelocity.y = 0.0f;
    }
    if (collision.normal.z != 0.0f) {
        resolvedVelocity.z = 0.0f;
    }
    
    return resolvedVelocity;
}

std::vector<AABB> CollisionSystem::getBlockAABBsInRegion(const AABB& region, ChunkManager* chunkManager) {
    std::vector<AABB> blockAABBs;
    
    // Use floor for both min and max, but subtract 1 from min to ensure we don't miss blocks
    glm::ivec3 minBlock = glm::floor(region.min);
    glm::ivec3 maxBlock = glm::floor(region.max);
    
    for (int x = minBlock.x; x <= maxBlock.x; ++x) {
        for (int y = minBlock.y; y <= maxBlock.y; ++y) {
            for (int z = minBlock.z; z <= maxBlock.z; ++z) {
                glm::vec3 blockPos(x, y, z);
                BlockType blockType = chunkManager->getBlock(blockPos);
                
                if (blockType != Blocks::AIR) {
                    // Check if the block has collision using the unified blocks system
                    if (Blocks::hasCollision(blockType)) {
                        blockAABBs.emplace_back(AABB::fromBlock(glm::ivec3(x, y, z)));
                    }
                }
            }
        }
    }
    
    return blockAABBs;
}

// AABBDebugRenderer implementation
void AABBDebugRenderer::clear() {
    m_debugData.clear();
}

void AABBDebugRenderer::addAABB(const AABB& aabb, const glm::vec3& color) {
    AABBDebugData data;
    data.min = glm::vec4(aabb.min, 0.0f);
    data.max = glm::vec4(aabb.max, 0.0f);
    data.color = glm::vec4(color, 1.0f);
    m_debugData.emplace_back(std::move(data));
}

void AABBDebugRenderer::addPlayerAABB(const AABB& aabb) {
    addAABB(aabb, glm::vec3(0.0f, 1.0f, 0.0f)); // Green for player
}

void AABBDebugRenderer::addBlockAABBs(const std::vector<AABB>& aabbs) {
    for (const auto& aabb : aabbs) {
        addAABB(aabb, glm::vec3(1.0f, 0.5f, 0.0f)); // Orange for blocks
    }
}

} // namespace Zerith