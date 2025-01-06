#pragma once
#include <vector>
#include <array>
#include <cstdint>

#include "BlockType.hpp"
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
               ((data.x & 0x1F) << 3) |                             // X position (5 bits)
               ((data.y & 0x1F) << 8) |                             // Y position (5 bits)
               ((data.z & 0x1F) << 13) |                            // Z position (5 bits)
               ((static_cast<uint32_t>(data.blockType) & 0x3F) << 18); // Block type (6 bits)
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
    static BlockGrid generateTestChunk() {
        BlockGrid blocks = {};  // Initialize all to AIR

        // Create a simple pattern - a solid ground with some blocks on top
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                // Solid ground at z=0
                blocks[x][y][0] = BlockType::STONE;

                // Add some grass blocks above stone
                if ((x + y) % 3 == 0) {
                    blocks[x][y][1] = BlockType::GRASS_BLOCK;
                }
                if ((x * y) % 5 == 0) {
                    blocks[x][y][2] = BlockType::DIRT;
                }
            }
        }

        return blocks;
    }
};
