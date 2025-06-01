#include "chunk_mesh_generator.h"
#include "blockbench_parser.h"
#include "logger.h"

namespace Zerith {

ChunkMeshGenerator::ChunkMeshGenerator() {
    m_textureArray = std::make_shared<TextureArray>();
    m_faceInstancePool = std::make_unique<FaceInstancePool>(16); // Pre-allocate 16 batches
    LOG_INFO("Initializing ChunkMeshGenerator with object pooling");
    loadBlockModels();
}

void ChunkMeshGenerator::loadBlockModels() {
    // Load oak planks (full block)
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/oak_planks.json");
        m_blockGenerators[BlockType::OAK_PLANKS] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::OAK_PLANKS, m_textureArray);
        LOG_DEBUG("Loaded oak planks model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load oak planks model: %s", e.what());
    }
    
    // Load oak slab
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/oak_slab.json");
        m_blockGenerators[BlockType::OAK_SLAB] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::OAK_SLAB, m_textureArray);
        LOG_DEBUG("Loaded oak slab model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load oak slab model: %s", e.what());
    }
    
    // Load oak stairs
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/oak_stairs.json");
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
    
    // Load dirt block
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/dirt.json");
        m_blockGenerators[BlockType::DIRT] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::DIRT, m_textureArray);
        LOG_DEBUG("Loaded dirt block model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load dirt block model: %s", e.what());
    }
    
    // Load oak log
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/oak_log.json");
        m_blockGenerators[BlockType::OAK_LOG] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::OAK_LOG, m_textureArray);
        LOG_DEBUG("Loaded oak log model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load oak log model: %s", e.what());
    }
    
    // Load oak leaves
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/oak_leaves.json");
        m_blockGenerators[BlockType::OAK_LEAVES] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::OAK_LEAVES, m_textureArray);
        LOG_DEBUG("Loaded oak leaves model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load oak leaves model: %s", e.what());
    }
    
    // Load crafting table
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/crafting_table.json");
        m_blockGenerators[BlockType::CRAFTING_TABLE] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::CRAFTING_TABLE, m_textureArray);
        LOG_DEBUG("Loaded crafting table model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load crafting table model: %s", e.what());
    }
    
    // Load glass block
    try {
        auto model = BlockbenchParser::parseFromFileWithParents("assets/glass.json");
        m_blockGenerators[BlockType::GLASS] = 
            std::make_unique<BlockbenchInstanceWrapper>(std::move(model), BlockType::GLASS, m_textureArray);
        LOG_DEBUG("Loaded glass block model");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load glass block model: %s", e.what());
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

FaceInstancePool::FaceInstanceBatch ChunkMeshGenerator::generateChunkMeshPooled(const Chunk& chunk) {
    auto batch = m_faceInstancePool->acquireBatch();
    
    // Estimate capacity based on chunk size (assume average 2 faces per block visible)
    batch.reserve(Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * 2);
    
    // Iterate through all blocks in the chunk
    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                generateBlockFacesPooled(chunk, x, y, z, batch);
            }
        }
    }
    
    return batch;
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
    
    // Face culling: only add faces that are visible
    for (auto&& face : blockFaces) {
        bool shouldRender = false;
        
        // Check visibility based on face direction
        switch (face.faceDirection) {
            case 0: // Down face (Y-)
                shouldRender = chunk.isFaceVisible(x, y, z, 0, -1, 0);
                break;
            case 1: // Up face (Y+)
                shouldRender = chunk.isFaceVisible(x, y, z, 0, 1, 0);
                break;
            case 2: // North face (Z-)
                shouldRender = chunk.isFaceVisible(x, y, z, 0, 0, -1);
                break;
            case 3: // South face (Z+)
                shouldRender = chunk.isFaceVisible(x, y, z, 0, 0, 1);
                break;
            case 4: // West face (X-)
                shouldRender = chunk.isFaceVisible(x, y, z, -1, 0, 0);
                break;
            case 5: // East face (X+)
                shouldRender = chunk.isFaceVisible(x, y, z, 1, 0, 0);
                break;
            default:
                shouldRender = true; // Render unknown faces by default
                break;
        }
        
        if (shouldRender) {
            faces.emplace_back(std::move(face));
        }
    }
}

void ChunkMeshGenerator::generateBlockFacesPooled(const Chunk& chunk, int x, int y, int z, 
                                                 FaceInstancePool::FaceInstanceBatch& batch) {
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
    
    // Generate faces at this position into a temporary container
    auto blockFaces = it->second->generateInstancesAtPosition(blockWorldPos);
    
    // Face culling: only add faces that are visible
    for (auto&& face : blockFaces) {
        bool shouldRender = false;
        
        // Check visibility based on face direction
        switch (face.faceDirection) {
            case 0: // Down face (Y-)
                shouldRender = chunk.isFaceVisible(x, y, z, 0, -1, 0);
                break;
            case 1: // Up face (Y+)
                shouldRender = chunk.isFaceVisible(x, y, z, 0, 1, 0);
                break;
            case 2: // North face (Z-)
                shouldRender = chunk.isFaceVisible(x, y, z, 0, 0, -1);
                break;
            case 3: // South face (Z+)
                shouldRender = chunk.isFaceVisible(x, y, z, 0, 0, 1);
                break;
            case 4: // West face (X-)
                shouldRender = chunk.isFaceVisible(x, y, z, -1, 0, 0);
                break;
            case 5: // East face (X+)
                shouldRender = chunk.isFaceVisible(x, y, z, 1, 0, 0);
                break;
            default:
                shouldRender = true; // Render unknown faces by default
                break;
        }
        
        if (shouldRender) {
            // Add face to batch through the pool
            batch.addFace(face.position, face.rotation, face.scale, face.faceDirection, 
                         face.uv, face.textureLayer, face.textureName);
        }
    }
}

bool ChunkMeshGenerator::isFaceVisibleWithNeighbors(const Chunk& chunk, int x, int y, int z, 
                                                   int dx, int dy, int dz,
                                                   const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                                   const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                                   const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    // Check if current block is not air
    if (chunk.getBlock(x, y, z) == BlockType::AIR) {
        return false;
    }
    
    // Check adjacent block
    int nx = x + dx;
    int ny = y + dy;
    int nz = z + dz;
    
    // If adjacent block is within chunk bounds, use normal check
    if (nx >= 0 && nx < Chunk::CHUNK_SIZE &&
        ny >= 0 && ny < Chunk::CHUNK_SIZE &&
        nz >= 0 && nz < Chunk::CHUNK_SIZE) {
        return chunk.getBlock(nx, ny, nz) == BlockType::AIR;
    }
    
    // Adjacent block is in a neighboring chunk
    const Chunk* neighborChunk = nullptr;
    int neighborX = nx;
    int neighborY = ny;
    int neighborZ = nz;
    
    // Determine which neighbor chunk and adjust coordinates
    if (nx < 0 && neighborXMinus) {
        neighborChunk = neighborXMinus;
        neighborX = Chunk::CHUNK_SIZE - 1;
    } else if (nx >= Chunk::CHUNK_SIZE && neighborXPlus) {
        neighborChunk = neighborXPlus;
        neighborX = 0;
    }
    
    if (ny < 0 && neighborYMinus) {
        neighborChunk = neighborYMinus;
        neighborY = Chunk::CHUNK_SIZE - 1;
    } else if (ny >= Chunk::CHUNK_SIZE && neighborYPlus) {
        neighborChunk = neighborYPlus;
        neighborY = 0;
    }
    
    if (nz < 0 && neighborZMinus) {
        neighborChunk = neighborZMinus;
        neighborZ = Chunk::CHUNK_SIZE - 1;
    } else if (nz >= Chunk::CHUNK_SIZE && neighborZPlus) {
        neighborChunk = neighborZPlus;
        neighborZ = 0;
    }
    
    // If no neighbor chunk exists, face is visible (edge of world)
    if (!neighborChunk) {
        return true;
    }
    
    // Check the block in the neighbor chunk
    return neighborChunk->getBlock(neighborX, neighborY, neighborZ) == BlockType::AIR;
}

std::vector<BlockbenchInstanceGenerator::FaceInstance> ChunkMeshGenerator::generateChunkMeshWithNeighbors(
    const Chunk& chunk, 
    const Chunk* neighborXMinus, const Chunk* neighborXPlus,
    const Chunk* neighborYMinus, const Chunk* neighborYPlus,
    const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    
    std::vector<BlockbenchInstanceGenerator::FaceInstance> allFaces;
    
    // Iterate through all blocks in the chunk
    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                generateBlockFacesWithNeighbors(chunk, x, y, z, allFaces,
                                              neighborXMinus, neighborXPlus,
                                              neighborYMinus, neighborYPlus,
                                              neighborZMinus, neighborZPlus);
            }
        }
    }
    
    return allFaces;
}

FaceInstancePool::FaceInstanceBatch ChunkMeshGenerator::generateChunkMeshPooledWithNeighbors(
    const Chunk& chunk,
    const Chunk* neighborXMinus, const Chunk* neighborXPlus,
    const Chunk* neighborYMinus, const Chunk* neighborYPlus,
    const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    
    auto batch = m_faceInstancePool->acquireBatch();
    
    // Estimate capacity based on chunk size (assume average 2 faces per block visible)
    batch.reserve(Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * 2);
    
    // Iterate through all blocks in the chunk
    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
                generateBlockFacesPooledWithNeighbors(chunk, x, y, z, batch,
                                                    neighborXMinus, neighborXPlus,
                                                    neighborYMinus, neighborYPlus,
                                                    neighborZMinus, neighborZPlus);
            }
        }
    }
    
    return batch;
}

void ChunkMeshGenerator::generateBlockFacesWithNeighbors(const Chunk& chunk, int x, int y, int z,
                                                        std::vector<BlockbenchInstanceGenerator::FaceInstance>& faces,
                                                        const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                                        const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                                        const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
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
    
    // Face culling: only add faces that are visible
    for (auto&& face : blockFaces) {
        bool shouldRender = false;
        
        // Check visibility based on face direction
        switch (face.faceDirection) {
            case 0: // Down face (Y-)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 0, -1, 0,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 1: // Up face (Y+)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 0, 1, 0,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 2: // North face (Z-)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 0, 0, -1,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 3: // South face (Z+)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 0, 0, 1,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 4: // West face (X-)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, -1, 0, 0,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 5: // East face (X+)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 1, 0, 0,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            default:
                shouldRender = true; // Render unknown faces by default
                break;
        }
        
        if (shouldRender) {
            faces.emplace_back(std::move(face));
        }
    }
}

void ChunkMeshGenerator::generateBlockFacesPooledWithNeighbors(const Chunk& chunk, int x, int y, int z,
                                                              FaceInstancePool::FaceInstanceBatch& batch,
                                                              const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                                              const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                                              const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
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
    
    // Generate faces at this position into a temporary container
    auto blockFaces = it->second->generateInstancesAtPosition(blockWorldPos);
    
    // Face culling: only add faces that are visible
    for (auto&& face : blockFaces) {
        bool shouldRender = false;
        
        // Check visibility based on face direction
        switch (face.faceDirection) {
            case 0: // Down face (Y-)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 0, -1, 0,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 1: // Up face (Y+)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 0, 1, 0,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 2: // North face (Z-)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 0, 0, -1,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 3: // South face (Z+)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 0, 0, 1,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 4: // West face (X-)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, -1, 0, 0,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            case 5: // East face (X+)
                shouldRender = isFaceVisibleWithNeighbors(chunk, x, y, z, 1, 0, 0,
                                                         neighborXMinus, neighborXPlus,
                                                         neighborYMinus, neighborYPlus,
                                                         neighborZMinus, neighborZPlus);
                break;
            default:
                shouldRender = true; // Render unknown faces by default
                break;
        }
        
        if (shouldRender) {
            // Add face to batch through the pool
            batch.addFace(face.position, face.rotation, face.scale, face.faceDirection, 
                         face.uv, face.textureLayer, face.textureName);
        }
    }
}

} // namespace Zerith