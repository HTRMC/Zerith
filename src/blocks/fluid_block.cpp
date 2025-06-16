#include "blocks/fluid_block.h"
#include "logger.h"

namespace Zerith {

FluidBlock::FluidBlock(const BlockSettings& settings) {
    // FluidBlock doesn't need to store settings since behavior is hardcoded
}

bool FluidBlock::hasCollision() const {
    // Fluids generally don't have collision, but can be overridden for surface walking
    return false;
}

bool FluidBlock::hasCollisionAt(const glm::vec3& position, const AABB& entityAABB) const {
    if (!m_allowSurfaceWalking) {
        return false;
    }
    
    // Allow walking on top surface of fluid (similar to Minecraft)
    // Check if entity is above the fluid block and moving downward
    float fluidTop = std::floor(position.y) + 1.0f; // Top of the fluid block
    float entityBottom = entityAABB.min.y;
    
    // Provide collision only if entity is above fluid surface with small tolerance
    const float surfaceTolerance = 0.1f;
    bool isAboveSurface = entityBottom >= (fluidTop - surfaceTolerance);
    
    return isAboveSurface;
}

void FluidBlock::onPlayerEnter(const glm::vec3& playerPos) {
    LOG_DEBUG("Player entered fluid at position (%.2f, %.2f, %.2f)", 
              playerPos.x, playerPos.y, playerPos.z);
}

void FluidBlock::onPlayerExit(const glm::vec3& playerPos) {
    LOG_DEBUG("Player exited fluid at position (%.2f, %.2f, %.2f)", 
              playerPos.x, playerPos.y, playerPos.z);
}

} // namespace Zerith