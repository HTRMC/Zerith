#pragma once
#include <unordered_map>
#include <vector>

#include "ChunkPos.hpp"
#include "ChunkStorage.hpp"

class ChunkManager {
public:
    static const int RENDER_DISTANCE = 8;  // Number of chunks in each direction

    ChunkManager() = default;

    // Get or create chunk at position
    ChunkStorage::BlockGrid& getChunk(const ChunkPos& pos) {
        auto it = chunks.find(pos);
        if (it == chunks.end()) {
            // Generate new chunk
            auto result = chunks.emplace(pos, ChunkStorage::generateChunk(pos));
            return result.first->second;
        }
        return it->second;
    }

    // Update chunk loading based on player position
    void updateChunks(const glm::vec3& playerPos) {
        // Convert player position to chunk coordinates
        ChunkPos centerChunk = {
            static_cast<int32_t>(std::floor(playerPos.x / ChunkStorage::CHUNK_SIZE)),
            static_cast<int32_t>(std::floor(playerPos.y / ChunkStorage::CHUNK_SIZE)),
            0  // For now, only handle 2D chunk loading
        };

        // Clear the list of chunks to render
        activeChunks.clear();

        // Load chunks in render distance
        for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
            for (int y = -RENDER_DISTANCE; y <= RENDER_DISTANCE; y++) {
                ChunkPos pos = {
                    centerChunk.x + x,
                    centerChunk.y + y,
                    0
                };

                // Check if chunk is within circular render distance
                float distance = std::sqrt(x*x + y*y);
                if (distance <= RENDER_DISTANCE) {
                    activeChunks.push_back(pos);
                    getChunk(pos);  // Ensure chunk is loaded
                }
            }
        }

        // TODO: Unload chunks outside render distance + buffer zone
    }

    // Get all visible faces for active chunks
    std::vector<uint32_t> getVisibleFaces() {
        std::vector<uint32_t> allFaces;

        for (const auto& pos : activeChunks) {
            auto& chunk = getChunk(pos);
            auto faces = ChunkStorage::generateVisibleFaces(chunk, pos);
            allFaces.insert(allFaces.end(), faces.begin(), faces.end());
        }

        return allFaces;
    }

private:
    std::unordered_map<ChunkPos, ChunkStorage::BlockGrid> chunks;
    std::vector<ChunkPos> activeChunks;
};