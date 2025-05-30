#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Zerith {

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() = default;
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}
    
    static AABB fromCenterAndSize(const glm::vec3& center, const glm::vec3& size);
    static AABB fromBlock(const glm::ivec3& blockPos);
    
    bool intersects(const AABB& other) const;
    glm::vec3 getCenter() const;
    glm::vec3 getSize() const;
    
    void translate(const glm::vec3& offset);
    AABB translated(const glm::vec3& offset) const;
    
    bool contains(const glm::vec3& point) const;
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