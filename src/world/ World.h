// World.h
#pragma once
#include "Chunk.h"
#include <unordered_map>
#include <glm/glm.hpp>

// AABB struct needs to be accessible to World class
struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

struct ChunkCoordHash {
    std::size_t operator()(const glm::ivec2& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
    }
};

class World {
public:
    static const int WORLD_SIZE = 2; // 5x5 chunks
    std::unordered_map<glm::ivec2, Chunk, ChunkCoordHash> chunks;

    World() {
        // Generate 5x5 chunks centered around (0,0)
        int offset = WORLD_SIZE / 2;
        for (int x = -offset; x <= offset; x++) {
            for (int z = -offset; z <= offset; z++) {
                glm::ivec2 chunkPos(x, z);
                // Use emplace with constructed Chunk
                auto [it, success] = chunks.emplace(chunkPos, Chunk(x, z));
                if (success) {
                    it->second.generateTerrain();
                }
            }
        }
    }

    Block* getBlock(int x, int y, int z) {
        // Convert world coordinates to chunk coordinates
        int chunkX = floor(float(x) / Chunk::CHUNK_SIZE);
        int chunkZ = floor(float(z) / Chunk::CHUNK_SIZE);

        // Convert world coordinates to local chunk coordinates
        int localX = x - (chunkX * Chunk::CHUNK_SIZE);
        int localZ = z - (chunkZ * Chunk::CHUNK_SIZE);

        // Check if the chunk exists
        glm::ivec2 chunkPos(chunkX, chunkZ);
        auto it = chunks.find(chunkPos);
        if (it != chunks.end() && y >= 0 && y < Chunk::CHUNK_SIZE) {
            return &it->second.blocks[localX][y][localZ];
        }
        return nullptr;
    }

    bool checkCollision(const AABB& playerBox) {
        // Convert AABB to chunk coordinates and check relevant chunks
        int minChunkX = floor(playerBox.min.x / Chunk::CHUNK_SIZE);
        int maxChunkX = floor(playerBox.max.x / Chunk::CHUNK_SIZE);
        int minChunkZ = floor(playerBox.min.z / Chunk::CHUNK_SIZE);
        int maxChunkZ = floor(playerBox.max.z / Chunk::CHUNK_SIZE);

        for (int chunkX = minChunkX; chunkX <= maxChunkX; chunkX++) {
            for (int chunkZ = minChunkZ; chunkZ <= maxChunkZ; chunkZ++) {
                glm::ivec2 chunkPos(chunkX, chunkZ);
                auto it = chunks.find(chunkPos);
                if (it != chunks.end()) {
                    // Check blocks in the chunk
                    const Chunk& chunk = it->second;
                    int startX = (chunkX * Chunk::CHUNK_SIZE);
                    int startZ = (chunkZ * Chunk::CHUNK_SIZE);

                    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
                        for (int y = 0; y < Chunk::CHUNK_SIZE; y++) {
                            for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
                                if (chunk.blocks[x][y][z].exists) {
                                    AABB blockBox(
                                        glm::vec3(startX + x, y, startZ + z),
                                        glm::vec3(startX + x + 1.0f, y + 1.0f, startZ + z + 1.0f)
                                    );
                                    if (playerBox.intersects(blockBox)) {
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
};