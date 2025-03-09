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
    struct TranslucentBlock {
        uint16_t blockId;
        glm::vec3 position;
        ModelData* model;
        std::unordered_map<std::string, bool> visibleFaces;
    };
    std::vector<TranslucentBlock> translucentBlocks;

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

                // Determine which faces are visible
                std::unordered_map<std::string, bool> visibleFaces;
                visibleFaces["north"] = shouldRenderFace(x, y, z, "north", registry);
                visibleFaces["south"] = shouldRenderFace(x, y, z, "south", registry);
                visibleFaces["east"] = shouldRenderFace(x, y, z, "east", registry);
                visibleFaces["west"] = shouldRenderFace(x, y, z, "west", registry);
                visibleFaces["up"] = shouldRenderFace(x, y, z, "up", registry);
                visibleFaces["down"] = shouldRenderFace(x, y, z, "down", registry);

                // Get the render layer for this block
                BlockRenderLayer renderLayer = registry.getBlockRenderLayer(blockId);

                // For LAYER_TRANSLUCENT blocks, store them for later processing
                if (renderLayer == BlockRenderLayer::LAYER_TRANSLUCENT) {
                    glm::vec3 blockPosition = glm::vec3(
                        chunkPosition.x * CHUNK_SIZE_X + x,
                        chunkPosition.y * CHUNK_SIZE_Y + y,
                        chunkPosition.z * CHUNK_SIZE_Z + z
                    );
                    translucentBlocks.push_back({blockId, blockPosition, &blockModels[blockId], visibleFaces});
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
                
                // Process each element in the model
                for (const Element& element : modelData.elements) {
                    // Process each face in the element - only if it should be rendered
                    for (const auto& [faceName, face] : element.faces) {
                        // Skip faces that should be culled
                        if (!visibleFaces[faceName]) {
                            continue;
                        }

                        // Get the base index for this face
                        uint16_t baseIndex = static_cast<uint16_t>(layerMesh.vertices.size());

                        // Use color from model or a default based on face
                        glm::vec3 faceColor = parseColor(element.color);

                        // Get UVs from face or use default
                        std::vector<glm::vec2> uvs = face.uvs.size() == 4 ? face.uvs : getDefaultUVs(faceName);

                        // Vertices for the face (simplified for clarity)
                        std::vector<Vertex> faceVertices = createFaceVertices(
                            element, faceName, faceColor, uvs, blockPosition, blockId, renderLayer
                        );

                        // Add vertices to mesh
                        for (const Vertex& vertex : faceVertices) {
                            layerMesh.vertices.push_back(vertex);
                        }

                        // Add indices for the face (typically 6 indices for a quad: two triangles)
                        std::vector<uint16_t> faceIndices = createFaceIndices(baseIndex);
                        for (uint16_t index : faceIndices) {
                            layerMesh.indices.push_back(index);
                        }
                    }
                }
            }
        }
    }

    // Now process LAYER_TRANSLUCENT blocks from back to front
    if (!translucentBlocks.empty()) {
        // Sort LAYER_TRANSLUCENT blocks back-to-front
        std::sort(translucentBlocks.begin(), translucentBlocks.end(),
            [](const TranslucentBlock& a, const TranslucentBlock& b) {
                return a.position.z > b.position.z;
            });

        // Now add the sorted LAYER_TRANSLUCENT blocks to the LAYER_TRANSLUCENT mesh
        RenderLayerMesh& translucentMesh = layerMeshes[BlockRenderLayer::LAYER_TRANSLUCENT];

        for (const TranslucentBlock& block : translucentBlocks) {
            // Process each element in the model
            for (const Element& element : block.model->elements) {
                // Process each face in the element - only if it should be rendered
                for (const auto& [faceName, face] : element.faces) {
                    // Skip faces that should be culled
                    if (!block.visibleFaces.at(faceName)) {
                        continue;
                    }

                    // Get the base index for this face
                    uint16_t baseIndex = static_cast<uint16_t>(translucentMesh.vertices.size());

                    // Use color from model or a default based on face
                    glm::vec3 faceColor = parseColor(element.color);

                    // Get UVs from face or use default
                    std::vector<glm::vec2> uvs = face.uvs.size() == 4 ? face.uvs : getDefaultUVs(faceName);

                    // Vertices for the face
                    std::vector<Vertex> faceVertices = createFaceVertices(
                        element, faceName, faceColor, uvs, block.position, block.blockId,
                        BlockRenderLayer::LAYER_TRANSLUCENT
                    );

                    // Add vertices to mesh
                    for (const Vertex& vertex : faceVertices) {
                        translucentMesh.vertices.push_back(vertex);
                    }

                    // Add indices for the face
                    std::vector<uint16_t> faceIndices = createFaceIndices(baseIndex);
                    for (uint16_t index : faceIndices) {
                        translucentMesh.indices.push_back(index);
                    }
                }
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

bool Chunk::shouldRenderFace(int x, int y, int z, const std::string& face, const BlockRegistry& registry) const {
    // Get the block at the current position
    uint16_t blockId = getBlockAt(x, y, z);

    // If the block is air (0), we don't render any faces
    if (blockId == 0) {
        return false;
    }

    // Check adjacent block based on face direction
    int dx = 0, dy = 0, dz = 0;

    if (face == "north") {
        dy = -1;
    } else if (face == "south") {
        dy = 1;
    } else if (face == "east") {
        dx = 1;
    } else if (face == "west") {
        dx = -1;
    } else if (face == "up") {
        dz = 1;
    } else if (face == "down") {
        dz = -1;
    }

    // Get the adjacent block
    uint16_t adjacentBlockId = getBlockAt(x + dx, y + dy, z + dz);

    // If the adjacent block is air, always render the face
    if (adjacentBlockId == 0) {
        return true;
    }

    // Get render layers for both blocks
    BlockRenderLayer currentBlockLayer = registry.getBlockRenderLayer(blockId);
    BlockRenderLayer adjacentBlockLayer = registry.getBlockRenderLayer(adjacentBlockId);

    // ONLY cull faces between two opaque blocks
    // All other combinations (opaque-cutout, opaque-translucent, cutout-cutout,
    // cutout-translucent, translucent-translucent) should be rendered
    if (currentBlockLayer == BlockRenderLayer::LAYER_OPAQUE &&
        adjacentBlockLayer == BlockRenderLayer::LAYER_OPAQUE) {
        return false; // Cull the face between two opaque blocks
        }

    // Render the face in all other cases
    return true;
}

std::vector<Vertex> Chunk::createFaceVertices(
    const Element& element,
    const std::string& faceName,
    const glm::vec3& color,
    const std::vector<glm::vec2>& uvs,
    const glm::vec3& position,
    uint16_t blockId,
    BlockRenderLayer renderLayer) {

    std::vector<Vertex> vertices;

    // Get the min and max coordinates of the element
    float x_min = element.from.x;
    float x_max = element.to.x;
    float y_min = element.from.y;
    float y_max = element.to.y;
    float z_min = element.from.z;
    float z_max = element.to.z;

    // Create vertices based on the face
    // IMPORTANT: Ensure counter-clockwise winding when looking at the face from outside the block
    if (faceName == "north") {
        // North face (negative Y) - when looking at it from outside (north side)
        vertices.push_back({{x_max + position.x, y_min + position.y, z_min + position.z}, color, uvs[0], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_min + position.x, y_min + position.y, z_min + position.z}, color, uvs[1], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_min + position.x, y_min + position.y, z_max + position.z}, color, uvs[2], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_min + position.y, z_max + position.z}, color, uvs[3], blockId - 1, static_cast<int>(renderLayer)});
    }
    else if (faceName == "south") {
        // South face (positive Y) - when looking at it from outside (south side)
        vertices.push_back({{x_min + position.x, y_max + position.y, z_min + position.z}, color, uvs[0], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_max + position.y, z_min + position.z}, color, uvs[1], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_max + position.y, z_max + position.z}, color, uvs[2], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_min + position.x, y_max + position.y, z_max + position.z}, color, uvs[3], blockId - 1, static_cast<int>(renderLayer)});
    }
    else if (faceName == "east") {
        // East face (positive X) - when looking at it from outside (east side)
        vertices.push_back({{x_max + position.x, y_max + position.y, z_min + position.z}, color, uvs[0], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_min + position.y, z_min + position.z}, color, uvs[1], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_min + position.y, z_max + position.z}, color, uvs[2], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_max + position.y, z_max + position.z}, color, uvs[3], blockId - 1, static_cast<int>(renderLayer)});
    }
    else if (faceName == "west") {
        // West face (negative X) - when looking at it from outside (west side)
        vertices.push_back({{x_min + position.x, y_min + position.y, z_min + position.z}, color, uvs[0], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_min + position.x, y_max + position.y, z_min + position.z}, color, uvs[1], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_min + position.x, y_max + position.y, z_max + position.z}, color, uvs[2], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_min + position.x, y_min + position.y, z_max + position.z}, color, uvs[3], blockId - 1, static_cast<int>(renderLayer)});
    }
    else if (faceName == "up") {
        // Top face (positive Z) - when looking at it from above
        vertices.push_back({{x_min + position.x, y_max + position.y, z_max + position.z}, color, uvs[0], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_max + position.y, z_max + position.z}, color, uvs[1], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_min + position.y, z_max + position.z}, color, uvs[2], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_min + position.x, y_min + position.y, z_max + position.z}, color, uvs[3], blockId - 1, static_cast<int>(renderLayer)});
    }
    else if (faceName == "down") {
        // Bottom face (negative Z) - when looking at it from below
        vertices.push_back({{x_min + position.x, y_min + position.y, z_min + position.z}, color, uvs[0], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_min + position.y, z_min + position.z}, color, uvs[1], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_max + position.x, y_max + position.y, z_min + position.z}, color, uvs[2], blockId - 1, static_cast<int>(renderLayer)});
        vertices.push_back({{x_min + position.x, y_max + position.y, z_min + position.z}, color, uvs[3], blockId - 1, static_cast<int>(renderLayer)});
    }

    return vertices;
}

// Helper function to create indices for a face (assumes 4 vertices in clockwise order)
std::vector<uint16_t> Chunk::createFaceIndices(uint16_t baseIndex) {
    return {
        baseIndex, static_cast<unsigned short>(baseIndex + 1), static_cast<unsigned short>(baseIndex + 2),
        static_cast<unsigned short>(baseIndex + 2), static_cast<unsigned short>(baseIndex + 3), baseIndex
    };
}

// Helper function to get default UVs for a face
std::vector<glm::vec2> Chunk::getDefaultUVs(const std::string& faceName) {
    // Provide sensible default UVs for each face type
    // These are counter-clockwise coordinates: bottom-left, bottom-right, top-right, top-left
    return {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
    };
}

glm::vec3 Chunk::parseColor(int colorIndex) const {
    // Default BlockBench color palette (simplified)
    // Returns RGB values normalized to 0-1 range
    switch (colorIndex) {
        case 0: return {0.0f, 0.0f, 0.0f}; // Black
        case 1: return {0.0f, 0.0f, 1.0f}; // Blue
        case 2: return {0.0f, 1.0f, 0.0f}; // Green
        case 3: return {0.0f, 1.0f, 1.0f}; // Cyan
        case 4: return {1.0f, 0.0f, 0.0f}; // Red
        case 5: return {1.0f, 0.0f, 1.0f}; // Magenta
        case 6: return {1.0f, 1.0f, 0.0f}; // Yellow
        case 7: return {1.0f, 1.0f, 1.0f}; // White
        case 8: return {0.5f, 0.5f, 0.5f}; // Gray
        // Add more colors as needed
        default: return {1.0f, 1.0f, 1.0f}; // Default white
    }
}