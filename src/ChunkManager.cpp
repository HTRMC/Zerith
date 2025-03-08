#include "ChunkManager.hpp"
#include "Chunk.hpp"
#include "TextureLoader.hpp"
#include <iostream>

ChunkManager::ChunkManager() {
    // Initialize block registry
    initializeBlockRegistry();
}

void ChunkManager::initializeBlockRegistry() {
    // Register block types with IDs
    blockRegistry.registerBlock(0, "air");         // Air (transparent)
    blockRegistry.registerBlock(1, "stone");       // Stone
    blockRegistry.registerBlock(2, "grass_block"); // Grass block
    blockRegistry.registerBlock(3, "dirt");        // Dirt
    blockRegistry.registerBlock(4, "cobblestone"); // Cobblestone
    blockRegistry.registerBlock(5, "glass");  // Torch
    
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
}

void ChunkManager::updateChunkMeshes(ModelLoader& modelLoader) {
    // Update meshes for all chunks that need it
    for (auto& chunk : chunks) {
        if (chunk->isMeshDirty()) {
            chunk->generateMesh(blockRegistry, modelLoader);
        }
    }
}

bool ChunkManager::getFirstChunkMeshData(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices) const {
    // If we have chunks, return the first one's mesh data
    if (!chunks.empty() && !chunks[0]->getVertices().empty()) {
        vertices = chunks[0]->getVertices();
        indices = chunks[0]->getIndices();
        return true;
    }
    return false;
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
    texturePaths.push_back("assets/minecraft/textures/block/dirt.png");

    // 4: Cobblestone
    texturePaths.push_back("assets/minecraft/textures/block/cobblestone.png");

    // 5: Oak Planks
    texturePaths.push_back("assets/minecraft/textures/block/green_stained_glass.png");

    // Create texture array from these textures
    VkDescriptorImageInfo textureArrayInfo = textureLoader.createTextureArray(texturePaths);

    std::cout << "Created texture array for " << texturePaths.size() << " block types" << std::endl;

    return textureArrayInfo;
}