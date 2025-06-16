#include "terrain_generator.h"
#include "logger.h"
#include "block_types.h"
#include "world_constants.h"
#include <cmath>

namespace Zerith {

TerrainGenerator::TerrainGenerator() {
    // Simple 3D Perlin noise
    m_perlinNoise = FastNoise::New<FastNoise::Perlin>();
    
    LOG_INFO("TerrainGenerator initialized with 3D Perlin noise");
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
                
                float noiseValue = m_perlinNoise->GenSingle3D(worldX * 0.01f, worldY * 0.01f, worldZ * 0.01f, m_seed);
                
                BlockType blockType = getBlockTypeForPosition(worldX, worldY, worldZ, noiseValue);
                chunk.setBlock(x, y, z, blockType);
            }
        }
    }
}

BlockType TerrainGenerator::getBlockTypeForPosition(int worldX, int worldY, int worldZ, float noiseValue) {
    // Check world bounds
    if (worldY < WORLD_MIN_Y || worldY > WORLD_MAX_Y) {
        return BlockTypes::AIR;
    }
    
    // Simple 3D Perlin noise logic:
    // If noise value is below 0, place air block
    // If noise value is above 0, place stone block
    if (noiseValue < 0.0f) {
        return BlockTypes::AIR;
    } else {
        return BlockTypes::STONE;
    }
}

} // namespace Zerith