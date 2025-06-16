#pragma once

#include "chunk.h"
#include "world_constants.h"
#include <FastNoise/FastNoise.h>
#include <memory>
#include <glm/glm.hpp>

namespace Zerith {

class TerrainGenerator {
public:
    TerrainGenerator();
    
    void generateTerrain(Chunk& chunk);
    
    void setSeed(int seed);
    
private:
    FastNoise::SmartNode<> m_heightNoise;
    FastNoise::SmartNode<> m_caveNoise;
    
    int m_seed = 1337;
    int m_seaLevel = 64;
    
    int getTerrainHeight(int worldX, int worldZ);
    BlockType getBlockTypeForPosition(int worldX, int worldY, int worldZ, int terrainHeight);
};

} // namespace Zerith