#pragma once
#include <vector>
#include <array>
#include <cstdint>

#include "QuadInstance.hpp"

class ChunkStorage {
public:
    static const int CHUNK_SIZE = 16;
    using BlockGrid = std::array<std::array<std::array<bool, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE>;

    static uint32_t createInstanceData(FaceType face, uint32_t x, uint32_t y, uint32_t z) {
        return (static_cast<uint32_t>(face) & 0x7) |
               ((x & 0x1F) << 3) |
               ((y & 0x1F) << 8) |
               ((z & 0x1F) << 13);
    }

    // Check if a block should have a face based on neighbors
    static bool shouldCreateFace(const BlockGrid& blocks, int x, int y, int z, FaceType face) {
        // If this position is empty, no faces needed
        if (!blocks[x][y][z]) return false;

        // Check neighbors based on face type
        switch(face) {
            case FaceType::XNegative: // Left
                return x == 0 || !blocks[x-1][y][z];
            case FaceType::XPositive: // Right
                return x == CHUNK_SIZE-1 || !blocks[x+1][y][z];
            case FaceType::YNegative: // Back
                return y == 0 || !blocks[x][y-1][z];
            case FaceType::YPositive: // Front
                return y == CHUNK_SIZE-1 || !blocks[x][y+1][z];
            case FaceType::ZNegative: // Bottom
                return z == 0 || !blocks[x][y][z-1];
            case FaceType::ZPositive: // Top
                return z == CHUNK_SIZE-1 || !blocks[x][y][z+1];
        }
        return false;
    }

    // Generate instance data for all visible faces in the chunk
    static std::vector<uint32_t> generateVisibleFaces(const BlockGrid& blocks) {
        std::vector<uint32_t> instances;
        // Reserve a reasonable amount of space (can be tuned based on expected occupancy)
        instances.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 3);

        // Check each block position
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    // If there's a block here, check which faces should be visible
                    if (blocks[x][y][z]) {
                        // Check each face
                        if (shouldCreateFace(blocks, x, y, z, FaceType::XNegative))
                            instances.push_back(createInstanceData(FaceType::XNegative, x, y, z));
                        if (shouldCreateFace(blocks, x, y, z, FaceType::XPositive))
                            instances.push_back(createInstanceData(FaceType::XPositive, x, y, z));
                        if (shouldCreateFace(blocks, x, y, z, FaceType::YNegative))
                            instances.push_back(createInstanceData(FaceType::YNegative, x, y, z));
                        if (shouldCreateFace(blocks, x, y, z, FaceType::YPositive))
                            instances.push_back(createInstanceData(FaceType::YPositive, x, y, z));
                        if (shouldCreateFace(blocks, x, y, z, FaceType::ZNegative))
                            instances.push_back(createInstanceData(FaceType::ZNegative, x, y, z));
                        if (shouldCreateFace(blocks, x, y, z, FaceType::ZPositive))
                            instances.push_back(createInstanceData(FaceType::ZPositive, x, y, z));
                    }
                }
            }
        }

        return instances;
    }

    // Generate a test chunk with some blocks
    static BlockGrid generateTestChunk() {
        BlockGrid blocks = {}; // Initialize all to false

        // Create a simple pattern - a solid ground with some blocks on top
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                // Solid ground at z=0
                blocks[x][y][0] = true;

                // Add some random blocks above ground
                if ((x + y) % 3 == 0) {
                    blocks[x][y][1] = true;
                }
                if ((x * y) % 5 == 0) {
                    blocks[x][y][2] = true;
                }
            }
        }

        return blocks;
    }
};
