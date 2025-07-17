#pragma once

#include "face_visibility_mask.h"
#include "extended_chunk_data.h"
#include "block_properties.h"
#include "blocks.h"

namespace Zerith {

/**
 * Generator for creating face visibility masks from chunk data.
 * Uses ExtendedChunkData for neighbor access and block properties for culling rules.
 */
class FaceVisibilityMaskGenerator {
public:
    /**
     * Generate face visibility mask for a chunk using extended chunk data.
     * 
     * @param extendedData 18x18x18 chunk data with neighbor borders
     * @return Pre-computed face visibility mask
     */
    static FaceVisibilityMask generateMask(const ExtendedChunkData& extendedData);

private:
    /**
     * Check if a face should be visible based on culling rules.
     * This replicates the logic from Chunk::isFaceVisible() but uses
     * ExtendedChunkData for O(1) neighbor access.
     * 
     * @param extendedData Extended chunk data with neighbors
     * @param x Local chunk X coordinate (0-15)
     * @param y Local chunk Y coordinate (0-15)
     * @param z Local chunk Z coordinate (0-15)
     * @param direction Face direction to check
     * @return true if face should be rendered
     */
    static bool isFaceVisibleInternal(const ExtendedChunkData& extendedData,
                                    int x, int y, int z,
                                    FaceVisibilityMask::FaceDirection direction);
    
    /**
     * Convert face direction to offset vector.
     */
    static glm::ivec3 getDirectionOffset(FaceVisibilityMask::FaceDirection direction);
    
    /**
     * Get opposite face index for culling checks.
     * When checking if our face is culled by adjacent block,
     * we need to check the adjacent block's opposite face.
     */
    static int getOppositeFaceIndex(FaceVisibilityMask::FaceDirection direction);
};

} // namespace Zerith