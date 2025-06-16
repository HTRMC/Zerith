#include "block_face_bounds.h"
#include "blocks.h"
#include "logger.h"

namespace Zerith {

void BlockFaceBoundsRegistry::initializeBlockBounds() {
    // This is now initialized by ChunkMeshGenerator when loading models
    // We just need to set AIR bounds here
    
    // AIR has no faces
    BlockFaceBounds airBounds;
    for (auto& face : airBounds.faces) {
        face = FaceBounds(0.0f, 0.0f, 0.0f, 0.0f);  // No coverage
    }
    setFaceBounds(Blocks::AIR, airBounds);
    
    LOG_INFO("BlockFaceBoundsRegistry initialized");
}

} // namespace Zerith