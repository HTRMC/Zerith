#include "terrain_generator.h"
#include "logger.h"
#include "block_types.h"
#include "world_constants.h"
#include <cmath>

namespace Zerith {

TerrainGenerator::TerrainGenerator() {
    // OpenSimplex2 noise source
    auto openSimplex2 = FastNoise::New<FastNoise::OpenSimplex2>();
    
    // Fractal FBm with specified parameters
    auto fractalFBm = FastNoise::New<FastNoise::FractalFBm>();
    fractalFBm->SetSource(openSimplex2);
    fractalFBm->SetOctaveCount(4);
    fractalFBm->SetLacunarity(2.5f);
    fractalFBm->SetGain(0.65f);
    fractalFBm->SetWeightedStrength(0.5f);
    
    // Domain Scale
    auto domainScale = FastNoise::New<FastNoise::DomainScale>();
    domainScale->SetSource(fractalFBm);
    domainScale->SetScale(0.66f);
    
    // Position Output for offset
    auto positionOutput = FastNoise::New<FastNoise::PositionOutput>();
    positionOutput->Set<FastNoise::Dim::X>(0.0f, 0.0f);
    positionOutput->Set<FastNoise::Dim::Y>(3.0f, 0.0f);
    positionOutput->Set<FastNoise::Dim::Z>(0.0f, 0.0f);
    positionOutput->Set<FastNoise::Dim::W>(-7.24f, 0.0f);
    
    // Add node to combine position output with domain scale
    auto addNode = FastNoise::New<FastNoise::Add>();
    addNode->SetLHS(positionOutput);
    addNode->SetRHS(domainScale);
    
    // Domain Warp Gradient
    auto domainWarp = FastNoise::New<FastNoise::DomainWarpGradient>();
    domainWarp->SetSource(addNode);
    domainWarp->SetWarpAmplitude(0.2f);
    domainWarp->SetWarpFrequency(2.0f);
    
    m_heightNoise = domainWarp;
    
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
    
    // Clamp terrain height to world bounds
    terrainHeight = std::min(terrainHeight, static_cast<float>(WORLD_MAX_Y - 1));
    terrainHeight = std::max(terrainHeight, static_cast<float>(WORLD_MIN_Y));
    
    // Check world bounds
    if (worldY < WORLD_MIN_Y || worldY > WORLD_MAX_Y) {
        return BlockTypes::AIR;
    }
    
    // Above terrain height
    if (worldY > terrainHeight) {
        // All air blocks below sea level become water
        if (worldY <= m_seaLevel) {
            return BlockTypes::WATER;
        }
        return BlockTypes::AIR;
    }
    
    // Surface layer - grass block at the surface
    if (worldY == static_cast<int>(terrainHeight) && worldY > m_seaLevel) {
        return BlockTypes::GRASS_BLOCK;
    }
    
    // Dirt layer (3 blocks deep from surface)
    if (worldY > terrainHeight - 4 && worldY <= terrainHeight) {
        return BlockTypes::DIRT;
    }
    
    // Everything else is stone
    return BlockTypes::STONE;
}

} // namespace Zerith