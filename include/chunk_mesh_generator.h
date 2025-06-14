#pragma once

#include "chunk.h"
#include "blockbench_instance_generator.h"
#include "blockbench_instance_wrapper.h"
#include "texture_array.h"
#include "face_instance_pool.h"
#include "binary_mesh_converter.h"
#include "block_registry.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <array>

namespace Zerith {

// Structure to hold faces separated by render layer
struct LayeredChunkMesh {
    std::array<std::vector<BlockbenchInstanceGenerator::FaceInstance>, 3> layers;
    
    std::vector<BlockbenchInstanceGenerator::FaceInstance>& getOpaqueFaces() { return layers[0]; }
    std::vector<BlockbenchInstanceGenerator::FaceInstance>& getCutoutFaces() { return layers[1]; }
    std::vector<BlockbenchInstanceGenerator::FaceInstance>& getTranslucentFaces() { return layers[2]; }
    
    const std::vector<BlockbenchInstanceGenerator::FaceInstance>& getOpaqueFaces() const { return layers[0]; }
    const std::vector<BlockbenchInstanceGenerator::FaceInstance>& getCutoutFaces() const { return layers[1]; }
    const std::vector<BlockbenchInstanceGenerator::FaceInstance>& getTranslucentFaces() const { return layers[2]; }
    
    std::vector<BlockbenchInstanceGenerator::FaceInstance>& getLayer(RenderLayer layer) {
        return layers[static_cast<int>(layer)];
    }
    
    const std::vector<BlockbenchInstanceGenerator::FaceInstance>& getLayer(RenderLayer layer) const {
        return layers[static_cast<int>(layer)];
    }
};

class ChunkMeshGenerator {
public:
    ChunkMeshGenerator();

    // Generate face instances for an entire chunk (legacy method)
    std::vector<BlockbenchInstanceGenerator::FaceInstance> generateChunkMesh(const Chunk& chunk);

    // Generate face instances for an entire chunk using object pooling
    FaceInstancePool::FaceInstanceBatch generateChunkMeshPooled(const Chunk& chunk);
    
    // Generate face instances with neighboring chunk data for proper culling
    std::vector<BlockbenchInstanceGenerator::FaceInstance> generateChunkMeshWithNeighbors(
        const Chunk& chunk, 
        const Chunk* neighborXMinus, const Chunk* neighborXPlus,
        const Chunk* neighborYMinus, const Chunk* neighborYPlus,
        const Chunk* neighborZMinus, const Chunk* neighborZPlus);
    
    // Generate face instances with neighboring chunk data using object pooling
    FaceInstancePool::FaceInstanceBatch generateChunkMeshPooledWithNeighbors(
        const Chunk& chunk,
        const Chunk* neighborXMinus, const Chunk* neighborXPlus,
        const Chunk* neighborYMinus, const Chunk* neighborYPlus,
        const Chunk* neighborZMinus, const Chunk* neighborZPlus);

    // Generate layered mesh with faces separated by render layer
    // LayeredChunkMesh generateLayeredChunkMesh(const Chunk& chunk);
    
    // Generate layered mesh with neighbor awareness
    // LayeredChunkMesh generateLayeredChunkMeshWithNeighbors(
    //     const Chunk& chunk,
    //     const Chunk* neighborXMinus, const Chunk* neighborXPlus,
    //     const Chunk* neighborYMinus, const Chunk* neighborYPlus,
    //     const Chunk* neighborZMinus, const Chunk* neighborZPlus);

    // Load block models
    void loadBlockModels();
    
    // Get the texture array for loading textures
    const std::shared_ptr<TextureArray>& getTextureArray() const { return m_textureArray; }
    
    // Enable/disable binary greedy meshing
    void setBinaryMeshingEnabled(bool enabled) { m_binaryMeshingEnabled = enabled; }
    bool isBinaryMeshingEnabled() const { return m_binaryMeshingEnabled; }

private:
    // Get the model path for a block type
    std::string getModelPath(BlockType type) const;
    
    // Generate faces for a single block (legacy method)
    void generateBlockFaces(const Chunk& chunk, int x, int y, int z, 
                           std::vector<BlockbenchInstanceGenerator::FaceInstance>& faces);
    
    // Generate faces for a single block using object pooling
    void generateBlockFacesPooled(const Chunk& chunk, int x, int y, int z, 
                                 FaceInstancePool::FaceInstanceBatch& batch);
    
    // Generate faces with neighbor awareness
    void generateBlockFacesWithNeighbors(const Chunk& chunk, int x, int y, int z,
                                        std::vector<BlockbenchInstanceGenerator::FaceInstance>& faces,
                                        const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                        const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                        const Chunk* neighborZMinus, const Chunk* neighborZPlus);
    
    // Generate faces with neighbor awareness using object pooling
    void generateBlockFacesPooledWithNeighbors(const Chunk& chunk, int x, int y, int z,
                                              FaceInstancePool::FaceInstanceBatch& batch,
                                              const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                              const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                              const Chunk* neighborZMinus, const Chunk* neighborZPlus);
    
    // Check if a face is visible considering neighboring chunks
    bool isFaceVisibleWithNeighbors(const Chunk& chunk, int x, int y, int z, 
                                   int dx, int dy, int dz,
                                   const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                   const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                   const Chunk* neighborZMinus, const Chunk* neighborZPlus);
    
    // Advanced version that considers block properties
    bool isFaceVisibleWithNeighborsAdvanced(const Chunk& chunk, int x, int y, int z, 
                                           int faceDir,
                                           const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                           const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                           const Chunk* neighborZMinus, const Chunk* neighborZPlus);

    // Generate faces for a single block with render layer separation
    // void generateBlockFacesLayered(const Chunk& chunk, int x, int y, int z, 
    //                               LayeredChunkMesh& layeredMesh);
    
    // Generate faces for a single block with neighbor awareness and render layer separation
    // void generateBlockFacesLayeredWithNeighbors(const Chunk& chunk, int x, int y, int z,
    //                                            LayeredChunkMesh& layeredMesh,
    //                                            const Chunk* neighborXMinus, const Chunk* neighborXPlus,
    //                                            const Chunk* neighborYMinus, const Chunk* neighborYPlus,
    //                                            const Chunk* neighborZMinus, const Chunk* neighborZPlus);

    // Block model generators
    std::unordered_map<BlockType, std::unique_ptr<BlockbenchInstanceWrapper>> m_blockGenerators;
    
    // Texture array manager
    std::shared_ptr<TextureArray> m_textureArray;
    
    // Object pool for face instances
    std::unique_ptr<FaceInstancePool> m_faceInstancePool;
    
    // Binary meshing flag
    bool m_binaryMeshingEnabled = true;
};

} // namespace Zerith