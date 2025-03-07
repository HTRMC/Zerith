#include "Chunk.hpp"
#include <iostream>
#include <unordered_set>

#include "VulkanApp.hpp"

Chunk::Chunk(const glm::ivec3& position) : chunkPosition(position) {
    // Initialize all blocks to air (0)
    blocks.fill(0);
}

uint16_t Chunk::getBlockAt(int x, int y, int z) const {
    if (!isInBounds(x, y, z)) {
        return 0; // Air outside bounds
    }
    
    return blocks[coordsToIndex(x, y, z)];
}

void Chunk::setBlockAt(int x, int y, int z, uint16_t blockId) {
    if (!isInBounds(x, y, z)) {
        return;
    }
    
    blocks[coordsToIndex(x, y, z)] = blockId;
    meshDirty = true; // Mark for mesh regeneration
}

void Chunk::fill(uint16_t blockId) {
    blocks.fill(blockId);
    meshDirty = true;
}

void Chunk::generateTestPattern() {
    // Generate a test pattern with different blocks
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                // Test pattern:
                // - Stone below half height
                // - Dirt for a layer above that
                // - Grass on top of dirt
                // - Air above
                if (z < 4) {
                    setBlockAt(x, y, z, 1); // Stone
                } 
                else if (z < 7) {
                    setBlockAt(x, y, z, 3); // Dirt
                }
                else if (z == 7) {
                    setBlockAt(x, y, z, 2); // Grass block
                }
                else {
                    setBlockAt(x, y, z, 0); // Air
                }
                
                // Create some patterns
                if ((x + y) % 5 == 0 && z < 10) {
                    setBlockAt(x, y, z, 5); // Oak planks
                }
                
                // Small pyramid in the center
                int centerX = CHUNK_SIZE_X / 2;
                int centerY = CHUNK_SIZE_Y / 2;
                int distX = std::abs(x - centerX);
                int distY = std::abs(y - centerY);
                if (distX + distY < 5 && z >= 7 && z < 12 - (distX + distY)) {
                    setBlockAt(x, y, z, 4); // Cobblestone
                }
            }
        }
    }
    
    meshDirty = true;
}

int Chunk::coordsToIndex(int x, int y, int z) const {
    return (z * CHUNK_SIZE_Y * CHUNK_SIZE_X) + (y * CHUNK_SIZE_X) + x;
}

bool Chunk::isInBounds(int x, int y, int z) const {
    return x >= 0 && x < CHUNK_SIZE_X &&
           y >= 0 && y < CHUNK_SIZE_Y &&
           z >= 0 && z < CHUNK_SIZE_Z;
}

bool Chunk::shouldRenderFace(int x, int y, int z, int dx, int dy, int dz, const BlockRegistry& registry) const {
    // Get the current block ID
    uint16_t blockId = getBlockAt(x, y, z);
    
    // Check the adjacent block
    int nx = x + dx;
    int ny = y + dy;
    int nz = z + dz;
    
    // If coordinates are outside chunk, render the face
    // (later this would check adjacent chunks)
    if (!isInBounds(nx, ny, nz)) {
        return true;
    }
    
    // Get the adjacent block type
    uint16_t adjacentBlockId = getBlockAt(nx, ny, nz);
    
    // If the adjacent block is transparent, render the face
    if (registry.isBlockTransparent(adjacentBlockId)) {
        return true;
    }
    
    // Otherwise, don't render the face (it's hidden)
    return false;
}

void Chunk::generateMesh(const BlockRegistry& registry, ModelLoader& modelLoader) {
    // Clear previous mesh data
    meshVertices.clear();
    meshIndices.clear();
    
    // Load all necessary block models
    std::unordered_map<uint16_t, ModelData> blockModels;
    
    // First, identify all unique block types in the chunk and load their models
    std::unordered_set<uint16_t> uniqueBlocks;
    for (uint16_t blockId : blocks) {
        if (blockId != 0 && registry.isValidBlock(blockId)) { // Skip air blocks (0)
            uniqueBlocks.insert(blockId);
        }
    }
    
    // Load models for each unique block type
    for (uint16_t blockId : uniqueBlocks) {
        std::string modelPath = registry.getModelPath(blockId);
        auto modelOpt = modelLoader.loadModel(modelPath);
        
        if (modelOpt.has_value()) {
            blockModels[blockId] = modelOpt.value();
        } else {
            std::cerr << "Failed to load model for block " << blockId << " (" 
                      << registry.getBlockName(blockId) << ") at " << modelPath << std::endl;
        }
    }
    
    // For each block in the chunk
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                uint16_t blockId = getBlockAt(x, y, z);
                
                // Skip air blocks and invalid blocks
                if (blockId == 0 || !registry.isValidBlock(blockId)) {
                    continue;
                }
                
                // Skip blocks without models
                if (blockModels.find(blockId) == blockModels.end()) {
                    continue;
                }
                
                // Calculate block position in world space
                glm::vec3 blockPosition = glm::vec3(
                    chunkPosition.x * CHUNK_SIZE_X + x,
                    chunkPosition.y * CHUNK_SIZE_Y + y,
                    chunkPosition.z * CHUNK_SIZE_Z + z
                );
                
                // Check face visibility
                // The 6 directions correspond to: east, west, north, south, up, down
                const std::array<std::pair<glm::ivec3, std::string>, 6> directions = {{
                    {{1, 0, 0}, "east"},
                    {{-1, 0, 0}, "west"},
                    {{0, 1, 0}, "north"},
                    {{0, -1, 0}, "south"},
                    {{0, 0, 1}, "up"},
                    {{0, 0, -1}, "down"}
                }};
                
                // Get the model data for this block
                const ModelData& modelData = blockModels[blockId];
                
                // Get the base index for this block
                uint16_t baseIndex = static_cast<uint16_t>(meshVertices.size());
                
                // Copy all vertices from the model, transformed to the block position
                for (const Vertex& vertex : modelData.vertices) {
                    Vertex transformedVertex = vertex;
                    transformedVertex.pos += blockPosition;
                    meshVertices.push_back(transformedVertex);
                }
                
                // Copy all indices from the model, offset by the base index
                for (uint16_t index : modelData.indices) {
                    meshIndices.push_back(baseIndex + index);
                }
            }
        }
    }
    
    // Mark mesh as clean after generation
    std::cout << "Generated mesh with " << meshVertices.size() << " vertices and " 
              << meshIndices.size() << " indices" << std::endl;
    meshDirty = false;
}