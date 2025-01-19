#pragma once
#include <vector>
#include <array>
#include <cstdint>

#include "BlockType.hpp"
#include "PerlinNoise.hpp"
#include "QuadInstance.hpp"

class ChunkStorage {
public:
    static const int CHUNK_SIZE = 16;
    using BlockGrid = std::array<std::array<std::array<BlockType, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE>;

    struct InstanceData {
        FaceType face;
        uint32_t x, y, z;
        BlockType blockType;
    };

    struct ChunkPositionData {
        glm::vec3 position;
        uint32_t instanceOffset;
    };

    static uint32_t packInstanceData(const InstanceData& data) {
        return (static_cast<uint32_t>(data.face) & 0x7) |           // Face type (3 bits)
               ((data.x & 0xF) << 3) |                             // X position (4 bits)
               ((data.y & 0xF) << 7) |                             // Y position (4 bits)
               ((data.z & 0xF) << 11) |                            // Z position (4 bits)
               ((static_cast<uint32_t>(data.blockType) & 0x1FFF) << 15); // Block type (17 bits)
    }

    // Check if a block should have a face based on neighbors
    static bool shouldCreateFace(const BlockGrid& blocks, int x, int y, int z, FaceType face) {
        // If this position is air, no faces needed
        if (blocks[x][y][z] == BlockType::AIR) return false;

        // Function to check if neighbor block occludes this face
        auto isOccluding = [](BlockType type) {
            return type != BlockType::AIR;
        };

        // Check neighbors based on face type
        switch(face) {
            case FaceType::XNegative: // Left
                return x == 0 || !isOccluding(blocks[x-1][y][z]);
            case FaceType::XPositive: // Right
                return x == CHUNK_SIZE-1 || !isOccluding(blocks[x+1][y][z]);
            case FaceType::YNegative: // Back
                return y == 0 || !isOccluding(blocks[x][y-1][z]);
            case FaceType::YPositive: // Front
                return y == CHUNK_SIZE-1 || !isOccluding(blocks[x][y+1][z]);
            case FaceType::ZNegative: // Bottom
                return z == 0 || !isOccluding(blocks[x][y][z-1]);
            case FaceType::ZPositive: // Top
                return z == CHUNK_SIZE-1 || !isOccluding(blocks[x][y][z+1]);
        }
        return false;
    }

    // Generate instance data for all visible faces in the chunk
    static std::vector<uint32_t> generateVisibleFaces(const BlockGrid& blocks) {
        std::vector<uint32_t> instances;
        instances.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 3);

        // Check each block position
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    BlockType blockType = blocks[x][y][z];

                    // If there's a block here, check which faces should be visible
                    if (blockType != BlockType::AIR) {
                        // Check each face
                        for (int f = 0; f < 6; f++) {
                            FaceType face = static_cast<FaceType>(f);
                            if (shouldCreateFace(blocks, x, y, z, face)) {
                                InstanceData data{face, static_cast<uint32_t>(x),
                                                     static_cast<uint32_t>(y),
                                                     static_cast<uint32_t>(z),
                                                     blockType};
                                instances.push_back(packInstanceData(data));
                            }
                        }
                    }
                }
            }
        }

        return instances;
    }

    // Generate a test chunk with some blocks
    static BlockGrid generateTestChunk(int chunkX, int chunkY) {
        BlockGrid blocks = {};  // Initialize all to AIR
        static PerlinNoise noise(42);

        // Generate heightmap for the chunk
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                // Calculate world coordinates for noise
                double worldX = (chunkX * CHUNK_SIZE + x);
                double worldY = (chunkY * CHUNK_SIZE + y);

                // Scale coordinates for noise sampling
                const double NOISE_SCALE = 0.02;  // Controls terrain frequency
                double nx = worldX * NOISE_SCALE;
                double ny = worldY * NOISE_SCALE;

                // Base terrain height using multiple noise octaves
                double baseHeight = noise.octaveNoise(nx, ny, 0.0, 4, 0.5);

                // Add some medium-scale variation
                const double DETAIL_SCALE = 2.0;  // Controls detail variation scale
                double detailNoise = noise.octaveNoise(nx * DETAIL_SCALE, ny * DETAIL_SCALE, 1.0, 2, 0.5) * 0.2;

                // Combine noise layers and normalize
                double totalNoise = baseHeight + detailNoise;

                // Map noise to desired height range
                int height = static_cast<int>(
                    PerlinNoise::normalize(
                        totalNoise,
                        CHUNK_SIZE * 0.3,  // Minimum height (30% of chunk)
                        CHUNK_SIZE * 0.8   // Maximum height (80% of chunk)
                    )
                );

                // Ensure minimum height
                height = std::max(4, height);

                // Fill blocks from bottom to height
                for (int z = 0; z < height; z++) {
                    if (z == height - 1) {
                        blocks[x][y][z] = BlockType::GRASS_BLOCK;
                    } else if (z >= height - 4) {
                        blocks[x][y][z] = BlockType::DIRT;
                    } else {
                        blocks[x][y][z] = BlockType::STONE;
                    }
                }
            }
        }

        return blocks;
    }

    static std::vector<uint32_t> generateMultiChunk(std::vector<ChunkPositionData>& chunkPositions) {
        std::vector<uint32_t> allInstances;
        chunkPositions.clear();

        const int CHUNKS_PER_ROW = 32;
        const int START_OFFSET = -(CHUNKS_PER_ROW / 2);

        // Pre-generate all chunks first to calculate offsets
        std::vector<std::vector<uint32_t>> chunkInstances;
        uint32_t totalInstanceCount = 0;

        // First pass: generate all chunks and store instance counts
        for (int x = 0; x < CHUNKS_PER_ROW; x++) {
            for (int y = 0; y < CHUNKS_PER_ROW; y++) {
                auto chunkData = generateTestChunk(x + START_OFFSET, y + START_OFFSET);
                auto instances = generateVisibleFaces(chunkData);

                // Store instances for this chunk
                chunkInstances.push_back(instances);

                // Add chunk position with instance offset
                ChunkPositionData posData;
                posData.position = glm::vec3(
                    (x + START_OFFSET) * CHUNK_SIZE,
                    (y + START_OFFSET) * CHUNK_SIZE,
                    0
                );
                posData.instanceOffset = totalInstanceCount;  // Store the offset
                chunkPositions.push_back(posData);

                totalInstanceCount += instances.size();
            }
        }

        // Second pass: combine all instances
        allInstances.reserve(totalInstanceCount);
        for (const auto& instances : chunkInstances) {
            allInstances.insert(allInstances.end(), instances.begin(), instances.end());
        }

        return allInstances;
    }
};
