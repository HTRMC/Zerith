#pragma once

#include "chunk.h"
#include "blockbench_instance_generator.h"
#include "blockbench_instance_wrapper.h"
#include "texture_array.h"
#include "face_instance_pool.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace Zerith {

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

    // Load block models
    void loadBlockModels();
    
    // Get the texture array for loading textures
    const std::shared_ptr<TextureArray>& getTextureArray() const { return m_textureArray; }

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

    // Block model generators
    std::unordered_map<BlockType, std::unique_ptr<BlockbenchInstanceWrapper>> m_blockGenerators;
    
    // Texture array manager
    std::shared_ptr<TextureArray> m_textureArray;
    
    // Object pool for face instances
    std::unique_ptr<FaceInstancePool> m_faceInstancePool;
};

} // namespace Zerith