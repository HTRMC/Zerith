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
    FastNoise::SmartNode<> m_perlinNoise;
    
    int m_seed = 1337;
    
    BlockType getBlockTypeForPosition(int worldX, int worldY, int worldZ, float noiseValue);
};

} // namespace Zerith