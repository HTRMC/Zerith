#pragma once
#include <glm/vec3.hpp>
#include <algorithm>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    // Calculate the center of the AABB
    glm::vec3 centre() const {
        return (min + max) / 2.0f;
    }

    // Move the AABB to a new center
    AABB move_to(const glm::vec3& new_centre) const {
        return offset(new_centre - centre());
    }

    // Offset the AABB by a delta vector
    AABB offset(const glm::vec3& delta) const {
        return AABB{
            min + delta,
            max + delta
        };
    }

    // Symmetrically resize the AABB by a delta vector
    AABB resize_symmetric(const glm::vec3& delta) const {
        return AABB{
            min - delta,
            max + delta
        };
    }

    // Resize the AABB directionally
    AABB resize_directional(const glm::vec3& delta) const {
        return AABB{
            glm::min(min, min + delta),
            glm::max(max, max + delta)
        };
    }

    // Check if this AABB intersects with another AABB
    bool intersects_aabb(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    // Check if a position (vector) is inside this AABB
    bool intersects_pos(const glm::vec3& vec) const {
        return min.x <= vec.x && max.x >= vec.x &&
               min.y <= vec.y && max.y >= vec.y &&
               min.z <= vec.z && max.z >= vec.z;
    }
};
