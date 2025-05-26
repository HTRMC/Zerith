#include "chunk_mesh_generator.h"
#include "blockbench_parser.h"
#include "logger.h"

namespace Zerith {

ChunkMeshGenerator::ChunkMeshGenerator() {
    m_textureArray = std::make_shared<TextureArray>();
    LOG_INFO("Initializing ChunkMeshGenerator");
    loadBlockModels();
}

void ChunkMeshGenerator::loadBlockModels() {
    // Load oak planks (full block)
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/block.json");
        m_blockGenerators[BlockType::OAK_PLANKS] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::OAK_PLANKS, m_textureArray);
        LOG_DEBUG("Loaded oak planks model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load oak planks model: %s", e.what());
    }
    
    // Load oak slab
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/slab.json");
        m_blockGenerators[BlockType::OAK_SLAB] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::OAK_SLAB, m_textureArray);
        LOG_DEBUG("Loaded oak slab model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load oak slab model: %s", e.what());
    }
    
    // Load oak stairs
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/oak_stairs.json");
        // Flip the stairs upside down to fix orientation
        BlockbenchModel::Conversion::flipModelUpsideDown(model);
        m_blockGenerators[BlockType::OAK_STAIRS] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::OAK_STAIRS, m_textureArray);
        LOG_DEBUG("Loaded oak stairs model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load oak stairs model: %s", e.what());
    }
    
    // Load grass block
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/grass_block.json");
        m_blockGenerators[BlockType::GRASS_BLOCK] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::GRASS_BLOCK, m_textureArray);
        LOG_DEBUG("Loaded grass block model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load grass block model: %s", e.what());
    }
    
    // Load stone block
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/stone.json");
        m_blockGenerators[BlockType::STONE] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::STONE, m_textureArray);
        LOG_DEBUG("Loaded stone block model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load stone block model: %s", e.what());
    }
}

std::vector<BlockbenchInstanceGenerator::FaceInstance> ChunkMeshGenerator::generateChunkMesh(const Chunk& chunk) {
    std::vector<BlockbenchInstanceGenerator::FaceInstance> allFaces;
    
    // Iterate through all blocks in the chunk
    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                generateBlockFaces(chunk, x, y, z, allFaces);
            }
        }
    }
    
    return allFaces;
}

void ChunkMeshGenerator::generateBlockFaces(const Chunk& chunk, int x, int y, int z, 
                                           std::vector<BlockbenchInstanceGenerator::FaceInstance>& faces) {
    BlockType blockType = chunk.getBlock(x, y, z);
    
    // Skip air blocks
    if (blockType == BlockType::AIR) {
        return;
    }
    
    // Find the generator for this block type
    auto it = m_blockGenerators.find(blockType);
    if (it == m_blockGenerators.end()) {
        return; // No model for this block type
    }
    
    // Calculate world position of this block
    glm::vec3 blockWorldPos = glm::vec3(chunk.getChunkPosition()) * static_cast<float>(Chunk::CHUNK_SIZE);
    blockWorldPos += glm::vec3(x, y, z);
    
    // Generate faces at this position
    auto blockFaces = it->second->generateInstancesAtPosition(blockWorldPos);
    
    // Add all faces without culling
    for (const auto& face : blockFaces) {
        faces.push_back(face);
    }
}

} // namespace Zerith