#include "voxel_ao.h"
#include "block_properties.h"

namespace Zerith
{
    // Debug variables
    bool VoxelAO::s_debugMode = false;
    glm::vec4 VoxelAO::s_debugAO = glm::vec4(1.0f, 0.8f, 0.6f, 0.4f); // Default test pattern
    float VoxelAO::s_aoMultiplier = 1.0f;

    float VoxelAO::calculateVertexAO(const Chunk& chunk,
                                     int x, int y, int z,
                                     int dx1, int dy1, int dz1,
                                     int dx2, int dy2, int dz2,
                                     int dx3, int dy3, int dz3)
    {
        // 0fps.net algorithm: Check the two edge neighbors and corner
        bool side1 = checkOcclusion(chunk, x + dx1, y + dy1, z + dz1) != 0;
        bool side2 = checkOcclusion(chunk, x + dx2, y + dy2, z + dz2) != 0;
        bool corner = checkOcclusion(chunk, x + dx3, y + dy3, z + dz3) != 0;

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

    glm::vec4 VoxelAO::calculateFaceAO(const Chunk& chunk, int x, int y, int z, int faceDirection)
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
            ao.x = calculateVertexAO(chunk, x, y - 1, z, 1, 0, 0, 0, 0, 1, 1, 0, 1); // +X, +Z vertex
            ao.y = calculateVertexAO(chunk, x, y - 1, z, -1, 0, 0, 0, 0, -1, -1, 0, -1); // -X, -Z vertex
            ao.z = calculateVertexAO(chunk, x, y - 1, z, -1, 0, 0, 0, 0, 1, -1, 0, 1); // -X, +Z vertex
            ao.w = calculateVertexAO(chunk, x, y - 1, z, 1, 0, 0, 0, 0, -1, 1, 0, -1); // +X, -Z vertex
            break;

        case 1: // Up face (Y+)
            // Looking at top face, vertices are at Y+1 - fixed left-to-right order
            ao.x = calculateVertexAO(chunk, x, y + 1, z, -1, 0, 0, 0, 0, 1, -1, 0, 1); // -X, +Z vertex (top-left)
            ao.y = calculateVertexAO(chunk, x, y + 1, z, -1, 0, 0, 0, 0, -1, -1, 0, -1); // -X, -Z vertex (bottom-left)
            ao.z = calculateVertexAO(chunk, x, y + 1, z, 1, 0, 0, 0, 0, 1, 1, 0, 1); // +X, +Z vertex (top-right)
            ao.w = calculateVertexAO(chunk, x, y + 1, z, 1, 0, 0, 0, 0, -1, 1, 0, -1); // +X, -Z vertex (bottom-right)
            break;

        case 2: // North face (Z-)
            // Looking at front face, vertices are at Z-1
            ao.x = calculateVertexAO(chunk, x, y, z - 1, 1, 0, 0, 0, 1, 0, 1, 1, 0); // +X, +Y vertex
            ao.y = calculateVertexAO(chunk, x, y, z - 1, -1, 0, 0, 0, -1, 0, -1, -1, 0); // -X, -Y vertex
            ao.z = calculateVertexAO(chunk, x, y, z - 1, -1, 0, 0, 0, 1, 0, -1, 1, 0); // -X, +Y vertex
            ao.w = calculateVertexAO(chunk, x, y, z - 1, 1, 0, 0, 0, -1, 0, 1, -1, 0); // +X, -Y vertex
            break;

        case 3: // South face (Z+)
            // Looking at back face, vertices are at Z+1
            ao.x = calculateVertexAO(chunk, x, y, z + 1, -1, 0, 0, 0, 1, 0, -1, 1, 0); // -X, +Y vertex (flipped)
            ao.y = calculateVertexAO(chunk, x, y, z + 1, 1, 0, 0, 0, -1, 0, 1, -1, 0); // +X, -Y vertex (flipped)
            ao.z = calculateVertexAO(chunk, x, y, z + 1, 1, 0, 0, 0, 1, 0, 1, 1, 0); // +X, +Y vertex (flipped)
            ao.w = calculateVertexAO(chunk, x, y, z + 1, -1, 0, 0, 0, -1, 0, -1, -1, 0); // -X, -Y vertex (flipped)
            break;

        case 4: // West face (X-)
            // Looking at left face, vertices are at X-1
            ao.x = calculateVertexAO(chunk, x - 1, y, z, 0, 0, -1, 0, 1, 0, 0, 1, 1); // +Z, +Y vertex
            ao.y = calculateVertexAO(chunk, x - 1, y, z, 0, 0, 1, 0, -1, 0, 0, -1, -1); // -Z, -Y vertex
            ao.z = calculateVertexAO(chunk, x - 1, y, z, 0, 0, 1, 0, 1, 0, 0, 1, -1); // -Z, +Y vertex
            ao.w = calculateVertexAO(chunk, x - 1, y, z, 0, 0, -1, 0, -1, 0, 0, -1, 1); // +Z, -Y vertex
            break;

        case 5: // East face (X+)
            // Looking at right face, vertices are at X+1
            ao.x = calculateVertexAO(chunk, x + 1, y, z, 0, 0, -1, 0, 1, 0, 0, 1, -1); // -Z, +Y vertex (flipped)
            ao.y = calculateVertexAO(chunk, x + 1, y, z, 0, 0, 1, 0, -1, 0, 0, -1, 1); // +Z, -Y vertex (flipped)
            ao.z = calculateVertexAO(chunk, x + 1, y, z, 0, 0, 1, 0, 1, 0, 0, 1, 1); // +Z, +Y vertex (flipped)
            ao.w = calculateVertexAO(chunk, x + 1, y, z, 0, 0, -1, 0, -1, 0, 0, -1, -1); // -Z, -Y vertex (flipped)
            break;
        }

        // Apply multiplier to final AO values
        ao *= s_aoMultiplier;
        ao = glm::clamp(ao, 0.0f, 1.0f);

        return ao;
    }

    glm::vec4 VoxelAO::calculateFaceAODebug(const Chunk& chunk, int x, int y, int z, int faceDirection)
    {
        glm::vec4 ao(1.0f);

        // For debugging, let's visualize neighbor positions as colors
        // This will help you see which vertices correspond to which neighbors
        switch (faceDirection)
        {
        case 2:
            {
                // North face - easiest to debug
                // Check each vertex's neighbors and assign debug colors
                int tl_left = checkOcclusion(chunk, x - 1, y, z); // Left neighbor
                int tl_up = checkOcclusion(chunk, x, y + 1, z); // Up neighbor
                int tl_corner = checkOcclusion(chunk, x - 1, y + 1, z); // Corner neighbor

                int bl_left = checkOcclusion(chunk, x - 1, y, z); // Left neighbor
                int bl_down = checkOcclusion(chunk, x, y - 1, z); // Down neighbor
                int bl_corner = checkOcclusion(chunk, x - 1, y - 1, z); // Corner neighbor

                int tr_right = checkOcclusion(chunk, x + 1, y, z); // Right neighbor
                int tr_up = checkOcclusion(chunk, x, y + 1, z); // Up neighbor
                int tr_corner = checkOcclusion(chunk, x + 1, y + 1, z); // Corner neighbor

                int br_right = checkOcclusion(chunk, x + 1, y, z); // Right neighbor
                int br_down = checkOcclusion(chunk, x, y - 1, z); // Down neighbor
                int br_corner = checkOcclusion(chunk, x + 1, y - 1, z); // Corner neighbor

                // Color code: 1.0=white (no occlusion), 0.5=gray (some), 0.0=black (full)
                ao.x = getAOStrength(tl_left, tl_up, tl_corner); // TL
                ao.y = getAOStrength(bl_left, bl_down, bl_corner); // BL
                ao.z = getAOStrength(tr_right, tr_up, tr_corner); // TR
                ao.w = getAOStrength(br_right, br_down, br_corner); // BR
                break;
            }
        default:
            // For other faces, just return the normal calculation
            return calculateFaceAO(chunk, x, y, z, faceDirection);
        }

        return ao;
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

    int VoxelAO::checkOcclusion(const Chunk& chunk, int x, int y, int z)
    {
        // Check bounds
        if (x < 0 || x >= Chunk::CHUNK_SIZE ||
            y < 0 || y >= Chunk::CHUNK_SIZE ||
            z < 0 || z >= Chunk::CHUNK_SIZE)
        {
            // Treat out-of-bounds as occluding (conservative approach)
            // This ensures consistent AO at chunk boundaries
            return 1;
        }

        BlockType blockType = chunk.getBlock(x, y, z);
        return isBlockOccluding(blockType) ? 1 : 0;
    }
} // namespace Zerith
