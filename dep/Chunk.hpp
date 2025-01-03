// Chunk.hpp
#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "BlockGeometry.hpp"

struct Chunk {
    static constexpr int SIZE = 16;
    glm::ivec2 position; // Position in chunk coordinates
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    size_t vertexCount = 0;

    Chunk(const glm::ivec2& pos) : position(pos) {}

    void generateGeometry() {
        auto [vertices, indices] = BlockGeometry::generateChunkGeometry(position.x * SIZE, position.y * SIZE);
        this->vertices = vertices;
        this->indices = indices;
        vertexCount = indices.size();
    }
};
