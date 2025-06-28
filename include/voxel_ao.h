#pragma once

#include <glm/glm.hpp>
#include "chunk.h"
#include "blocks.h"

namespace Zerith {

// Forward declaration
class ChunkManager;

class VoxelAO {
public:
    // Calculate ambient occlusion value for a vertex with cross-chunk support
    // Returns a value between 0.0 (fully occluded) and 1.0 (no occlusion)
    static float calculateVertexAO(const ChunkManager* chunkManager, const glm::ivec3& chunkWorldPos,
                                  int x, int y, int z,
                                  int dx1, int dy1, int dz1,  // First neighbor offset
                                  int dx2, int dy2, int dz2,  // Second neighbor offset
                                  int dx3, int dy3, int dz3); // Corner neighbor offset
    
    // Calculate AO values for all 4 vertices of a face with cross-chunk support
    // Face direction: 0=down, 1=up, 2=north, 3=south, 4=west, 5=east
    static glm::vec4 calculateFaceAO(const ChunkManager* chunkManager, const glm::ivec3& chunkWorldPos, int x, int y, int z, int faceDirection);
    
    // Check if a block contributes to occlusion
    static bool isBlockOccluding(BlockType blockType);
    
    // Get AO strength based on number of occluding neighbors
    static float getAOStrength(int side1, int side2, int corner);
    
    // Debug functions for manual tweaking
    static void setDebugMode(bool enabled) { s_debugMode = enabled; }
    static void setDebugAOValues(float tl, float bl, float tr, float br) {
        s_debugAO = glm::vec4(tl, bl, tr, br);
    }
    static void setAOStrengthMultiplier(float multiplier) { s_aoMultiplier = multiplier; }
    
    // Debug visualization - returns AO as colors for easy identification
    static glm::vec4 calculateFaceAODebug(const Chunk& chunk, int x, int y, int z, int faceDirection);
    
    // Simple test function to validate AO logic
    static void printAODebugInfo(const Chunk& chunk, int x, int y, int z, int faceDirection);
    
private:
    // Check if a position has an occluding block (cross-chunk version)
    static int checkOcclusionCrossChunk(const ChunkManager* chunkManager, const glm::ivec3& chunkWorldPos, int x, int y, int z);
    
    // Debug variables
    static bool s_debugMode;
    static glm::vec4 s_debugAO;
    static float s_aoMultiplier;
};

} // namespace Zerith