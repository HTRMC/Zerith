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

        // Pre-allocate with a reasonable size estimate (can be tuned based on typical usage)
        instances.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 3);
        outBlockTypes.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 3);

        // Visited masks for each face direction
        using VisitedArray = std::array<std::array<std::array<bool, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE>;
        VisitedArray visited{};

        // Helper function to check if a face can be included in the current merge
        auto canMergeFace = [&](int x, int y, int z, BlockType blockType, FaceType face) -> bool {
            return x >= 0 && x < CHUNK_SIZE &&
                   y >= 0 && y < CHUNK_SIZE &&
                   z >= 0 && z < CHUNK_SIZE &&
                   !visited[x][y][z] &&
                   blocks[x][y][z] == blockType &&
                   shouldCreateFace(blocks, x, y, z, face);
        };

        // For each face direction
        for (int faceType = 0; faceType < 6; faceType++) {
            auto face = static_cast<FaceType>(faceType);

            // Reset visited mask
            std::fill(&visited[0][0][0], &visited[0][0][0] + CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, false);

            // Scan each slice
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_SIZE; y++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        if (visited[x][y][z] || blocks[x][y][z] == BlockType::AIR ||
                            !shouldCreateFace(blocks, x, y, z, face)) {
                            continue;
                        }

                        // Determine scanning direction based on face type
                        bool scanX = (face == FaceType::XPositive || face == FaceType::XNegative ||
                                      face == FaceType::ZPositive || face == FaceType::ZNegative);
                        BlockType currentType = blocks[x][y][z];

                        // Find maximum width (constrained by chunk size and face orientation)
                        uint32_t width = 1;
                        uint32_t maxWidth = std::min<uint32_t>(
                            15, // Maximum value that fits in 4 bits
                            scanX ? CHUNK_SIZE - x : CHUNK_SIZE - y
                        );

                        for (uint32_t w = 1; w < maxWidth; w++) {
                            int nextX = x + (scanX ? w : 0);
                            int nextY = y + (scanX ? 0 : w);

                            if (!canMergeFace(nextX, nextY, z, currentType, face)) {
                                break;
                            }
                            width++;
                        }

                        // Find maximum height (constrained by chunk size and 4-bit limit)
                        uint32_t height = 1;
                        uint32_t maxHeight = std::min<uint32_t>(
                            15, // Maximum value that fits in 4 bits
                            CHUNK_SIZE - z
                        );

                        // Check each potential row for merging
                        for (uint32_t h = 1; h < maxHeight; h++) {
                            bool rowValid = true;

                            // Verify entire row can be merged
                            for (uint32_t w = 0; w < width; w++) {
                                int testX = x + (scanX ? w : 0);
                                int testY = y + (scanX ? 0 : w);
                                int testZ = z + h;

                                if (!canMergeFace(testX, testY, testZ, currentType, face)) {
                                    rowValid = false;
                                    break;
                                }
                            }

                            if (!rowValid) break;
                            height++;
                        }

                        // Only create an instance if we found a valid merge opportunity
                        if (width > 0 && height > 0) {
                            // Mark all covered blocks as visited
                            for (uint32_t h = 0; h < height; h++) {
                                for (uint32_t w = 0; w < width; w++) {
                                    int visitX = x + (scanX ? w : 0);
                                    int visitY = y + (scanX ? 0 : w);
                                    int visitZ = z + h;
                                    visited[visitX][visitY][visitZ] = true;
                                }
                            }

                            // Create and pack instance data
                            InstanceData instance{
                                face,
                                static_cast<uint32_t>(x),
                                static_cast<uint32_t>(y),
                                static_cast<uint32_t>(z),
                                width,
                                height
                            };

                            instances.push_back(packInstanceData(instance));
                            outBlockTypes.push_back(static_cast<uint32_t>(currentType));
                        }
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
