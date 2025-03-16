#pragma once

#include <array>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "ModelLoader.hpp"
#include "Block.hpp"
#include "Vertex.hpp"

// Constants for chunk dimensions
constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 16;
constexpr int CHUNK_SIZE_Z = 16;
constexpr int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

// Structure to hold mesh data for a specific render layer
struct RenderLayerMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    bool dirty = true;
};

class Chunk {
public:
    Chunk(const glm::ivec3& position);
    ~Chunk() = default;

    // Get the block ID at the specified position
    uint16_t getBlockAt(int x, int y, int z) const;
    
    // Set the block ID at the specified position
    void setBlockAt(int x, int y, int z, uint16_t blockId);
    
    // Fill the chunk with the same block ID
    void fill(uint16_t blockId);
    
    // Generate a test pattern
    void generateTestPattern();
    
    // Generate mesh data for rendering
    void generateMesh(const BlockRegistry& registry, ModelLoader& modelLoader);

    // Get chunk position in world coordinates
    glm::ivec3 getPosition() const { return chunkPosition; }
    
    // Get mesh data for a specific render layer
    const RenderLayerMesh& getRenderLayerMesh(BlockRenderLayer layer) const;
    
    // Check if mesh for a specific layer is dirty and needs regeneration
    bool isMeshDirty(BlockRenderLayer layer) const;
    
    // Mark mesh as clean after regeneration
    void markMeshClean(BlockRenderLayer layer);

    // Check if any mesh layer is dirty
    bool isAnyMeshDirty() const;

private:
    // Convert 3D coordinates to 1D index
    int coordsToIndex(int x, int y, int z) const;
    
    // Check if coordinates are in bounds
    bool isInBounds(int x, int y, int z) const;
    
    // Check if a block has any visible faces and should be included in the mesh
    bool isBlockVisible(int x, int y, int z, const BlockRegistry& registry) const;

    // Check if a specific face is occluded by an adjacent block
    bool isFaceOccluded(uint16_t blockId, uint16_t adjacentBlockId, const BlockRegistry& registry) const;

    // Position of this chunk in chunk coordinates
    glm::ivec3 chunkPosition;
    
    // Block data - array of block IDs
    std::array<uint16_t, CHUNK_VOLUME> blocks;
    
    // Mesh data for each render layer
    std::map<BlockRenderLayer, RenderLayerMesh> layerMeshes;

    // Tracking visible blocks for optimization
    std::vector<bool> visibleBlocks;

    // Check if a face should be rendered
    bool shouldRenderFace(int x, int y, int z, const std::string& face, const BlockRegistry& registry) const;

    // Helper function to create vertices for a face
    std::vector<Vertex> createFaceVertices(
        const Element& element,
        const std::string& faceName,
        const glm::vec3& color,
        const std::vector<glm::vec2>& uvs,
        const glm::vec3& position,
        uint16_t blockId,
        BlockRenderLayer renderLayer);

    // Helper function to create indices for a face
    std::vector<uint32_t> createFaceIndices(uint32_t baseIndex);

    // Helper function to get default UVs for a face
    std::vector<glm::vec2> getDefaultUVs(const std::string& faceName);

    glm::vec3 parseColor(int colorIndex) const;

    const Element &getBlockElement(uint16_t blockId, const std::string &face) const;
};