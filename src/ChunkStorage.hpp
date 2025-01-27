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
        uint32_t width, height;
    };

    struct ChunkPositionData {
        glm::vec3 position;
        uint32_t instanceStart;
    };

    static uint32_t packInstanceData(const InstanceData &data) {
        return (static_cast<uint32_t>(data.face) & 0x7) | // Face type (3 bits)
               ((data.x & 0xF) << 3) | // X position (4 bits)
               ((data.y & 0xF) << 7) | // Y position (4 bits)
               ((data.z & 0xF) << 11) | // Z position (4 bits)
               ((data.width & 0xF) << 15) | // Width (4 bits)
               ((data.height & 0xF) << 19); // Height (4 bits)
    }

    // Check if a block should have a face based on neighbors
    static bool shouldCreateFace(const BlockGrid &blocks, int x, int y, int z, FaceType face) {
        // If this position is air, no faces needed
        if (blocks[x][y][z] == BlockType::AIR) return false;

        // Function to check if neighbor block occludes this face
        auto isOccluding = [](BlockType type) {
            return type != BlockType::AIR;
        };

        // Check neighbors based on face type
        switch (face) {
            case FaceType::XNegative: // Left
                return x == 0 || !isOccluding(blocks[x - 1][y][z]);
            case FaceType::XPositive: // Right
                return x == CHUNK_SIZE - 1 || !isOccluding(blocks[x + 1][y][z]);
            case FaceType::YNegative: // Back
                return y == 0 || !isOccluding(blocks[x][y - 1][z]);
            case FaceType::YPositive: // Front
                return y == CHUNK_SIZE - 1 || !isOccluding(blocks[x][y + 1][z]);
            case FaceType::ZNegative: // Bottom
                return z == 0 || !isOccluding(blocks[x][y][z - 1]);
            case FaceType::ZPositive: // Top
                return z == CHUNK_SIZE - 1 || !isOccluding(blocks[x][y][z + 1]);
        }
        return false;
    }

    // Generate instance data for all visible faces in the chunk
static std::vector<uint32_t> generateVisibleFaces(const BlockGrid &blocks, std::vector<uint32_t> &outBlockTypes) {
    std::vector<uint32_t> instances;
    outBlockTypes.clear();

    std::vector<std::vector<std::vector<bool>>> processed(
        CHUNK_SIZE, std::vector<std::vector<bool>>(
            CHUNK_SIZE, std::vector<bool>(CHUNK_SIZE, false)));

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        FaceType face = static_cast<FaceType>(faceIndex);

        // Reset processed flags
        for (int x = 0; x < CHUNK_SIZE; x++)
            for (int y = 0; y < CHUNK_SIZE; y++)
                for (int z = 0; z < CHUNK_SIZE; z++)
                    processed[x][y][z] = false;

        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    if (processed[x][y][z] || !shouldCreateFace(blocks, x, y, z, face))
                        continue;

                    BlockType currentType = blocks[x][y][z];
                    uint32_t maxWidth = 1, maxHeight = 1;

                    // Determine merge directions based on face orientation
                    switch (face) {
                        case FaceType::XPositive:
                        case FaceType::XNegative: {
                            // Merge along Y and Z
                            while (y + maxWidth < CHUNK_SIZE &&
                                   !processed[x][y + maxWidth][z] &&
                                   blocks[x][y + maxWidth][z] == currentType &&
                                   shouldCreateFace(blocks, x, y + maxWidth, z, face)) {
                                maxWidth++;
                            }

                            bool canExtend = true;
                            while (z + maxHeight < CHUNK_SIZE && canExtend) {
                                for (uint32_t w = 0; w < maxWidth; w++) {
                                    if (processed[x][y + w][z + maxHeight] ||
                                        blocks[x][y + w][z + maxHeight] != currentType ||
                                        !shouldCreateFace(blocks, x, y + w, z + maxHeight, face)) {
                                        canExtend = false;
                                        break;
                                    }
                                }
                                if (canExtend) maxHeight++;
                            }
                            break;
                        }

                        case FaceType::YPositive:
                        case FaceType::YNegative: {
                            // Merge along X and Z
                            while (x + maxWidth < CHUNK_SIZE &&
                                   !processed[x + maxWidth][y][z] &&
                                   blocks[x + maxWidth][y][z] == currentType &&
                                   shouldCreateFace(blocks, x + maxWidth, y, z, face)) {
                                maxWidth++;
                            }

                            bool canExtend = true;
                            while (z + maxHeight < CHUNK_SIZE && canExtend) {
                                for (uint32_t w = 0; w < maxWidth; w++) {
                                    if (processed[x + w][y][z + maxHeight] ||
                                        blocks[x + w][y][z + maxHeight] != currentType ||
                                        !shouldCreateFace(blocks, x + w, y, z + maxHeight, face)) {
                                        canExtend = false;
                                        break;
                                    }
                                }
                                if (canExtend) maxHeight++;
                            }
                            break;
                        }

                        case FaceType::ZPositive:
                        case FaceType::ZNegative: {
                            // Original logic for top/bottom faces (merge along X and Y)
                            while (x + maxWidth < CHUNK_SIZE &&
                                   !processed[x + maxWidth][y][z] &&
                                   blocks[x + maxWidth][y][z] == currentType &&
                                   shouldCreateFace(blocks, x + maxWidth, y, z, face)) {
                                maxWidth++;
                            }

                            bool canExtend = true;
                            while (y + maxHeight < CHUNK_SIZE && canExtend) {
                                for (uint32_t w = 0; w < maxWidth; w++) {
                                    if (processed[x + w][y + maxHeight][z] ||
                                        blocks[x + w][y + maxHeight][z] != currentType ||
                                        !shouldCreateFace(blocks, x + w, y + maxHeight, z, face)) {
                                        canExtend = false;
                                        break;
                                    }
                                }
                                if (canExtend) maxHeight++;
                            }
                            break;
                        }
                    }

                    // Mark processed blocks
                    for (uint32_t dh = 0; dh < maxHeight; dh++) {
                        for (uint32_t dw = 0; dw < maxWidth; dw++) {
                            switch (face) {
                                case FaceType::XPositive:
                                case FaceType::XNegative:
                                    processed[x][y + dw][z + dh] = true;
                                    break;
                                case FaceType::YPositive:
                                case FaceType::YNegative:
                                    processed[x + dw][y][z + dh] = true;
                                    break;
                                case FaceType::ZPositive:
                                case FaceType::ZNegative:
                                    processed[x + dw][y + dh][z] = true;
                                    break;
                            }
                        }
                    }

                    InstanceData data{
                        face,
                        static_cast<uint32_t>(x),
                        static_cast<uint32_t>(y),
                        static_cast<uint32_t>(z),
                        maxWidth - 1,
                        maxHeight - 1
                    };

                    instances.push_back(packInstanceData(data));
                    outBlockTypes.push_back(static_cast<uint32_t>(currentType));
                }
            }
        }
    }

    return instances;
}

    // Generate a test chunk with some blocks
    static BlockGrid generateTestChunk(int chunkX, int chunkY) {
        BlockGrid blocks = {}; // Initialize all to AIR
        static PerlinNoise noise(42);

        // Generate heightmap for the chunk
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                // Calculate world coordinates for noise
                double worldX = (chunkX * CHUNK_SIZE + x);
                double worldY = (chunkY * CHUNK_SIZE + y);

                // Scale coordinates for noise sampling
                const double NOISE_SCALE = 0.02; // Controls terrain frequency
                double nx = worldX * NOISE_SCALE;
                double ny = worldY * NOISE_SCALE;

                // Base terrain height using multiple noise octaves
                double baseHeight = noise.octaveNoise(nx, ny, 0.0, 4, 0.5);

                // Add some medium-scale variation
                const double DETAIL_SCALE = 2.0; // Controls detail variation scale
                double detailNoise = noise.octaveNoise(nx * DETAIL_SCALE, ny * DETAIL_SCALE, 1.0, 2, 0.5) * 0.2;

                // Combine noise layers and normalize
                double totalNoise = baseHeight + detailNoise;

                // Map noise to desired height range
                int height = static_cast<int>(
                    PerlinNoise::normalize(
                        totalNoise,
                        CHUNK_SIZE * 0.3, // Minimum height (30% of chunk)
                        CHUNK_SIZE * 0.8 // Maximum height (80% of chunk)
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

    static std::vector<uint32_t> generateMultiChunk(
        std::vector<ChunkPositionData> &chunkPositions,
        std::vector<uint32_t> &chunkIndices,
        std::vector<uint32_t> &outBlockTypes) {
        std::vector<uint32_t> allInstances;
        chunkPositions.clear();
        chunkIndices.clear();
        outBlockTypes.clear();

        const int CHUNKS_PER_ROW = 32;
        const int START_OFFSET = -(CHUNKS_PER_ROW / 2);
        uint32_t currentOffset = 0;
        uint32_t chunkIndex = 0;

        // Pre-generate all chunks first to calculate offsets
        std::vector<std::pair<std::vector<uint32_t>, std::vector<uint32_t> > > chunkData; // <instances, blockTypes>
        uint32_t totalInstanceCount = 0;

        // First pass: generate all chunks and calculate total size
        for (int x = 0; x < CHUNKS_PER_ROW; x++) {
            for (int y = 0; y < CHUNKS_PER_ROW; y++) {
                auto chunk = generateTestChunk(x + START_OFFSET, y + START_OFFSET);
                std::vector<uint32_t> chunkBlockTypes;
                auto instances = generateVisibleFaces(chunk, chunkBlockTypes);

                // Store instances and block types for this chunk
                chunkData.push_back({instances, chunkBlockTypes});

                // Add chunk position with instance offset
                ChunkPositionData posData;
                posData.position = glm::vec3(
                    (x + START_OFFSET) * CHUNK_SIZE,
                    (y + START_OFFSET) * CHUNK_SIZE,
                    0
                );
                posData.instanceStart = currentOffset;
                chunkPositions.push_back(posData);

                currentOffset += instances.size();
                totalInstanceCount += instances.size();
                chunkIndex++;
            }
        }

        // Pre-allocate vectors to avoid reallocations
        allInstances.reserve(totalInstanceCount);
        chunkIndices.reserve(totalInstanceCount);
        outBlockTypes.reserve(totalInstanceCount);

        // Second pass: combine all instances and create chunk indices
        chunkIndex = 0;
        for (const auto &[instances, blockTypes]: chunkData) {
            allInstances.insert(allInstances.end(), instances.begin(), instances.end());
            outBlockTypes.insert(outBlockTypes.end(), blockTypes.begin(), blockTypes.end());
            chunkIndices.insert(chunkIndices.end(), instances.size(), chunkIndex);
            chunkIndex++;
        }

        return allInstances;
    }
};
