#pragma once

#include "chunk.h"
#include "blockbench_instance_generator.h"
#include "blockbench_instance_wrapper.h"
#include "texture_array.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace Zerith {

class ChunkMeshGenerator {
public:
    ChunkMeshGenerator();

    // Generate face instances for an entire chunk
    std::vector<BlockbenchInstanceGenerator::FaceInstance> generateChunkMesh(const Chunk& chunk);

    // Load block models
    void loadBlockModels();
    
    // Get the texture array for loading textures
    const std::shared_ptr<TextureArray>& getTextureArray() const { return m_textureArray; }

private:
    // Get the model path for a block type
    std::string getModelPath(BlockType type) const;
    
    // Generate faces for a single block
    void generateBlockFaces(const Chunk& chunk, int x, int y, int z, 
                           std::vector<BlockbenchInstanceGenerator::FaceInstance>& faces);

    // Block model generators
    std::unordered_map<BlockType, std::unique_ptr<BlockbenchInstanceWrapper>> m_blockGenerators;
    
    // Texture array manager
    std::shared_ptr<TextureArray> m_textureArray;
};

} // namespace Zerith