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
    blockRegistry.registerBlock(5, "oak_planks");  // Oak planks
    
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

uint32_t ChunkManager::loadChunkTextures(TextureLoader& textureLoader) const {
    // Get the texture loader's default texture ID as fallback
    uint32_t textureId = textureLoader.getDefaultTextureId();
    
    // For now, we'll just load a default texture for all blocks
    // In a more complete implementation, we'd load textures for each block type
    
    // Try loading textures in order of preference
    const std::array<std::string, 8> preferredTextures = {
        "all", "side", "bottom", "top", "north", "south", "east", "west"
    };
    
    // Try to find a texture for stone (block ID 1)
    std::string modelPath = blockRegistry.getModelPath(1);
    auto modelOpt = textureLoader.loadTexture("assets/minecraft/textures/block/stone.png");
    
    if (modelOpt != textureId) {
        std::cout << "Loaded texture for chunk blocks" << std::endl;
        return modelOpt;
    }
    
    // If that fails, try dirt
    modelOpt = textureLoader.loadTexture("assets/minecraft/textures/block/dirt.png");
    if (modelOpt != textureId) {
        std::cout << "Loaded dirt texture for chunk blocks" << std::endl;
        return modelOpt;
    }
    
    std::cout << "Using default texture for chunk" << std::endl;
    return textureId;
}