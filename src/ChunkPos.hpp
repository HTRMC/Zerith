#pragma once
#include <cstdint>

// Structure to represent chunk coordinates
struct ChunkPos {
    int32_t x;
    int32_t y;
    int32_t z;

    bool operator==(const ChunkPos& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Hash function for ChunkPos
namespace std {
    template<>
    struct hash<ChunkPos> {
        size_t operator()(const ChunkPos& pos) const {
            // Simple hash combining function
            size_t h1 = std::hash<int32_t>{}(pos.x);
            size_t h2 = std::hash<int32_t>{}(pos.y);
            size_t h3 = std::hash<int32_t>{}(pos.z);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}