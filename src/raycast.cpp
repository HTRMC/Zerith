#include "raycast.h"
#include "chunk_manager.h"
#include <cmath>
#include <algorithm>

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
    
    glm::ivec3 current(
        static_cast<int>(std::floor(origin.x)),
        static_cast<int>(std::floor(origin.y)),
        static_cast<int>(std::floor(origin.z))
    );
    
    glm::ivec3 step = sign(normalizedDir);
    
    glm::vec3 tMax(
        intbound(origin.x, normalizedDir.x),
        intbound(origin.y, normalizedDir.y),
        intbound(origin.z, normalizedDir.z)
    );
    
    glm::vec3 tDelta(
        step.x / normalizedDir.x,
        step.y / normalizedDir.y,
        step.z / normalizedDir.z
    );
    
    glm::ivec3 face(0);
    glm::ivec3 previous = current;
    
    float distanceTraveled = 0.0f;
    
    while (distanceTraveled < maxDistance) {
        BlockType blockType = chunkManager->getBlock(glm::vec3(current));
        
        if (blockType != BlockType::AIR) {
            RaycastHit hit;
            hit.blockPos = current;
            hit.previousPos = previous;
            hit.normal = -face;
            hit.distance = distanceTraveled;
            hit.blockType = blockType;
            
            glm::vec3 hitPos = origin + normalizedDir * distanceTraveled;
            hit.hitPoint = hitPos;
            
            return hit;
        }
        
        previous = current;
        
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                current.x += step.x;
                distanceTraveled = tMax.x;
                tMax.x += tDelta.x;
                face = glm::ivec3(-step.x, 0, 0);
            } else {
                current.z += step.z;
                distanceTraveled = tMax.z;
                tMax.z += tDelta.z;
                face = glm::ivec3(0, 0, -step.z);
            }
        } else {
            if (tMax.y < tMax.z) {
                current.y += step.y;
                distanceTraveled = tMax.y;
                tMax.y += tDelta.y;
                face = glm::ivec3(0, -step.y, 0);
            } else {
                current.z += step.z;
                distanceTraveled = tMax.z;
                tMax.z += tDelta.z;
                face = glm::ivec3(0, 0, -step.z);
            }
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

float Raycast::intbound(float s, float ds) {
    if (ds < 0) {
        return intbound(-s, -ds);
    } else {
        s = fmod(s, 1.0f);
        return (1 - s) / ds;
    }
}

} // namespace Zerith