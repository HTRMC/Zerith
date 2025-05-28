#pragma once

#include "chunk.h"
#include <FastNoise/FastNoise.h>
#include <memory>
#include <glm/glm.hpp>

namespace Zerith {

class TerrainGenerator {
public:
    TerrainGenerator();
    
    void generateTerrain(Chunk& chunk);
    
    void setSeed(int seed);
    void setHeightScale(float scale) { m_heightScale = scale; }
    void setSeaLevel(int level) { m_seaLevel = level; }
    
private:
    FastNoise::SmartNode<> m_heightNoise;
    FastNoise::SmartNode<> m_caveNoise;
    
    float m_heightScale = 32.0f;
    int m_seaLevel = 8;
    int m_seed = 1337;
    
    BlockType getBlockTypeForPosition(int worldX, int worldY, int worldZ, float heightValue, float caveValue);
};

} // namespace Zerith