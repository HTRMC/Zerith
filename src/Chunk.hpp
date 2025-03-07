#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include "ModelLoader.hpp"
#include "Block.hpp"
#include "Vertex.hpp"

// Constants for chunk dimensions
constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 16;
constexpr int CHUNK_SIZE_Z = 16;
constexpr int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

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
    
    // Get mesh data
    const std::vector<Vertex>& getVertices() const { return meshVertices; }
    const std::vector<uint16_t>& getIndices() const { return meshIndices; }
    
    // Check if mesh is dirty and needs regeneration
    bool isMeshDirty() const { return meshDirty; }
    
    // Mark mesh as clean after regeneration
    void markMeshClean() { meshDirty = false; }

private:
    // Convert 3D coordinates to 1D index
    int coordsToIndex(int x, int y, int z) const;
    
    // Check if coordinates are in bounds
    bool isInBounds(int x, int y, int z) const;
    
    // Helper function to determine if a face should be rendered
    bool shouldRenderFace(int x, int y, int z, int dx, int dy, int dz, const BlockRegistry& registry) const;

    // Position of this chunk in chunk coordinates
    glm::ivec3 chunkPosition;
    
    // Block data - array of block IDs
    std::array<uint16_t, CHUNK_VOLUME> blocks;
    
    // Mesh data
    std::vector<Vertex> meshVertices;
    std::vector<uint16_t> meshIndices;
    bool meshDirty = true;
};