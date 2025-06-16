#include "blocks/water_block.h"
#include "logger.h"

namespace Zerith {

void WaterBlock::onPlayerEnter(const glm::vec3& playerPos) {
    LOG_DEBUG("Player entered water at position (%.2f, %.2f, %.2f)", 
              playerPos.x, playerPos.y, playerPos.z);
}

void WaterBlock::onPlayerExit(const glm::vec3& playerPos) {
    LOG_DEBUG("Player exited water at position (%.2f, %.2f, %.2f)", 
              playerPos.x, playerPos.y, playerPos.z);
}

} // namespace Zerith