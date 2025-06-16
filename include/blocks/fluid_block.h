#pragma once

#include "blocks/block_behavior.h"
#include "aabb.h"

namespace Zerith {

// Forward declaration
class BlockSettings;

// Fluid block implementation similar to Minecraft's FluidBlock
class FluidBlock : public BlockBehavior {
public:
    explicit FluidBlock(const BlockSettings& settings);
    
    // Override collision behavior - fluids can have surface collision
    bool hasCollision() const override;
    
    // Check if collision should apply based on context
    bool hasCollisionAt(const glm::vec3& position, const AABB& entityAABB) const;
    
    // Players can walk through fluids
    bool isWalkThrough() const override { return true; }
    
    // Fluid-specific behavior
    void onPlayerEnter(const glm::vec3& playerPos) override;
    void onPlayerExit(const glm::vec3& playerPos) override;
    
    // Rendering properties
    bool shouldRender() const override { return true; }
    
private:
    bool m_allowSurfaceWalking = true; // Allow walking on top of fluids
};

} // namespace Zerith