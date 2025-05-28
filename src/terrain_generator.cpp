#include "terrain_generator.h"
#include "logger.h"
#include <cmath>

namespace Zerith {

TerrainGenerator::TerrainGenerator() {
    auto heightGenerator = FastNoise::New<FastNoise::Simplex>();
    auto heightFractal = FastNoise::New<FastNoise::FractalFBm>();
    heightFractal->SetSource(heightGenerator);
    heightFractal->SetOctaveCount(4);
    heightFractal->SetLacunarity(2.0f);
    heightFractal->SetGain(0.5f);
    m_heightNoise = heightFractal;
    
    auto caveGenerator = FastNoise::New<FastNoise::Perlin>();
    auto caveFractal = FastNoise::New<FastNoise::FractalFBm>();
    caveFractal->SetSource(caveGenerator);
    caveFractal->SetOctaveCount(3);
    caveFractal->SetLacunarity(2.0f);
    caveFractal->SetGain(0.6f);
    m_caveNoise = caveFractal;
    
    LOG_INFO("TerrainGenerator initialized with FastNoise2");
}

void TerrainGenerator::setSeed(int seed) {
    m_seed = seed;
}

void TerrainGenerator::generateTerrain(Chunk& chunk) {
    glm::ivec3 chunkPos = chunk.getChunkPosition();
    glm::ivec3 worldChunkStart = chunkPos * Chunk::CHUNK_SIZE;
    
    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                int worldX = worldChunkStart.x + x;
                int worldY = worldChunkStart.y + y;
                int worldZ = worldChunkStart.z + z;
                
                float heightValue = m_heightNoise->GenSingle2D(worldX * 0.01f, worldZ * 0.01f, m_seed);
                float caveValue = m_caveNoise->GenSingle3D(worldX * 0.05f, worldY * 0.05f, worldZ * 0.05f, m_seed + 1000);
                
                BlockType blockType = getBlockTypeForPosition(worldX, worldY, worldZ, heightValue, caveValue);
                chunk.setBlock(x, y, z, blockType);
            }
        }
    }
}

BlockType TerrainGenerator::getBlockTypeForPosition(int worldX, int worldY, int worldZ, float heightValue, float caveValue) {
    float terrainHeight = m_seaLevel + (heightValue * m_heightScale);
    
    if (worldY > terrainHeight) {
        return BlockType::AIR;
    }
    
    if (caveValue > 0.3f && worldY > 0) {
        return BlockType::AIR;
    }
    
    if (worldY > terrainHeight - 1 && worldY <= terrainHeight) {
        return BlockType::GRASS_BLOCK;
    }
    
    if (worldY > terrainHeight - 4) {
        return BlockType::DIRT;
    }
    
    return BlockType::STONE;
}

} // namespace Zerith