#include "terrain_generator.h"
#include "logger.h"
#include "block_types.h"
#include "world_constants.h"
#include <cmath>

namespace Zerith {

TerrainGenerator::TerrainGenerator() {
    // 2D height noise for terrain surface
    auto heightPerlin = FastNoise::New<FastNoise::Perlin>();
    auto heightFractal = FastNoise::New<FastNoise::FractalFBm>();
    heightFractal->SetSource(heightPerlin);
    heightFractal->SetOctaveCount(4);
    heightFractal->SetLacunarity(2.0f);
    heightFractal->SetGain(0.5f);
    m_heightNoise = heightFractal;
    
    // 3D cave noise for underground caves
    m_caveNoise = FastNoise::New<FastNoise::Perlin>();
    
    LOG_INFO("TerrainGenerator initialized with Minecraft-style terrain");
}

void TerrainGenerator::setSeed(int seed) {
    m_seed = seed;
}

void TerrainGenerator::generateTerrain(Chunk& chunk) {
    glm::ivec3 chunkPos = chunk.getChunkPosition();
    glm::ivec3 worldChunkStart = chunkPos * Chunk::CHUNK_SIZE;
    
    // Pre-calculate terrain heights for this chunk
    int terrainHeights[Chunk::CHUNK_SIZE][Chunk::CHUNK_SIZE];
    for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            int worldX = worldChunkStart.x + x;
            int worldZ = worldChunkStart.z + z;
            terrainHeights[x][z] = getTerrainHeight(worldX, worldZ);
        }
    }
    
    // Generate blocks
    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                int worldX = worldChunkStart.x + x;
                int worldY = worldChunkStart.y + y;
                int worldZ = worldChunkStart.z + z;
                
                BlockType blockType = getBlockTypeForPosition(worldX, worldY, worldZ, terrainHeights[x][z]);
                chunk.setBlock(x, y, z, blockType);
            }
        }
    }
}

int TerrainGenerator::getTerrainHeight(int worldX, int worldZ) {
    // Sample 2D noise for terrain height
    float heightNoise = m_heightNoise->GenSingle2D(worldX * 0.005f, worldZ * 0.005f, m_seed);
    
    // Convert to terrain height (around sea level with variation)
    int height = m_seaLevel + static_cast<int>(heightNoise * 40.0f); // ±40 blocks variation
    
    // Clamp to world bounds
    height = std::max(WORLD_MIN_Y, std::min(height, WORLD_MAX_Y - 10));
    
    return height;
}

BlockType TerrainGenerator::getBlockTypeForPosition(int worldX, int worldY, int worldZ, int terrainHeight) {
    // Check world bounds
    if (worldY < WORLD_MIN_Y || worldY > WORLD_MAX_Y) {
        return BlockTypes::AIR;
    }
    
    // Above terrain surface - air or water
    if (worldY > terrainHeight) {
        if (worldY <= m_seaLevel) {
            return BlockTypes::WATER;  // Water fills areas below sea level
        }
        return BlockTypes::AIR;
    }
    
    // Below terrain surface - check for caves first
    if (worldY < terrainHeight) {
        // Cave generation using 3D Perlin noise
        float caveNoise = m_caveNoise->GenSingle3D(worldX * 0.02f, worldY * 0.02f, worldZ * 0.02f, m_seed + 1000);
        
        // Create caves where noise is above threshold (adjust 0.6f to control cave density)
        if (caveNoise > 0.6f) {
            return BlockTypes::AIR;
        }
    }
    
    // Solid terrain - Minecraft-style layering
    if (worldY == terrainHeight && terrainHeight > m_seaLevel) {
        return BlockTypes::GRASS_BLOCK;  // Grass on surface above water
    } else if (worldY > terrainHeight - 4) {
        return BlockTypes::DIRT;  // Dirt layer (3-4 blocks deep)
    } else {
        return BlockTypes::STONE;  // Stone everywhere else
    }
}

} // namespace Zerith