#pragma once

#include <glm/glm.hpp>
#include <optional>
#include "chunk.h"

namespace Zerith {

class ChunkManager;

struct RaycastHit {
    glm::ivec3 blockPos;
    glm::ivec3 previousPos;
    glm::vec3 hitPoint;
    glm::ivec3 normal;
    float distance;
    BlockType blockType;
};

class Raycast {
public:
    static std::optional<RaycastHit> cast(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        ChunkManager* chunkManager
    );

private:
    static glm::ivec3 sign(const glm::vec3& v);
    static float intbound(float s, float ds);
};

} // namespace Zerith