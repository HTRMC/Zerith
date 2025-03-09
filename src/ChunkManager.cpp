#include "ChunkManager.hpp"
#include "Chunk.hpp"
#include "TextureLoader.hpp"
#include <iostream>

ChunkManager::ChunkManager() {
    // Initialize block registry
    initializeBlockRegistry();

    // Initialize render data for each layer
    layerRenderData[BlockRenderLayer::LAYER_OPAQUE] = LayerRenderData{};
    layerRenderData[BlockRenderLayer::LAYER_CUTOUT] = LayerRenderData{};
    layerRenderData[BlockRenderLayer::LAYER_TRANSLUCENT] = LayerRenderData{};
}

void ChunkManager::initializeBlockRegistry() {
    // Register block types with IDs and appropriate render layers
    blockRegistry.registerBlock(0, "air", BlockRenderLayer::LAYER_CUTOUT);                      // Air (transparent)
    blockRegistry.registerBlock(1, "stone", BlockRenderLayer::LAYER_OPAQUE);                    // Stone
    blockRegistry.registerBlock(2, "grass_block", BlockRenderLayer::LAYER_OPAQUE);              // Grass block
    blockRegistry.registerBlock(3, "oak_fence_post", BlockRenderLayer::LAYER_CUTOUT);           // Oak Fence Post
    blockRegistry.registerBlock(4, "cobblestone", BlockRenderLayer::LAYER_OPAQUE);              // Cobblestone
    blockRegistry.registerBlock(5, "green_stained_glass", BlockRenderLayer::LAYER_TRANSLUCENT); // Green Stained Glass (translucent)
    
    std::cout << "Initialized block registry with 6 block types" << std::endl;
}

void ChunkManager::createChunks() {
    // Create a single chunk at (0,0,0) for now
    auto chunk = std::make_unique<Chunk>(glm::ivec3(0, 0, 0));
    
    // Generate a test pattern in the chunk
    chunk->generateTestPattern();
    
    // Add to chunks vector
    chunks.push_back(std::move(chunk));
    
    std::cout << "Created 1 chunk at position (0,0,0)" << std::endl;

    // Mark all layers as dirty
    for (auto& [layer, data] : layerRenderData) {
        data.dirty = true;
    }
}

void ChunkManager::updateChunkMeshes(ModelLoader& modelLoader) {
    bool anyChunkUpdated = false;

    // Update meshes for all chunks that need it
    for (auto& chunk : chunks) {
        if (chunk->isAnyMeshDirty()) {
            chunk->generateMesh(blockRegistry, modelLoader);
            anyChunkUpdated = true;
        }
    }

    // If any chunk was updated, mark all layer render data as dirty
    if (anyChunkUpdated) {
        for (auto& [layer, data] : layerRenderData) {
            data.dirty = true;
        }
    }
}

bool ChunkManager::getLayerMeshData(BlockRenderLayer layer, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices) const {
    vertices.clear();
    indices.clear();

    // Collect mesh data from all chunks for the specified layer
    for (const auto& chunk : chunks) {
        const auto& layerMesh = chunk->getRenderLayerMesh(layer);

        if (layerMesh.vertices.empty() || layerMesh.indices.empty()) {
            continue; // Skip empty meshes
        }

        // Get base index for this chunk's vertices
        uint16_t baseIndex = static_cast<uint16_t>(vertices.size());

        // Add vertices
        vertices.insert(vertices.end(), layerMesh.vertices.begin(), layerMesh.vertices.end());

        // Add indices (adjusted by base index)
        for (uint16_t index : layerMesh.indices) {
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

    // Collect the mesh data for this layer
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

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
    VkDeviceSize indexBufferSize = sizeof(uint16_t) * indices.size();

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

    std::cout << "Created buffers for render layer " << static_cast<int>(layer)
              << " with " << vertices.size() << " vertices and "
              << indices.size() << " indices" << std::endl;
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
    texturePaths.push_back("assets/minecraft/textures/block/stone.png");

    // 2: Grass Block (using top texture for simplicity)
    texturePaths.push_back("assets/minecraft/textures/block/grass_block_top.png");

    // 3: Dirt
    texturePaths.push_back("assets/minecraft/textures/block/oak_planks.png");

    // 4: Cobblestone
    texturePaths.push_back("assets/minecraft/textures/block/cobblestone.png");

    // 5: Glass (translucent)
    texturePaths.push_back("assets/minecraft/textures/block/green_stained_glass.png");

    // Create texture array from these textures
    VkDescriptorImageInfo textureArrayInfo = textureLoader.createTextureArray(texturePaths);

    std::cout << "Created texture array for " << texturePaths.size() << " block types" << std::endl;

    return textureArrayInfo;
}