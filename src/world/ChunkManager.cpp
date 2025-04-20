#include "ChunkManager.hpp"
#include "Chunk.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

#include "Blocks.hpp"
#include "Logger.hpp"
#include "core/VulkanApp.hpp"

ChunkManager::ChunkManager() {
    // Initialize block registry
    Blocks::registerAllBlocks(blockRegistry);

    // Initialize render data for each layer
    layerRenderData[BlockRenderLayer::LAYER_OPAQUE] = LayerRenderData{};
    layerRenderData[BlockRenderLayer::LAYER_CUTOUT] = LayerRenderData{};
    layerRenderData[BlockRenderLayer::LAYER_TRANSLUCENT] = LayerRenderData{};

    // Initialize player chunk position to an invalid position
    lastPlayerChunkPos = glm::ivec3(INT_MAX, INT_MAX, INT_MAX);
}

ChunkManager::~ChunkManager() {
    // Clear all loaded chunks
    chunks.clear();
}

void ChunkManager::setVulkanResources(VkDevice device, VkPhysicalDevice physicalDevice,
                                    VkCommandPool commandPool, VkQueue graphicsQueue) {
    this->device = device;
    this->physicalDevice = physicalDevice;
    this->commandPool = commandPool;
    this->graphicsQueue = graphicsQueue;
}

void ChunkManager::updateLoadedChunks(const glm::vec3& playerPosition) {
    // Convert player position to chunk coordinates
    glm::ivec3 playerChunkPos = worldToChunkPos(playerPosition);

    // If player hasn't moved to a new chunk, just process the queue
    if (playerChunkPos == lastPlayerChunkPos) {
        return;
    }

    // Update the last known player chunk position
    lastPlayerChunkPos = playerChunkPos;
    
    // Identify chunks to load and unload
    for (int x = playerChunkPos.x - chunkLoadRadius; x <= playerChunkPos.x + chunkLoadRadius; x++) {
        for (int y = playerChunkPos.y - chunkLoadRadius; y <= playerChunkPos.y + chunkLoadRadius; y++) {
            for (int z = playerChunkPos.z - chunkLoadRadius; z <= playerChunkPos.z + chunkLoadRadius; z++) {
                glm::ivec3 checkPos(x, y, z);

                // If chunk isn't loaded and isn't already queued, add it to load queue
                if (chunks.find(checkPos) == chunks.end() &&
                    queuedChunks.find(checkPos) == queuedChunks.end()) {

                    chunkLoadQueue.push({checkPos});  // No priority needed
                    queuedChunks.insert(checkPos);
                }
            }
        }
    }
    
    // Find chunks to unload (chunks that are too far from player)
    std::vector<glm::ivec3> chunksToUnload;
    for (const auto& [pos, chunk] : chunks) {
        // Check if chunk is outside the cubic loading area
        bool outsideLoadArea =
            pos.x < playerChunkPos.x - chunkLoadRadius - 2 ||
            pos.x > playerChunkPos.x + chunkLoadRadius + 2 ||
            pos.y < playerChunkPos.y - chunkLoadRadius - 2 ||
            pos.y > playerChunkPos.y + chunkLoadRadius + 2 ||
            pos.z < playerChunkPos.z - chunkLoadRadius - 2 ||
            pos.z > playerChunkPos.z + chunkLoadRadius + 2;

        if (outsideLoadArea) {
            chunksToUnload.push_back(pos);
        }
    }
    
    // Unload chunks that are too far
    for (const auto& pos : chunksToUnload) {
        unloadChunk(pos);
    }

    // Mark render data as dirty if any chunks were loaded or unloaded
    if (!chunksToUnload.empty() || !chunkLoadQueue.empty()) {
        for (auto& [layer, data] : layerRenderData) {
            data.dirty = true;
        }
    }
}

void ChunkManager::updateChunkMeshes(ModelLoader& modelLoader, TextureLoader& textureLoader) {
    // Process the chunk load queue
    processChunkQueue(modelLoader);

    // Generate meshes for chunks that need it
    generateChunkMeshes(modelLoader, textureLoader);
}

bool ChunkManager::getLayerMeshData(BlockRenderLayer layer, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) const {
    vertices.clear();
    indices.clear();

    // Collect mesh data from all chunks for the specified layer
    for (const auto& [pos, chunk] : chunks) {
        const auto& layerMesh = chunk->getRenderLayerMesh(layer);

        if (layerMesh.vertices.empty() || layerMesh.indices.empty()) {
            continue; // Skip empty meshes
        }

        // Get base index for this chunk's vertices
        uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

        // Add vertices
        vertices.insert(vertices.end(), layerMesh.vertices.begin(), layerMesh.vertices.end());

        // Add indices (adjusted by base index)
        for (uint32_t index : layerMesh.indices) {
            indices.push_back(baseIndex + index);
        }
    }

    // Set the render layer value in each vertex
    int renderLayerValue = static_cast<int>(layer);
    for (Vertex& vertex : vertices) {
        vertex.renderLayer = renderLayerValue;
    }

    return !vertices.empty() && !indices.empty();
}

const LayerRenderData& ChunkManager::getLayerRenderData(BlockRenderLayer layer) const {
    auto it = layerRenderData.find(layer);
    if (it != layerRenderData.end()) {
        return it->second;
    }

    // Should never happen if properly initialized
    static LayerRenderData emptyData;
    return emptyData;
}

void ChunkManager::markLayerDirty(BlockRenderLayer layer) {
    auto it = layerRenderData.find(layer);
    if (it != layerRenderData.end()) {
        it->second.dirty = true;
    }
}

bool ChunkManager::isLayerDirty(BlockRenderLayer layer) const {
    auto it = layerRenderData.find(layer);
    if (it != layerRenderData.end()) {
        return it->second.dirty;
    }
    return false;
}

void ChunkManager::createLayerBuffers(BlockRenderLayer layer, VkDevice device, VkPhysicalDevice physicalDevice,
                                     VkCommandPool commandPool, VkQueue graphicsQueue) {
    auto& data = layerRenderData[layer];

    // Make sure device is idle before destroying old buffers
    vkDeviceWaitIdle(device);

    // Clean up previous buffers if they exist
    if (data.vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, data.vertexBuffer, nullptr);
        data.vertexBuffer = VK_NULL_HANDLE;
    }

    if (data.vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, data.vertexBufferMemory, nullptr);
        data.vertexBufferMemory = VK_NULL_HANDLE;
    }

    if (data.indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, data.indexBuffer, nullptr);
        data.indexBuffer = VK_NULL_HANDLE;
    }

    if (data.indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, data.indexBufferMemory, nullptr);
        data.indexBufferMemory = VK_NULL_HANDLE;
    }

    // Collect the mesh data for this layer
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    if (!getLayerMeshData(layer, vertices, indices)) {
        // No mesh data for this layer
        data.dirty = false;
        return;
    }

    // Store the mesh data
    data.vertices = vertices;
    data.indices = indices;

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();

    // Create a staging buffer for vertices
    VkBuffer vertexStagingBuffer;
    VkDeviceMemory vertexStagingBufferMemory;

    // Create staging buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = vertexBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexStagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex staging buffer!");
    }

    // Get memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, vertexStagingBuffer, &memRequirements);

    // Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;

    // Find suitable memory type
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32_t memoryTypeIndex = 0;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexStagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
        throw std::runtime_error("Failed to allocate vertex staging buffer memory!");
    }

    vkBindBufferMemory(device, vertexStagingBuffer, vertexStagingBufferMemory, 0);

    // Copy vertex data to the staging buffer
    void *vertexData;
    vkMapMemory(device, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &vertexData);
    memcpy(vertexData, vertices.data(), (size_t)vertexBufferSize);
    vkUnmapMemory(device, vertexStagingBufferMemory);

    // Create the final vertex buffer
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &data.vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    vkGetBufferMemoryRequirements(device, data.vertexBuffer, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(device, &allocInfo, nullptr, &data.vertexBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, data.vertexBuffer, nullptr);
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, data.vertexBuffer, data.vertexBufferMemory, 0);

    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();

    // Create staging buffer for indices
    bufferInfo.size = indexBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkBuffer indexStagingBuffer;
    VkDeviceMemory indexStagingBufferMemory;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &indexStagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create index staging buffer!");
    }

    vkGetBufferMemoryRequirements(device, indexStagingBuffer, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex; // Reuse the same memory type as before

    if (vkAllocateMemory(device, &allocInfo, nullptr, &indexStagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, indexStagingBuffer, nullptr);
        throw std::runtime_error("Failed to allocate index staging buffer memory!");
    }

    vkBindBufferMemory(device, indexStagingBuffer, indexStagingBufferMemory, 0);

    // Copy index data to the staging buffer
    void *indexData;
    vkMapMemory(device, indexStagingBufferMemory, 0, indexBufferSize, 0, &indexData);
    memcpy(indexData, indices.data(), (size_t)indexBufferSize);
    vkUnmapMemory(device, indexStagingBufferMemory);

    // Create the final index buffer
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &data.indexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create index buffer!");
    }

    vkGetBufferMemoryRequirements(device, data.indexBuffer, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &data.indexBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, data.indexBuffer, nullptr);
        throw std::runtime_error("Failed to allocate index buffer memory!");
    }

    vkBindBufferMemory(device, data.indexBuffer, data.indexBufferMemory, 0);

    // Copy data from staging buffers to final buffers
    VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandPool = commandPool;
    cmdBufferAllocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Copy vertex buffer
    VkBufferCopy vertexCopyRegion{};
    vertexCopyRegion.srcOffset = 0;
    vertexCopyRegion.dstOffset = 0;
    vertexCopyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(commandBuffer, vertexStagingBuffer, data.vertexBuffer, 1, &vertexCopyRegion);

    // Copy index buffer
    VkBufferCopy indexCopyRegion{};
    indexCopyRegion.srcOffset = 0;
    indexCopyRegion.dstOffset = 0;
    indexCopyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(commandBuffer, indexStagingBuffer, data.indexBuffer, 1, &indexCopyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    // Clean up staging buffers
    vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
    vkFreeMemory(device, vertexStagingBufferMemory, nullptr);
    vkDestroyBuffer(device, indexStagingBuffer, nullptr);
    vkFreeMemory(device, indexStagingBufferMemory, nullptr);

    // Mark layer as clean
    data.dirty = false;

    LOG_DEBUG("Created buffers for render layer %d with %zu vertices and %zu indices",
             static_cast<int>(layer), vertices.size(), indices.size());
}

void ChunkManager::cleanupLayerBuffers(VkDevice device) {
    for (auto& [layer, data] : layerRenderData) {
        if (data.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, data.vertexBuffer, nullptr);
            data.vertexBuffer = VK_NULL_HANDLE;
        }

        if (data.vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, data.vertexBufferMemory, nullptr);
            data.vertexBufferMemory = VK_NULL_HANDLE;
        }

        if (data.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, data.indexBuffer, nullptr);
            data.indexBuffer = VK_NULL_HANDLE;
        }

        if (data.indexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, data.indexBufferMemory, nullptr);
            data.indexBufferMemory = VK_NULL_HANDLE;
        }
    }
}

VkDescriptorImageInfo ChunkManager::loadChunkTextures(TextureLoader& textureLoader) const {
    // Create a vector to hold the texture paths for each block type
    std::vector<std::string> texturePaths;

    // Add textures for each block type in order of block ID
    // This way the texture index corresponds to the block ID
    // We'll skip ID 0 (air) since it's transparent

    // 1: Stone
    texturePaths.emplace_back("assets/minecraft/textures/block/stone.png");

    // 2: Grass Block
    texturePaths.emplace_back("assets/minecraft/textures/block/grass_block_top.png");

    // 3: Dirt
    texturePaths.emplace_back("assets/minecraft/textures/block/oak_planks.png");

    // 4: Cobblestone
    texturePaths.emplace_back("assets/minecraft/textures/block/cobblestone.png");

    // 5: Glass (translucent)
    texturePaths.emplace_back("assets/minecraft/textures/block/green_stained_glass.png");

    // 6: Oak Log
    texturePaths.emplace_back("assets/minecraft/textures/block/oak_log.png");

    // Create texture array from these textures
    VkDescriptorImageInfo textureArrayInfo = textureLoader.createTextureArray(texturePaths);

    LOG_INFO("Created texture array for %zu block types", texturePaths.size());

    return textureArrayInfo;
}

void ChunkManager::loadChunk(const glm::ivec3& position) {
    // Check if chunk already exists
    if (chunks.find(position) != chunks.end()) {
        return;
    }

    // Create new chunk
    auto chunk = std::make_unique<Chunk>(position);

    // Initialize chunk data - generate terrain
    chunk->generateTestPattern();

    // Insert into chunks map
    chunks[position] = std::move(chunk);

    // Mark all render layers as dirty
    for (auto& [layer, data] : layerRenderData) {
        data.dirty = true;
    }

    LOG_DEBUG("Loaded chunk at position (%d, %d, %d)", position.x, position.y, position.z);
}

void ChunkManager::unloadChunk(const glm::ivec3& position) {
    // Remove chunk from map
    auto it = chunks.find(position);
    if (it != chunks.end()) {
        chunks.erase(it);

        // Also remove from queuedChunks set if it's there - THIS LINE IS MISSING
        queuedChunks.erase(position);

        // Mark all render layers as dirty
        for (auto& [layer, data] : layerRenderData) {
            data.dirty = true;
        }

        LOG_DEBUG("Unloaded chunk at position (%d, %d, %d)", position.x, position.y, position.z);
    }
}

bool ChunkManager::isChunkInRange(const glm::ivec3& chunkPos, const glm::ivec3& playerChunkPos, int radius) const {
    // Calculate squared Euclidean distance
    int dx = chunkPos.x - playerChunkPos.x;
    int dy = chunkPos.y - playerChunkPos.y;
    int dz = chunkPos.z - playerChunkPos.z;

    int distanceSquared = dx*dx + dy*dy + dz*dz;

    // Compare to radius squared for efficiency (avoid square root)
    return distanceSquared <= radius*radius;
}

void ChunkManager::processChunkQueue(ModelLoader& modelLoader) {
    // Process a limited number of chunks per frame
    int processedCount = 0;

    while (!chunkLoadQueue.empty() && processedCount < maxChunksPerFrame) {
        // Get highest priority chunk
        ChunkLoadRequest request = chunkLoadQueue.front();
        chunkLoadQueue.pop();

        // Remove from queued set
        queuedChunks.erase(request.position);

        // Load the chunk
        loadChunk(request.position);

        processedCount++;
    }
}

void ChunkManager::generateChunkMeshes(ModelLoader& modelLoader, TextureLoader& textureLoader) {
    bool anyChunkUpdated = false;
    size_t meshGenerationCount = 0;

    // Update meshes for all chunks that need it
    for (auto& [pos, chunk] : chunks) {
        if (chunk->isAnyMeshDirty()) {
            chunk->generateMesh(blockRegistry, modelLoader, textureLoader);
            anyChunkUpdated = true;
            meshGenerationCount++;
        }
    }

    // If any chunk was updated, mark all layer render data as dirty
    if (anyChunkUpdated) {
        for (auto& [layer, data] : layerRenderData) {
            data.dirty = true;
        }

        // Log performance stats
        if (meshGenerationCount > 0) {
            LOG_DEBUG("Generated meshes for %zu chunks. Model cache: %zu models, hits: %zu, misses: %zu",
                     meshGenerationCount, modelLoader.getCacheSize(), modelLoader.getCacheHits(), modelLoader.getCacheMisses());
        }
    }
}

void ChunkManager::preloadBlockModels(ModelLoader& modelLoader) {
    LOG_INFO("Preloading block models...");

    // Preload models for registered blocks
    std::vector<std::string> blockModels = {
        "assets/minecraft/models/block/stone.json",
        "assets/minecraft/models/block/grass_block.json",
        "assets/minecraft/models/block/oak_fence_post.json",
        "assets/minecraft/models/block/cobblestone.json",
        "assets/minecraft/models/block/green_stained_glass.json",
        "assets/minecraft/models/block/oak_log.json"
    };

    // Keep track of loaded models to pass to texture loader
    std::vector<ModelData> loadedModels;

    for (const auto& modelPath : blockModels) {
        auto modelOpt = modelLoader.loadModel(modelPath);
        if (modelOpt.has_value()) {
            loadedModels.push_back(modelOpt.value());
        }
    }

    // Cache stats after preloading
    LOG_INFO("Preloaded %zu block models", loadedModels.size());
}

Chunk* ChunkManager::getChunk(const glm::ivec3& position) {
    auto it = chunks.find(position);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

glm::ivec3 ChunkManager::worldToChunkPos(const glm::vec3& worldPos) {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE_X)),
        static_cast<int>(std::floor(worldPos.y / CHUNK_SIZE_Y)),
        static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE_Z))
    );
}

glm::ivec3 ChunkManager::worldToLocalPos(const glm::vec3& worldPos) {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x)) % CHUNK_SIZE_X,
        static_cast<int>(std::floor(worldPos.y)) % CHUNK_SIZE_Y,
        static_cast<int>(std::floor(worldPos.z)) % CHUNK_SIZE_Z
    );
}

uint16_t ChunkManager::getBlockAt(const glm::vec3& worldPos) const {
    // Convert world position to chunk position
    glm::ivec3 chunkPos = worldToChunkPos(worldPos);

    // Convert world position to local position within the chunk
    glm::ivec3 localPos = worldToLocalPos(worldPos);

    // Make sure local coordinates are non-negative
    if (localPos.x < 0) localPos.x += CHUNK_SIZE_X;
    if (localPos.y < 0) localPos.y += CHUNK_SIZE_Y;
    if (localPos.z < 0) localPos.z += CHUNK_SIZE_Z;

    // Find the chunk
    auto it = chunks.find(chunkPos);
    if (it != chunks.end()) {
        // Get block from chunk
        return it->second->getBlockAt(localPos.x, localPos.y, localPos.z);
    }

    // If chunk isn't loaded, return air
    return 0;
}

void ChunkManager::setBlockAt(const glm::vec3& worldPos, uint16_t blockId) {
    // Convert world position to chunk position
    glm::ivec3 chunkPos = worldToChunkPos(worldPos);

    // Convert world position to local position within the chunk
    glm::ivec3 localPos = worldToLocalPos(worldPos);

    // Make sure local coordinates are non-negative
    if (localPos.x < 0) localPos.x += CHUNK_SIZE_X;
    if (localPos.y < 0) localPos.y += CHUNK_SIZE_Y;
    if (localPos.z < 0) localPos.z += CHUNK_SIZE_Z;

    // Find the chunk
    auto it = chunks.find(chunkPos);
    if (it != chunks.end()) {
        // Set block in chunk
        it->second->setBlockAt(localPos.x, localPos.y, localPos.z, blockId);

        // Mark layer render data as dirty
        for (auto& [layer, data] : layerRenderData) {
            data.dirty = true;
        }
    } else {
        // Chunk isn't loaded, we could queue it for loading or ignore
        // For now, we'll just ignore blocks set outside loaded chunks
    }
}