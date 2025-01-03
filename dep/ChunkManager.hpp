// ChunkManager.hpp
#pragma once
#include <unordered_map>
#include <memory>
#include <unordered_set>
#include <vulkan/vulkan_core.h>

#include "Chunk.hpp"

struct ChunkHasher {
    size_t operator()(const glm::ivec2& pos) const {
        return std::hash<int>()(pos.x) ^ (std::hash<int>()(pos.y) << 1);
    }
};

class ChunkManager {
public:
    static constexpr int RENDER_DISTANCE = 8; // Number of chunks to render in each direction

    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, ChunkHasher> chunks;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    ChunkManager(VkDevice device, VkPhysicalDevice physicalDevice,
                VkCommandPool commandPool, VkQueue graphicsQueue)
        : device(device), physicalDevice(physicalDevice),
          commandPool(commandPool), graphicsQueue(graphicsQueue) {}

    ~ChunkManager() {
        for (auto& pair : chunks) {
            if (pair.second->vertexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, pair.second->vertexBuffer, nullptr);
                vkDestroyBuffer(device, pair.second->indexBuffer, nullptr);
                vkFreeMemory(device, pair.second->vertexBufferMemory, nullptr);
                vkFreeMemory(device, pair.second->indexBufferMemory, nullptr);
            }
        }
    }

    void updateLoadedChunks(const glm::vec3& cameraPos) {
        // Convert camera position to chunk coordinates
        glm::ivec2 centerChunk(
            static_cast<int>(floor(cameraPos.x / Chunk::SIZE)),
            static_cast<int>(floor(cameraPos.y / Chunk::SIZE))
        );

        // Determine which chunks should be loaded
        std::unordered_set<glm::ivec2, ChunkHasher> neededChunks;
        for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
            for (int y = -RENDER_DISTANCE; y <= RENDER_DISTANCE; y++) {
                glm::ivec2 chunkPos = centerChunk + glm::ivec2(x, y);
                neededChunks.insert(chunkPos);

                // Load new chunks
                if (chunks.find(chunkPos) == chunks.end()) {
                    auto chunk = std::make_unique<Chunk>(chunkPos);
                    chunk->generateGeometry();
                    createVertexBuffer(chunk.get());
                    chunks[chunkPos] = std::move(chunk);
                }
            }
        }

        // Unload distant chunks
        for (auto it = chunks.begin(); it != chunks.end();) {
            if (neededChunks.find(it->first) == neededChunks.end()) {
                vkDestroyBuffer(device, it->second->vertexBuffer, nullptr);
                vkFreeMemory(device, it->second->vertexBufferMemory, nullptr);
                it = chunks.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    void createVertexBuffer(Chunk* chunk) {
        VkDeviceSize vertexBufferSize = sizeof(Vertex) * chunk->vertices.size();
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * chunk->indices.size();

        // Create staging buffers
        VkBuffer stagingVertexBuffer, stagingIndexBuffer;
        VkDeviceMemory stagingVertexMemory, stagingIndexMemory;

        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingVertexBuffer, stagingVertexMemory);

        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingIndexBuffer, stagingIndexMemory);

        // Copy vertex data
        void* data;
        vkMapMemory(device, stagingVertexMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, chunk->vertices.data(), vertexBufferSize);
        vkUnmapMemory(device, stagingVertexMemory);

        // Copy index data
        vkMapMemory(device, stagingIndexMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, chunk->indices.data(), indexBufferSize);
        vkUnmapMemory(device, stagingIndexMemory);

        // Create device local buffers
        createBuffer(vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            chunk->vertexBuffer, chunk->vertexBufferMemory);

        createBuffer(indexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            chunk->indexBuffer, chunk->indexBufferMemory);

        // Copy to device local memory
        copyBuffer(stagingVertexBuffer, chunk->vertexBuffer, vertexBufferSize);
        copyBuffer(stagingIndexBuffer, chunk->indexBuffer, indexBufferSize);

        // Cleanup staging buffers
        vkDestroyBuffer(device, stagingVertexBuffer, nullptr);
        vkDestroyBuffer(device, stagingIndexBuffer, nullptr);
        vkFreeMemory(device, stagingVertexMemory, nullptr);
        vkFreeMemory(device, stagingIndexMemory, nullptr);
    }

    // Include your existing buffer creation and copying methods here
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer& buffer,
                     VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};