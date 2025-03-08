#include "Chunk.hpp"
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include "VulkanApp.hpp"

Chunk::Chunk(const glm::ivec3& position) : chunkPosition(position) {
    // Initialize all blocks to air (0)
    blocks.fill(0);

    // Initialize render layer meshes
    layerMeshes[BlockRenderLayer::LAYER_OPAQUE] = RenderLayerMesh{};
    layerMeshes[BlockRenderLayer::LAYER_CUTOUT] = RenderLayerMesh{};
    layerMeshes[BlockRenderLayer::LAYER_TRANSLUCENT] = RenderLayerMesh{};
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

    // Mark all render layer meshes as dirty
    for (auto& [layer, mesh] : layerMeshes) {
        mesh.dirty = true;
    }
}

void Chunk::fill(uint16_t blockId) {
    blocks.fill(blockId);

    // Mark all render layer meshes as dirty
    for (auto& [layer, mesh] : layerMeshes) {
        mesh.dirty = true;
    }
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
                    setBlockAt(x, y, z, 5); // Glass
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
    
    // Mark all render layer meshes as dirty
    for (auto& [layer, mesh] : layerMeshes) {
        mesh.dirty = true;
    }
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
    
    // Special case for LAYER_TRANSLUCENT blocks - always render faces between different LAYER_TRANSLUCENT blocks
    if (registry.getBlockRenderLayer(blockId) == BlockRenderLayer::LAYER_TRANSLUCENT &&
        registry.getBlockRenderLayer(adjacentBlockId) == BlockRenderLayer::LAYER_TRANSLUCENT &&
        blockId != adjacentBlockId) {
        return true;
    }

    // Otherwise, don't render the face (it's hidden)
    return false;
}

const RenderLayerMesh& Chunk::getRenderLayerMesh(BlockRenderLayer layer) const {
    auto it = layerMeshes.find(layer);
    if (it != layerMeshes.end()) {
        return it->second;
    }

    // This should never happen if layers are properly initialized
    static RenderLayerMesh emptyMesh;
    return emptyMesh;
}

bool Chunk::isMeshDirty(BlockRenderLayer layer) const {
    auto it = layerMeshes.find(layer);
    if (it != layerMeshes.end()) {
        return it->second.dirty;
    }
    return false;
}

bool Chunk::isAnyMeshDirty() const {
    for (const auto& [layer, mesh] : layerMeshes) {
        if (mesh.dirty) {
            return true;
        }
    }
    return false;
}

void Chunk::markMeshClean(BlockRenderLayer layer) {
    auto it = layerMeshes.find(layer);
    if (it != layerMeshes.end()) {
        it->second.dirty = false;
    }
}

void Chunk::generateMesh(const BlockRegistry& registry, ModelLoader& modelLoader) {
    // Clear previous mesh data for all layers
    for (auto& [layer, mesh] : layerMeshes) {
        mesh.vertices.clear();
        mesh.indices.clear();
    }
    
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

    // For LAYER_TRANSLUCENT blocks, we'll create a list of blocks to sort by distance from camera
    struct LAYER_TRANSLUCENTBlock {
        uint16_t blockId;
        glm::vec3 position;
        ModelData* model;
    };
    std::vector<LAYER_TRANSLUCENTBlock> LAYER_TRANSLUCENTBlocks;

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

                // Get the render layer for this block
                BlockRenderLayer renderLayer = registry.getBlockRenderLayer(blockId);

                // For LAYER_TRANSLUCENT blocks, just store them for now - we'll process them later
                if (renderLayer == BlockRenderLayer::LAYER_TRANSLUCENT) {
                    glm::vec3 blockPosition = glm::vec3(
                        chunkPosition.x * CHUNK_SIZE_X + x,
                        chunkPosition.y * CHUNK_SIZE_Y + y,
                        chunkPosition.z * CHUNK_SIZE_Z + z
                    );
                    LAYER_TRANSLUCENTBlocks.push_back({blockId, blockPosition, &blockModels[blockId]});
                    continue;
                }

                // Get the appropriate mesh for this block's render layer
                RenderLayerMesh& layerMesh = layerMeshes[renderLayer];

                // Calculate block position in world space
                glm::vec3 blockPosition = glm::vec3(
                    chunkPosition.x * CHUNK_SIZE_X + x,
                    chunkPosition.y * CHUNK_SIZE_Y + y,
                    chunkPosition.z * CHUNK_SIZE_Z + z
                );

                // Get the model data for this block
                const ModelData& modelData = blockModels[blockId];
                
                // Get the base index for this block
                uint16_t baseIndex = static_cast<uint16_t>(layerMesh.vertices.size());
                
                // Copy all vertices from the model, transformed to the block position
                for (const Vertex& vertex : modelData.vertices) {
                    Vertex transformedVertex = vertex;
                    transformedVertex.pos += blockPosition;

                    // Set the texture index to match the block ID
                    // Since air is 0, and we don't include it in our texture array,
                    // we need to subtract 1 from blockId to get the correct texture index
                    transformedVertex.textureIndex = blockId - 1;

                    // Set the render layer value based on the block's render layer
                    transformedVertex.renderLayer = static_cast<int>(renderLayer);

                    layerMesh.vertices.push_back(transformedVertex);
                }
                
                // Copy all indices from the model, offset by the base index
                for (uint16_t index : modelData.indices) {
                    layerMesh.indices.push_back(baseIndex + index);
                }
            }
        }
    }

    // Now process LAYER_TRANSLUCENT blocks from back to front
    if (!LAYER_TRANSLUCENTBlocks.empty()) {
        // Sort LAYER_TRANSLUCENT blocks back-to-front (furthest first)
        // This is a simple example - in reality, you'd want to sort based on camera position
        // For now, we'll sort them by Z coordinate as a simple approximation
        std::sort(LAYER_TRANSLUCENTBlocks.begin(), LAYER_TRANSLUCENTBlocks.end(),
            [](const LAYER_TRANSLUCENTBlock& a, const LAYER_TRANSLUCENTBlock& b) {
                return a.position.z > b.position.z; // Higher Z coordinates (further away) come first
            });

        // Now add the sorted LAYER_TRANSLUCENT blocks to the LAYER_TRANSLUCENT mesh
        RenderLayerMesh& LAYER_TRANSLUCENTMesh = layerMeshes[BlockRenderLayer::LAYER_TRANSLUCENT];

        for (const LAYER_TRANSLUCENTBlock& block : LAYER_TRANSLUCENTBlocks) {
            // Get the base index for this block
            uint16_t baseIndex = static_cast<uint16_t>(LAYER_TRANSLUCENTMesh.vertices.size());

            // Copy all vertices from the model, transformed to the block position
            for (const Vertex& vertex : block.model->vertices) {
                Vertex transformedVertex = vertex;
                transformedVertex.pos += block.position;

                // Set the texture index
                transformedVertex.textureIndex = block.blockId - 1;

                LAYER_TRANSLUCENTMesh.vertices.push_back(transformedVertex);
            }

            // Copy all indices from the model, offset by the base index
            for (uint16_t index : block.model->indices) {
                LAYER_TRANSLUCENTMesh.indices.push_back(baseIndex + index);
            }
        }
    }

    // Mark all meshes as clean after generation
    std::cout << "Generated meshes: "
              << "LAYER_OPAQUE: " << layerMeshes[BlockRenderLayer::LAYER_OPAQUE].vertices.size() << " vertices, "
              << layerMeshes[BlockRenderLayer::LAYER_OPAQUE].indices.size() << " indices, "
              << "LAYER_CUTOUT: " << layerMeshes[BlockRenderLayer::LAYER_CUTOUT].vertices.size() << " vertices, "
              << layerMeshes[BlockRenderLayer::LAYER_CUTOUT].indices.size() << " indices, "
              << "LAYER_TRANSLUCENT: " << layerMeshes[BlockRenderLayer::LAYER_TRANSLUCENT].vertices.size() << " vertices, "
              << layerMeshes[BlockRenderLayer::LAYER_TRANSLUCENT].indices.size() << " indices" << std::endl;

    for (auto& [layer, mesh] : layerMeshes) {
        mesh.dirty = false;
    }
}