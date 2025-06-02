#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>

namespace Zerith {

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    constexpr AABB() = default;
    constexpr AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}
    
    static constexpr AABB fromCenterAndSize(const glm::vec3& center, const glm::vec3& size) {
        glm::vec3 halfSize = size * 0.5f;
        return AABB(center - halfSize, center + halfSize);
    }
    
    static constexpr AABB fromBlock(const glm::ivec3& blockPos) {
        return AABB(glm::vec3(blockPos), glm::vec3(blockPos) + glm::vec3(1.0f));
    }
    
    constexpr bool intersects(const AABB& other) const {
        return (min.x < other.max.x && max.x > other.min.x) &&
               (min.y < other.max.y && max.y > other.min.y) &&
               (min.z < other.max.z && max.z > other.min.z);
    }
    
    constexpr glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }
    
    constexpr glm::vec3 getSize() const {
        return max - min;
    }
    
    constexpr void translate(const glm::vec3& offset) {
        min += offset;
        max += offset;
    }
    
    constexpr AABB translated(const glm::vec3& offset) const {
        return AABB(min + offset, max + offset);
    }
    
    constexpr bool contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    // Check if this AABB completely contains another AABB
    constexpr bool contains(const AABB& other) const {
        return min.x <= other.min.x && max.x >= other.max.x &&
               min.y <= other.min.y && max.y >= other.max.y &&
               min.z <= other.min.z && max.z >= other.max.z;
    }
    
    // Get half the size of the AABB
    constexpr glm::vec3 getExtents() const {
        return (max - min) * 0.5f;
    }
    
    // Ray-AABB intersection test
    bool intersectsRay(const glm::vec3& origin, const glm::vec3& direction, float& t) const {
        // Check if ray direction is valid
        if (glm::length(direction) < 0.00001f) {
            return false;
        }
        
        // Normalize direction if needed
        glm::vec3 dir = glm::normalize(direction);
        
        // Calculate t values for each slab
        float tMin = -std::numeric_limits<float>::infinity();
        float tMax = std::numeric_limits<float>::infinity();
        
        for (int i = 0; i < 3; i++) {
            float component = (&dir.x)[i];
            
            // Ray parallel to slab
            if (std::abs(component) < 0.00001f) {
                // If origin outside slab, no intersection
                if ((&origin.x)[i] < (&min.x)[i] || (&origin.x)[i] > (&max.x)[i]) {
                    return false;
                }
                // Otherwise, ray is inside this slab, so continue to next slab
            } else {
                // Calculate distances to slab planes
                float invD = 1.0f / component;
                float t1 = ((&min.x)[i] - (&origin.x)[i]) * invD;
                float t2 = ((&max.x)[i] - (&origin.x)[i]) * invD;
                
                // Ensure t1 < t2
                if (t1 > t2) {
                    std::swap(t1, t2);
                }
                
                // Update tMin and tMax
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                
                // Check for no overlap
                if (tMin > tMax) {
                    return false;
                }
            }
        }
        
        // Ray intersects AABB, store distance
        t = tMin > 0 ? tMin : tMax; // Use tMin if positive, otherwise tMax
        
        // Only report hits in front of the ray origin
        return t >= 0;
    }
};

// Debug rendering data for AABBs
struct AABBDebugData {
    glm::vec4 min;  // xyz + padding
    glm::vec4 max;  // xyz + padding
    glm::vec4 color; // rgba
};

class CollisionSystem {
public:
    struct CollisionResult {
        bool hasCollision = false;
        glm::vec3 penetration = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
    };
    
    static CollisionResult checkAABBCollision(const AABB& a, const AABB& b);
    static glm::vec3 resolveCollision(const AABB& movingBox, const AABB& staticBox, const glm::vec3& velocity);
    static std::vector<AABB> getBlockAABBsInRegion(const AABB& region, class ChunkManager* chunkManager);
};

// AABB Debug Renderer for collecting AABBs to visualize
class AABBDebugRenderer {
public:
    void clear();
    void addAABB(const AABB& aabb, const glm::vec3& color = glm::vec3(1.0f, 0.0f, 0.0f));
    void addPlayerAABB(const AABB& aabb);
    void addBlockAABBs(const std::vector<AABB>& aabbs);
    
    const std::vector<AABBDebugData>& getDebugData() const { return m_debugData; }
    size_t getCount() const { return m_debugData.size(); }
    
private:
    std::vector<AABBDebugData> m_debugData;
};

} // namespace Zerith