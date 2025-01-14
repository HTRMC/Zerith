#pragma once
#include <vector>
#include <array>
#include <cstdint>

#include "BlockType.hpp"
#include "ChunkManager.hpp"
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

    static uint32_t packInstanceData(const InstanceData& data) {
        return (static_cast<uint32_t>(data.face) & 0x7) |           // Face type (3 bits)
               ((data.x & 0x1F) << 3) |                             // World X position (5 bits)
               ((data.y & 0x1F) << 8) |                             // World Y position (5 bits)
               ((data.z & 0x1F) << 13) |                            // World Z position (5 bits)
               ((static_cast<uint32_t>(data.blockType) & 0x3F) << 18); // Block type (6 bits)
    }

    // Generate instance data for all visible faces in the chunk
    static std::vector<uint32_t> generateVisibleFaces(const BlockGrid& blocks, const ChunkPos& pos) {
        std::vector<uint32_t> instances;
        instances.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 3);

        // Calculate world position offset for this chunk
        uint32_t worldX = pos.x * CHUNK_SIZE;
        uint32_t worldY = pos.y * CHUNK_SIZE;
        uint32_t worldZ = pos.z * CHUNK_SIZE;

        // Check each block position
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    BlockType blockType = blocks[x][y][z];

                    if (blockType != BlockType::AIR) {
                        for (int f = 0; f < 6; f++) {
                            FaceType face = static_cast<FaceType>(f);
                            if (shouldCreateFace(blocks, x, y, z, face)) {
                                InstanceData data{face,
                                    worldX + static_cast<uint32_t>(x),
                                    worldY + static_cast<uint32_t>(y),
                                    worldZ + static_cast<uint32_t>(z),
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

    // Generate a chunk with terrain
    static BlockGrid generateChunk(const ChunkPos& pos) {
        BlockGrid blocks = {};  // Initialize all to AIR
        static PerlinNoise noise(42); // Use static to keep same seed

        // Generate heightmap for the chunk
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                // Calculate world coordinates
                double worldX = (pos.x * CHUNK_SIZE + x) / static_cast<double>(CHUNK_SIZE) * 0.3;
                double worldY = (pos.y * CHUNK_SIZE + y) / static_cast<double>(CHUNK_SIZE) * 0.3;

                // Generate height value
                int height = static_cast<int>(
                    PerlinNoise::normalize(
                        noise.octaveNoise(worldX, worldY, 0.0, 4, 1.0),
                        1.0,
                        CHUNK_SIZE * 0.75
                    )
                );

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

private:
    // Check if a block should have a face based on neighbors
    static bool shouldCreateFace(const BlockGrid& blocks, int x, int y, int z, FaceType face) {
        if (blocks[x][y][z] == BlockType::AIR) return false;

        auto isOccluding = [](BlockType type) {
            return type != BlockType::AIR;
        };

        // Check neighbors based on face type
        switch(face) {
            case FaceType::XNegative:
                return x == 0 || !isOccluding(blocks[x-1][y][z]);
            case FaceType::XPositive:
                return x == CHUNK_SIZE-1 || !isOccluding(blocks[x+1][y][z]);
            case FaceType::YNegative:
                return y == 0 || !isOccluding(blocks[x][y-1][z]);
            case FaceType::YPositive:
                return y == CHUNK_SIZE-1 || !isOccluding(blocks[x][y+1][z]);
            case FaceType::ZNegative:
                return z == 0 || !isOccluding(blocks[x][y][z-1]);
            case FaceType::ZPositive:
                return z == CHUNK_SIZE-1 || !isOccluding(blocks[x][y][z+1]);
        }
        return false;
    }
};