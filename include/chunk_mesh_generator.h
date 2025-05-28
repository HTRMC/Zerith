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

    // Block model generators
    std::unordered_map<BlockType, std::unique_ptr<BlockbenchInstanceWrapper>> m_blockGenerators;
    
    // Texture array manager
    std::shared_ptr<TextureArray> m_textureArray;
    
    // Object pool for face instances
    std::unique_ptr<FaceInstancePool> m_faceInstancePool;
};

} // namespace Zerith