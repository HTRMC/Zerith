#pragma once

#include "block_behavior.h"

namespace Zerith {

class WaterBlock : public BlockBehavior {
public:
    // Water blocks don't provide collision
    bool hasCollision() const override { return false; }
    
    // Players can walk through water
    bool isWalkThrough() const override { return true; }
    
    // Called when player enters water
    void onPlayerEnter(const glm::vec3& playerPos) override;
    
    // Called when player exits water
    void onPlayerExit(const glm::vec3& playerPos) override;
};

} // namespace Zerith