// World.h
#pragma once
#include <queue>
#include <set>

#include "Chunk.h"
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// AABB struct needs to be accessible to World class
struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB(const glm::vec3 &min, const glm::vec3 &max) : min(min), max(max) {
    }

    bool intersects(const AABB &other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

struct ChunkCoordHash {
    std::size_t operator()(const glm::ivec2 &k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
    }
};

struct ChunkCoordCompare {
    bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
        if (a.x != b.x) return a.x < b.x;
        return a.y < b.y;
    }
};

class World {
public:
    static const int RENDER_DISTANCE = 8; // Number of chunks to render in each direction
    std::unordered_map<glm::ivec2, Chunk, ChunkCoordHash> chunks;
    std::queue<glm::ivec2> chunkLoadQueue;
    std::set<glm::ivec2, ChunkCoordCompare> loadedChunks;

    World() {
        // Generate 5x5 chunks centered around (0,0)
        int spawnRadius = 2;
        for (int x = -spawnRadius; x <= spawnRadius; x++) {
            for (int z = -spawnRadius; z <= spawnRadius; z++) {
                loadChunk(glm::ivec2(x, z));
            }
        }
    }

    void update(const glm::vec3& playerPos) {
        // Convert player position to chunk coordinates
        int playerChunkX = floor(playerPos.x / Chunk::CHUNK_SIZE);
        int playerChunkZ = floor(playerPos.z / Chunk::CHUNK_SIZE);

        // Queue chunks to load
        for (int x = playerChunkX - RENDER_DISTANCE; x <= playerChunkX + RENDER_DISTANCE; x++) {
            for (int z = playerChunkZ - RENDER_DISTANCE; z <= playerChunkZ + RENDER_DISTANCE; z++) {
                glm::ivec2 chunkPos(x, z);
                if (shouldLoadChunk(chunkPos, glm::ivec2(playerChunkX, playerChunkZ))) {
                    queueChunkLoad(chunkPos);
                }
            }
        }

        // Process chunk loading queue (limit per frame)
        int chunksToLoadPerFrame = 2;
        while (!chunkLoadQueue.empty() && chunksToLoadPerFrame > 0) {
            glm::ivec2 chunkPos = chunkLoadQueue.front();
            chunkLoadQueue.pop();
            loadChunk(chunkPos);
            chunksToLoadPerFrame--;
        }

        // Unload distant chunks
        std::vector<glm::ivec2> chunksToUnload;
        for (const auto& chunk : chunks) {
            if (!shouldKeepChunk(chunk.first, glm::ivec2(playerChunkX, playerChunkZ))) {
                chunksToUnload.push_back(chunk.first);
            }
        }

        for (const auto& pos : chunksToUnload) {
            unloadChunk(pos);
        }
    }

    bool shouldLoadChunk(const glm::ivec2& chunkPos, const glm::ivec2& playerChunkPos) const {
        // Check if the chunk is within render distance and not already loaded
        float distance = glm::length(glm::vec2(chunkPos - playerChunkPos));
        return distance <= RENDER_DISTANCE && chunks.find(chunkPos) == chunks.end();
    }

    bool shouldKeepChunk(const glm::ivec2& chunkPos, const glm::ivec2& playerChunkPos) const {
        float distance = glm::length(glm::vec2(chunkPos - playerChunkPos));
        return distance <= RENDER_DISTANCE + 2; // Add small buffer to prevent pop-in
    }

    void queueChunkLoad(const glm::ivec2& pos) {
        if (loadedChunks.find(pos) == loadedChunks.end()) {
            chunkLoadQueue.push(pos);
            loadedChunks.insert(pos);
        }
    }

    void loadChunk(const glm::ivec2& pos) {
        if (chunks.find(pos) == chunks.end()) {
            chunks.emplace(pos, Chunk(pos.x, pos.y));
            chunks[pos].generateTerrain();

            // Update neighboring chunks' mesh
            updateNeighboringChunks(pos);
        }
    }

    void unloadChunk(const glm::ivec2& pos) {
        chunks.erase(pos);
        loadedChunks.erase(pos);

        // Update neighboring chunks' mesh
        updateNeighboringChunks(pos);
    }

    void updateNeighboringChunks(const glm::ivec2& pos) {
        // Update meshes of neighboring chunks
        const std::array<glm::ivec2, 4> neighbors = {
            glm::ivec2(pos.x + 1, pos.y),
            glm::ivec2(pos.x - 1, pos.y),
            glm::ivec2(pos.x, pos.y + 1),
            glm::ivec2(pos.x, pos.y - 1)
        };

        for (const auto& neighborPos : neighbors) {
            auto it = chunks.find(neighborPos);
            if (it != chunks.end()) {
                it->second.needsRemesh = true;
            }
        }
    }

    Block *getBlock(int x, int y, int z) {
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
            // Create a static Block to return
            static Block returnBlock;
            returnBlock = it->second.getBlock(localX, y, localZ);
            return &returnBlock;
        }
        return nullptr;
    }

    bool checkCollision(const AABB &playerBox) {
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
                    const Chunk &chunk = it->second;
                    int startX = (chunkX * Chunk::CHUNK_SIZE);
                    int startZ = (chunkZ * Chunk::CHUNK_SIZE);

                    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
                        for (int y = 0; y < Chunk::CHUNK_SIZE; y++) {
                            for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
                                Block block = chunk.getBlock(x, y, z);
                                if (block.exists) {
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
