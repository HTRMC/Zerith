#include "voxel_ao.h"
#include "block_properties.h"
#include "chunk_manager.h"

namespace Zerith
{
    // Debug variables
    bool VoxelAO::s_debugMode = false;
    glm::vec4 VoxelAO::s_debugAO = glm::vec4(1.0f, 0.8f, 0.6f, 0.4f); // Default test pattern
    float VoxelAO::s_aoMultiplier = 1.0f;

    // Cross-chunk AO calculation
    float VoxelAO::calculateVertexAO(const ChunkManager* chunkManager, const glm::ivec3& chunkWorldPos,
                                     int x, int y, int z,
                                     int dx1, int dy1, int dz1,
                                     int dx2, int dy2, int dz2,
                                     int dx3, int dy3, int dz3)
    {
        // 0fps.net algorithm: Check the two edge neighbors and corner
        bool side1 = checkOcclusionCrossChunk(chunkManager, chunkWorldPos, x + dx1, y + dy1, z + dz1) != 0;
        bool side2 = checkOcclusionCrossChunk(chunkManager, chunkWorldPos, x + dx2, y + dy2, z + dz2) != 0;
        bool corner = checkOcclusionCrossChunk(chunkManager, chunkWorldPos, x + dx3, y + dy3, z + dz3) != 0;

        // 0fps.net formula: if both sides are blocked, return 0, otherwise count neighbors
        if (side1 && side2)
        {
            return 0.0f; // Fully occluded
        }

        // Count occluding neighbors
        int occluders = (side1 ? 1 : 0) + (side2 ? 1 : 0) + (corner ? 1 : 0);

        // 0fps.net AO values: 3 - occluders / 3
        return (3.0f - occluders) / 3.0f;
    }

    glm::vec4 VoxelAO::calculateFaceAO(const ChunkManager* chunkManager, const glm::ivec3& chunkWorldPos, int x, int y, int z, int faceDirection)
    {
        // Debug mode override
        if (s_debugMode)
        {
            return s_debugAO;
        }

        glm::vec4 ao(1.0f); // Default to no occlusion

        // 0fps.net algorithm: proper vertex neighbor sampling for each face
        // Each vertex samples 3 neighbors: 2 edge-adjacent + 1 corner
        switch (faceDirection)
        {
        case 0: // Down face (Y-)
            // Looking at bottom face, vertices are at Y-1
            ao.x = calculateVertexAO(chunkManager, chunkWorldPos, x, y - 1, z, 1, 0, 0, 0, 0, 1, 1, 0, 1); // +X, +Z vertex
            ao.y = calculateVertexAO(chunkManager, chunkWorldPos, x, y - 1, z, -1, 0, 0, 0, 0, -1, -1, 0, -1); // -X, -Z vertex
            ao.z = calculateVertexAO(chunkManager, chunkWorldPos, x, y - 1, z, -1, 0, 0, 0, 0, 1, -1, 0, 1); // -X, +Z vertex
            ao.w = calculateVertexAO(chunkManager, chunkWorldPos, x, y - 1, z, 1, 0, 0, 0, 0, -1, 1, 0, -1); // +X, -Z vertex
            break;

        case 1: // Up face (Y+)
            // Looking at top face, vertices are at Y+1 - fixed left-to-right order
            ao.x = calculateVertexAO(chunkManager, chunkWorldPos, x, y + 1, z, -1, 0, 0, 0, 0, 1, -1, 0, 1); // -X, +Z vertex (top-left)
            ao.y = calculateVertexAO(chunkManager, chunkWorldPos, x, y + 1, z, -1, 0, 0, 0, 0, -1, -1, 0, -1); // -X, -Z vertex (bottom-left)
            ao.z = calculateVertexAO(chunkManager, chunkWorldPos, x, y + 1, z, 1, 0, 0, 0, 0, 1, 1, 0, 1); // +X, +Z vertex (top-right)
            ao.w = calculateVertexAO(chunkManager, chunkWorldPos, x, y + 1, z, 1, 0, 0, 0, 0, -1, 1, 0, -1); // +X, -Z vertex (bottom-right)
            break;

        case 2: // North face (Z-)
            // Looking at front face, vertices are at Z-1
            ao.x = calculateVertexAO(chunkManager, chunkWorldPos, x, y, z - 1, 1, 0, 0, 0, 1, 0, 1, 1, 0); // +X, +Y vertex
            ao.y = calculateVertexAO(chunkManager, chunkWorldPos, x, y, z - 1, -1, 0, 0, 0, -1, 0, -1, -1, 0); // -X, -Y vertex
            ao.z = calculateVertexAO(chunkManager, chunkWorldPos, x, y, z - 1, -1, 0, 0, 0, 1, 0, -1, 1, 0); // -X, +Y vertex
            ao.w = calculateVertexAO(chunkManager, chunkWorldPos, x, y, z - 1, 1, 0, 0, 0, -1, 0, 1, -1, 0); // +X, -Y vertex
            break;

        case 3: // South face (Z+)
            // Looking at back face, vertices are at Z+1
            ao.x = calculateVertexAO(chunkManager, chunkWorldPos, x, y, z + 1, -1, 0, 0, 0, 1, 0, -1, 1, 0); // -X, +Y vertex (flipped)
            ao.y = calculateVertexAO(chunkManager, chunkWorldPos, x, y, z + 1, 1, 0, 0, 0, -1, 0, 1, -1, 0); // +X, -Y vertex (flipped)
            ao.z = calculateVertexAO(chunkManager, chunkWorldPos, x, y, z + 1, 1, 0, 0, 0, 1, 0, 1, 1, 0); // +X, +Y vertex (flipped)
            ao.w = calculateVertexAO(chunkManager, chunkWorldPos, x, y, z + 1, -1, 0, 0, 0, -1, 0, -1, -1, 0); // -X, -Y vertex (flipped)
            break;

        case 4: // West face (X-)
            // Looking at left face, vertices are at X-1
            ao.x = calculateVertexAO(chunkManager, chunkWorldPos, x - 1, y, z, 0, 0, -1, 0, 1, 0, 0, 1, 1); // +Z, +Y vertex
            ao.y = calculateVertexAO(chunkManager, chunkWorldPos, x - 1, y, z, 0, 0, 1, 0, -1, 0, 0, -1, -1); // -Z, -Y vertex
            ao.z = calculateVertexAO(chunkManager, chunkWorldPos, x - 1, y, z, 0, 0, 1, 0, 1, 0, 0, 1, -1); // -Z, +Y vertex
            ao.w = calculateVertexAO(chunkManager, chunkWorldPos, x - 1, y, z, 0, 0, -1, 0, -1, 0, 0, -1, 1); // +Z, -Y vertex
            break;

        case 5: // East face (X+)
            // Looking at right face, vertices are at X+1 - fixed forwards-to-backwards order
            ao.x = calculateVertexAO(chunkManager, chunkWorldPos, x + 1, y, z, 0, 0, 1, 0, 1, 0, 0, 1, 1); // +Z, +Y vertex (top-front)
            ao.y = calculateVertexAO(chunkManager, chunkWorldPos, x + 1, y, z, 0, 0, 1, 0, -1, 0, 0, -1, 1); // +Z, -Y vertex (bottom-front)
            ao.z = calculateVertexAO(chunkManager, chunkWorldPos, x + 1, y, z, 0, 0, -1, 0, 1, 0, 0, 1, -1); // -Z, +Y vertex (top-back)
            ao.w = calculateVertexAO(chunkManager, chunkWorldPos, x + 1, y, z, 0, 0, -1, 0, -1, 0, 0, -1, -1); // -Z, -Y vertex (bottom-back)
            break;
        }

        // Apply multiplier to final AO values
        ao *= s_aoMultiplier;
        ao = glm::clamp(ao, 0.0f, 1.0f);

        return ao;
    }



    glm::vec4 VoxelAO::calculateFaceAODebug(const Chunk& chunk, int x, int y, int z, int faceDirection)
    {
        // Debug function disabled - requires ChunkManager for cross-chunk AO
        // Return debug pattern instead
        return glm::vec4(1.0f, 0.8f, 0.6f, 0.4f);
    }

    bool VoxelAO::isBlockOccluding(BlockType blockType)
    {
        if (blockType == Blocks::AIR)
        {
            return false;
        }

        // Explicitly check for water - liquids don't cast AO shadows
        if (blockType == Blocks::WATER)
        {
            return false;
        }

        // Use block properties to determine if the block occludes
        const auto& props = BlockProperties::getCullingProperties(blockType);

        // Transparent blocks (glass, leaves) don't occlude for AO
        if (props.isTransparent)
        {
            return false;
        }

        // All solid blocks occlude
        return true;
    }

    float VoxelAO::getAOStrength(int side1, int side2, int corner)
    {
        // Legacy function - kept for compatibility
        // 0fps.net algorithm is now used in calculateVertexAO
        if (side1 && side2)
        {
            return 0.0f; // Fully occluded
        }

        int occluders = side1 + side2 + corner;
        return (3.0f - occluders) / 3.0f; // 0fps.net formula
    }

    int VoxelAO::checkOcclusionCrossChunk(const ChunkManager* chunkManager, const glm::ivec3& chunkWorldPos, int x, int y, int z)
    {
        // Convert chunk-relative coordinates to world coordinates
        glm::vec3 worldPos = glm::vec3(chunkWorldPos) + glm::vec3(x, y, z);
        
        // Use ChunkManager to get block at world position (handles cross-chunk access)
        BlockType blockType = chunkManager->getBlock(worldPos);
        return isBlockOccluding(blockType) ? 1 : 0;
    }

} // namespace Zerith
