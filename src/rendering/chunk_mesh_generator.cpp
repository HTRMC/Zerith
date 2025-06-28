#include "chunk_mesh_generator.h"
#include "blockbench_parser.h"
#include "logger.h"
#include "block_properties.h"
#include "block_face_bounds.h"
#include "blockbench_face_extractor.h"
#include "blocks.h"
#include "profiler.h"
#include "voxel_ao.h"
#include <filesystem>

namespace Zerith {

ChunkMeshGenerator::ChunkMeshGenerator() {
    m_textureArray = std::make_shared<TextureArray>();
    m_faceInstancePool = std::make_unique<FaceInstancePool>(16); // Pre-allocate 16 batches
    LOG_INFO("Initializing ChunkMeshGenerator with object pooling");
    loadBlockModels();
}

void ChunkMeshGenerator::loadBlockModels() {
    PROFILE_FUNCTION();
    // Initialize the unified blocks system
    Blocks::initialize();
    BlockProperties::initialize();
    
    const std::string modelsPath = "assets/zerith/models/block/";
    
    // Initialize the face bounds registry
    BlockFaceBoundsRegistry::getInstance().initialize(Blocks::getBlockCount());
    
    // Iterate through all registered blocks
    for (const auto& blockDef : Blocks::getAllBlocks()) {
        BlockType blockType = blockDef->getBlockType();
        
        // Skip air blocks
        if (blockDef->getId() == "air") {
            continue;
        }
        
        // Construct model path from block's model name
        std::string modelPath = modelsPath + blockDef->getModelName() + ".json";
        
        // Check if model file exists
        if (!std::filesystem::exists(modelPath)) {
            LOG_WARN("Model file not found for block '%s': %s", 
                blockDef->getDisplayName().c_str(), modelPath.c_str());
            continue;
        }
        
        try {
            // Load the model with texture array for immediate texture resolution
            auto model = BlockbenchParser::parseFromFileWithParents(modelPath, m_textureArray.get());
            
            // Extract and register face bounds
            auto faceBounds = BlockbenchFaceExtractor::extractBlockFaceBounds(model);
            BlockFaceBoundsRegistry::getInstance().setFaceBounds(blockType, faceBounds);
            
            // Debug output for partial blocks (slabs, stairs)
            if (blockDef->getId() == "oak_slab" || blockDef->getId() == "oak_stairs") {
                BlockbenchFaceExtractor::printBlockFaceBounds(faceBounds, blockDef->getDisplayName());
            }
            
            // Create the instance generator
            m_blockGenerators[blockType] = 
                std::make_unique<BlockbenchInstanceWrapper>(std::move(model), blockType, m_textureArray);
            
            LOG_INFO("Loaded model for block '%s' (ID: %s, Type: %d)", 
                blockDef->getDisplayName().c_str(), 
                blockDef->getId().c_str(), 
                static_cast<int>(blockType));
                
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to load model for block '%s': %s", 
                blockDef->getDisplayName().c_str(), e.what());
        }
    }
    
    LOG_INFO("Loaded models for %zu blocks", m_blockGenerators.size());
}

std::vector<BlockbenchInstanceGenerator::FaceInstance> ChunkMeshGenerator::generateChunkMesh(const Chunk& chunk) {
    PROFILE_FUNCTION();
    
    // Early exit for empty chunks
    bool isEmpty = true;
    for (int x = 0; x < Chunk::CHUNK_SIZE && isEmpty; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE && isEmpty; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE && isEmpty; ++z) {
                if (chunk.getBlock(x, y, z) != Blocks::AIR) {
                    isEmpty = false;
                }
            }
        }
    }
    
    if (isEmpty) {
        return {}; // Return empty vector for empty chunks
    }
    
    // Use binary meshing if enabled
    LOG_INFO("MESH GENERATOR: Binary meshing enabled = %s", m_binaryMeshingEnabled ? "true" : "false");
    if (m_binaryMeshingEnabled) {
        // Get chunk world position from the chunk itself
        glm::ivec3 chunkWorldPos = chunk.getChunkPosition();
        
        // Use AO-enabled binary meshing if ChunkManager is available
        std::optional<std::vector<BlockbenchInstanceGenerator::FaceInstance>> binaryResult;
        if (m_chunkManager) {
            LOG_INFO("MESH GENERATOR: Using binary meshing WITH AO (ChunkManager available)");
            binaryResult = HybridChunkMeshGenerator::generateOptimizedMeshWithAO(
                chunk, 
                chunkWorldPos, 
                m_chunkManager,
                *m_textureArray
            );
        } else {
            LOG_INFO("MESH GENERATOR: Using binary meshing WITHOUT AO (ChunkManager not available)");
            // Fallback to non-AO binary meshing
            binaryResult = HybridChunkMeshGenerator::generateOptimizedMesh(
                chunk, 
                chunkWorldPos, 
                *m_textureArray
            );
        }
        
        // If binary meshing succeeded (returned a value), use those faces
        if (binaryResult.has_value()) {
            LOG_INFO("MESH GENERATOR: Binary meshing SUCCESS - using binary mesh with %zu faces", binaryResult.value().size());
            return binaryResult.value();
        }
        
        LOG_INFO("MESH GENERATOR: Binary meshing FAILED - falling back to traditional meshing");
        // If binary meshing returned nullopt, fall through to traditional meshing
    }
    
    // Fall back to traditional meshing
    std::vector<BlockbenchInstanceGenerator::FaceInstance> allFaces;
    
    // Iterate through all blocks in the chunk (xyz order)
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                generateBlockFaces(chunk, x, y, z, allFaces);
            }
        }
    }
    
    return allFaces;
}

FaceInstancePool::FaceInstanceBatch ChunkMeshGenerator::generateChunkMeshPooled(const Chunk& chunk) {
    PROFILE_FUNCTION();
    
    auto batch = m_faceInstancePool->acquireBatch();
    
    // Early exit for empty chunks
    bool isEmpty = true;
    for (int x = 0; x < Chunk::CHUNK_SIZE && isEmpty; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE && isEmpty; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE && isEmpty; ++z) {
                if (chunk.getBlock(x, y, z) != Blocks::AIR) {
                    isEmpty = false;
                }
            }
        }
    }
    
    if (isEmpty) {
        return batch; // Return empty batch for empty chunks
    }
    
    // Estimate capacity based on chunk size (assume average 2 faces per block visible)
    batch.reserve(Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * 2);
    
    // Iterate through all blocks in the chunk (xyz order)
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                generateBlockFacesPooled(chunk, x, y, z, batch);
            }
        }
    }
    
    return batch;
}

LayeredChunkMesh ChunkMeshGenerator::generateLayeredChunkMesh(const Chunk& chunk) {
    PROFILE_FUNCTION();
    
    LayeredChunkMesh layeredMesh;
    
    // Early exit for empty chunks
    bool isEmpty = true;
    for (int x = 0; x < Chunk::CHUNK_SIZE && isEmpty; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE && isEmpty; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE && isEmpty; ++z) {
                if (chunk.getBlock(x, y, z) != Blocks::AIR) {
                    isEmpty = false;
                }
            }
        }
    }
    
    if (isEmpty) {
        return layeredMesh; // Return empty layered mesh for empty chunks
    }
    
    // Use binary meshing if enabled
    if (m_binaryMeshingEnabled) {
        LOG_INFO("LAYERED MESH GENERATOR (no neighbors): Binary meshing enabled = true");
        
        // Get chunk world position from the chunk itself
        glm::ivec3 chunkWorldPos = chunk.getChunkPosition();
        
        // Use AO-enabled binary meshing if ChunkManager is available
        std::optional<std::vector<BlockbenchInstanceGenerator::FaceInstance>> binaryResult;
        if (m_chunkManager) {
            LOG_INFO("LAYERED MESH GENERATOR (no neighbors): Using binary meshing WITH AO (ChunkManager available)");
            binaryResult = HybridChunkMeshGenerator::generateOptimizedMeshWithAO(
                chunk, 
                chunkWorldPos, 
                m_chunkManager,
                *m_textureArray
            );
        } else {
            LOG_INFO("LAYERED MESH GENERATOR (no neighbors): Using binary meshing WITHOUT AO (ChunkManager not available)");
            binaryResult = HybridChunkMeshGenerator::generateOptimizedMesh(
                chunk, 
                chunkWorldPos, 
                *m_textureArray
            );
        }
        
        // If binary meshing succeeded (returned a value), use those faces and separate by layer
        if (binaryResult.has_value()) {
            LOG_INFO("LAYERED MESH GENERATOR (no neighbors): Binary meshing SUCCESS - separating %zu faces by layer", binaryResult.value().size());
            
            // Separate the binary-generated faces into layers
            for (const auto& face : binaryResult.value()) {
                layeredMesh.getLayer(face.renderLayer).emplace_back(face);
            }
            
            return layeredMesh;
        }
        
        LOG_INFO("LAYERED MESH GENERATOR (no neighbors): Binary meshing FAILED - falling back to traditional meshing");
        // If binary meshing returned nullopt, fall through to traditional meshing
    }
    
    // Use traditional per-block meshing
    // Iterate through all blocks in the chunk (xyz order)
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                generateBlockFacesLayered(chunk, x, y, z, layeredMesh);
            }
        }
    }
    
    return layeredMesh;
}

LayeredChunkMesh ChunkMeshGenerator::generateLayeredChunkMeshWithNeighbors(
    const Chunk& chunk,
    const Chunk* neighborXMinus, const Chunk* neighborXPlus,
    const Chunk* neighborYMinus, const Chunk* neighborYPlus,
    const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    PROFILE_FUNCTION();
    
    LayeredChunkMesh layeredMesh;
    
    // Early exit for empty chunks
    bool isEmpty = true;
    for (int x = 0; x < Chunk::CHUNK_SIZE && isEmpty; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE && isEmpty; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE && isEmpty; ++z) {
                if (chunk.getBlock(x, y, z) != Blocks::AIR) {
                    isEmpty = false;
                }
            }
        }
    }
    
    if (isEmpty) {
        return layeredMesh; // Return empty layered mesh for empty chunks
    }
    
    // Use binary meshing if enabled
    if (m_binaryMeshingEnabled) {
        LOG_INFO("LAYERED MESH GENERATOR: Binary meshing enabled = true");
        
        // Get chunk world position from the chunk itself
        glm::ivec3 chunkWorldPos = chunk.getChunkPosition();
        
        // Use AO-enabled binary meshing if ChunkManager is available
        std::optional<std::vector<BlockbenchInstanceGenerator::FaceInstance>> binaryResult;
        if (m_chunkManager) {
            LOG_INFO("LAYERED MESH GENERATOR: Using binary meshing WITH AO (ChunkManager available)");
            binaryResult = HybridChunkMeshGenerator::generateOptimizedMeshWithAO(
                chunk, 
                chunkWorldPos, 
                m_chunkManager,
                *m_textureArray
            );
        } else {
            LOG_INFO("LAYERED MESH GENERATOR: Using binary meshing WITHOUT AO (ChunkManager not available)");
            binaryResult = HybridChunkMeshGenerator::generateOptimizedMesh(
                chunk, 
                chunkWorldPos, 
                *m_textureArray
            );
        }
        
        // If binary meshing succeeded (returned a value), use those faces and separate by layer
        if (binaryResult.has_value()) {
            LOG_INFO("LAYERED MESH GENERATOR: Binary meshing SUCCESS - separating %zu faces by layer", binaryResult.value().size());
            
            // Separate the binary-generated faces into layers
            for (const auto& face : binaryResult.value()) {
                layeredMesh.getLayer(face.renderLayer).emplace_back(face);
            }
            
            return layeredMesh;
        }
        
        LOG_INFO("LAYERED MESH GENERATOR: Binary meshing FAILED - falling back to traditional meshing");
        // If binary meshing returned nullopt, fall through to traditional meshing
    }
    
    // Use traditional per-block meshing
    // Iterate through all blocks in the chunk (xyz order)
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                generateBlockFacesLayeredWithNeighbors(chunk, x, y, z, layeredMesh,
                                                     neighborXMinus, neighborXPlus,
                                                     neighborYMinus, neighborYPlus,
                                                     neighborZMinus, neighborZPlus);
            }
        }
    }
    
    return layeredMesh;
}

void ChunkMeshGenerator::generateBlockFacesLayered(const Chunk& chunk, int x, int y, int z, 
                                                  LayeredChunkMesh& layeredMesh) {
    BlockType blockType = chunk.getBlock(x, y, z);
    
    // Skip air blocks
    if (blockType == Blocks::AIR) {
        return;
    }
    
    // Debug: check if this method is being called for water
    auto blockDefPtr = Blocks::getBlock(blockType);
    if (blockDefPtr && blockDefPtr->getId() == "water") {
        LOG_INFO("LAYERED METHOD CALLED FOR WATER at (%d,%d,%d)", x, y, z);
    }
    
    // Find the generator for this block type
    auto it = m_blockGenerators.find(blockType);
    if (it == m_blockGenerators.end()) {
        return; // No model for this block type
    }
    
    // Get the render layer for this block using the unified system
    RenderLayer renderLayer = Blocks::getRenderLayer(blockType);
    
    // Debug: log render layer for specific blocks
    if (blockDefPtr && blockDefPtr->getId() == "water") {
        LOG_INFO("WATER BLOCK FOUND: %s (type %d) has render layer %d", blockDefPtr->getId().c_str(), static_cast<int>(blockType), static_cast<int>(renderLayer));
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
            // Calculate ambient occlusion for this face (requires ChunkManager for cross-chunk support)
            if (m_chunkManager) {
                glm::ivec3 chunkWorldPos = chunk.getChunkPosition() * Chunk::CHUNK_SIZE;
                face.ao = VoxelAO::calculateFaceAO(m_chunkManager, chunkWorldPos, x, y, z, face.faceDirection);
            } else {
                // No ChunkManager available - set default AO (no occlusion)
                face.ao = glm::vec4(1.0f);
            }
            
            // Set the render layer on the face
            face.renderLayer = renderLayer;
            
            // Debug water faces
            if (blockDefPtr && blockDefPtr->getId() == "water") {
                LOG_INFO("WATER FACE ADDED: renderLayer=%d, assigned to layer %d", 
                        static_cast<int>(face.renderLayer), static_cast<int>(renderLayer));
            }
            
            // Add face to the appropriate render layer
            layeredMesh.getLayer(renderLayer).emplace_back(std::move(face));
        }
    }
}

void ChunkMeshGenerator::generateBlockFacesLayeredWithNeighbors(const Chunk& chunk, int x, int y, int z,
                                                               LayeredChunkMesh& layeredMesh,
                                                               const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                                               const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                                               const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    BlockType blockType = chunk.getBlock(x, y, z);
    
    // Skip air blocks
    if (blockType == Blocks::AIR) {
        return;
    }
    
    // Find the generator for this block type
    auto it = m_blockGenerators.find(blockType);
    if (it == m_blockGenerators.end()) {
        return; // No model for this block type
    }
    
    // Get the render layer for this block using the unified system
    RenderLayer renderLayer = Blocks::getRenderLayer(blockType);
    
    // Debug: log render layer for specific blocks
    auto blockDef = Blocks::getBlock(blockType);
    if (blockDef && blockDef->getId() == "water") {
        LOG_INFO("WATER BLOCK FOUND: %s (type %d) has render layer %d", blockDef->getId().c_str(), static_cast<int>(blockType), static_cast<int>(renderLayer));
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
            // Calculate ambient occlusion for this face (requires ChunkManager for cross-chunk support)
            if (m_chunkManager) {
                glm::ivec3 chunkWorldPos = chunk.getChunkPosition() * Chunk::CHUNK_SIZE;
                face.ao = VoxelAO::calculateFaceAO(m_chunkManager, chunkWorldPos, x, y, z, face.faceDirection);
            } else {
                // No ChunkManager available - set default AO (no occlusion)
                face.ao = glm::vec4(1.0f);
            }
            
            // Set the render layer on the face
            face.renderLayer = renderLayer;
            // Add face to the appropriate render layer
            layeredMesh.getLayer(renderLayer).emplace_back(std::move(face));
        }
    }
}

void ChunkMeshGenerator::generateBlockFaces(const Chunk& chunk, int x, int y, int z, 
                                           std::vector<BlockbenchInstanceGenerator::FaceInstance>& faces) {
    BlockType blockType = chunk.getBlock(x, y, z);
    
    // Skip air blocks
    if (blockType == Blocks::AIR) {
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
            // Calculate ambient occlusion for this face (requires ChunkManager for cross-chunk support)
            if (m_chunkManager) {
                glm::ivec3 chunkWorldPos = chunk.getChunkPosition() * Chunk::CHUNK_SIZE;
                face.ao = VoxelAO::calculateFaceAO(m_chunkManager, chunkWorldPos, x, y, z, face.faceDirection);
            } else {
                // No ChunkManager available - set default AO (no occlusion)
                face.ao = glm::vec4(1.0f);
            }
            faces.emplace_back(std::move(face));
        }
    }
}

void ChunkMeshGenerator::generateBlockFacesPooled(const Chunk& chunk, int x, int y, int z, 
                                                 FaceInstancePool::FaceInstanceBatch& batch) {
    BlockType blockType = chunk.getBlock(x, y, z);
    
    // Skip air blocks
    if (blockType == Blocks::AIR) {
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
                         face.uv, face.textureLayer);
        }
    }
}

bool ChunkMeshGenerator::isFaceVisibleWithNeighbors(const Chunk& chunk, int x, int y, int z, 
                                                   int dx, int dy, int dz,
                                                   const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                                   const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                                   const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    // Check if current block is not air
    BlockType currentBlock = chunk.getBlock(x, y, z);
    if (currentBlock == Blocks::AIR) {
        return false;
    }
    
    // Check adjacent block
    int nx = x + dx;
    int ny = y + dy;
    int nz = z + dz;
    
    BlockType adjacentBlock = Blocks::AIR;
    
    // If adjacent block is within chunk bounds, use normal check
    if (nx >= 0 && nx < Chunk::CHUNK_SIZE &&
        ny >= 0 && ny < Chunk::CHUNK_SIZE &&
        nz >= 0 && nz < Chunk::CHUNK_SIZE) {
        adjacentBlock = chunk.getBlock(nx, ny, nz);
    } else {
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
        
        // Get the block in the neighbor chunk
        adjacentBlock = neighborChunk->getBlock(neighborX, neighborY, neighborZ);
    }
    
    // If adjacent block is air, face is always visible
    if (adjacentBlock == Blocks::AIR) {
        return true;
    }
    
    // Get properties for both blocks
    const auto& currentProps = BlockProperties::getCullingProperties(currentBlock);
    const auto& adjacentProps = BlockProperties::getCullingProperties(adjacentBlock);
    
    // HACK: Never let stairs cull anything
    if (adjacentBlock == Blocks::OAK_STAIRS) {
        return true;
    }
    
    // Special handling for transparent blocks
    if (currentProps.isTransparent) {
        // If both blocks are the same type (e.g., glass-to-glass, water-to-water), cull the face
        if (currentBlock == adjacentBlock) {
            return false;
        }
        
        // Special handling for liquids (water)
        if (currentBlock == Blocks::WATER) {
            // Determine which face of the adjacent block we're against
            int adjacentFaceIndex = -1;
            if (dy == -1) adjacentFaceIndex = 1; // Adjacent block's up face
            else if (dy == 1) adjacentFaceIndex = 0; // Adjacent block's down face
            else if (dz == -1) adjacentFaceIndex = 3; // Adjacent block's south face
            else if (dz == 1) adjacentFaceIndex = 2; // Adjacent block's north face
            else if (dx == -1) adjacentFaceIndex = 5; // Adjacent block's east face
            else if (dx == 1) adjacentFaceIndex = 4; // Adjacent block's west face
            
            // If adjacent block is opaque and has full face culling, don't render water face
            if (!adjacentProps.isTransparent && adjacentFaceIndex >= 0 && 
                adjacentProps.faceCulling[adjacentFaceIndex] == CullFace::FULL) {
                return false;
            }
        }
        
        // Other transparent blocks (glass, leaves) should always show their faces
        return true;
    }
    
    // If adjacent block is transparent, always render the face
    if (adjacentProps.isTransparent) {
        return true;
    }
    
    // Determine which face we're checking based on direction
    int currentFaceIndex = -1;
    int adjacentFaceIndex = -1;
    
    if (dy == -1) {
        currentFaceIndex = 0;      // Current block's down face
        adjacentFaceIndex = 1;     // Adjacent block's up face
    } else if (dy == 1) {
        currentFaceIndex = 1;      // Current block's up face
        adjacentFaceIndex = 0;     // Adjacent block's down face
    } else if (dz == -1) {
        currentFaceIndex = 2;      // Current block's north face
        adjacentFaceIndex = 3;     // Adjacent block's south face
    } else if (dz == 1) {
        currentFaceIndex = 3;      // Current block's south face
        adjacentFaceIndex = 2;     // Adjacent block's north face
    } else if (dx == -1) {
        currentFaceIndex = 4;      // Current block's west face
        adjacentFaceIndex = 5;     // Adjacent block's east face
    } else if (dx == 1) {
        currentFaceIndex = 5;      // Current block's east face
        adjacentFaceIndex = 4;     // Adjacent block's west face
    }
    
    // Use face bounds for accurate culling
    if (currentFaceIndex >= 0 && adjacentFaceIndex >= 0) {
        const auto& faceBoundsRegistry = BlockFaceBoundsRegistry::getInstance();
        
        // Check if the adjacent face covers the current face
        // But don't cull stairs faces
        if (currentBlock != Blocks::OAK_STAIRS && 
            faceBoundsRegistry.shouldCullFaces(currentBlock, currentFaceIndex, 
                                               adjacentBlock, adjacentFaceIndex)) {
            return false; // Face is culled
        }
    }
    
    // Legacy check for backwards compatibility
    if (adjacentFaceIndex >= 0 && adjacentProps.faceCulling[adjacentFaceIndex] == CullFace::FULL && currentProps.canBeCulled) {
        // Additional check: don't cull stairs faces even if they can normally be culled
        if (currentBlock == Blocks::OAK_STAIRS) {
            return true; // Stairs faces are always visible
        }
        
        // Only use legacy culling if face bounds indicate full coverage
        const auto& faceBoundsRegistry = BlockFaceBoundsRegistry::getInstance();
        const auto& adjacentBounds = faceBoundsRegistry.getFaceBounds(adjacentBlock);
        if (adjacentBounds.faces[adjacentFaceIndex].isFull()) {
            return false; // Face is culled
        }
    }
    
    return true; // Face is visible
}

bool ChunkMeshGenerator::isFaceVisibleWithNeighborsAdvanced(const Chunk& chunk, int x, int y, int z, 
                                                           int faceDir,
                                                           const Chunk* neighborXMinus, const Chunk* neighborXPlus,
                                                           const Chunk* neighborYMinus, const Chunk* neighborYPlus,
                                                           const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    // Convert face direction to dx,dy,dz
    int dx = 0, dy = 0, dz = 0;
    switch (faceDir) {
        case 0: dy = -1; break; // Down
        case 1: dy = 1; break;  // Up
        case 2: dz = -1; break; // North
        case 3: dz = 1; break;  // South
        case 4: dx = -1; break; // West
        case 5: dx = 1; break;  // East
    }
    
    return isFaceVisibleWithNeighbors(chunk, x, y, z, dx, dy, dz,
                                     neighborXMinus, neighborXPlus,
                                     neighborYMinus, neighborYPlus,
                                     neighborZMinus, neighborZPlus);
}

std::vector<BlockbenchInstanceGenerator::FaceInstance> ChunkMeshGenerator::generateChunkMeshWithNeighbors(
    const Chunk& chunk, 
    const Chunk* neighborXMinus, const Chunk* neighborXPlus,
    const Chunk* neighborYMinus, const Chunk* neighborYPlus,
    const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    PROFILE_FUNCTION();
    
    // Early exit for empty chunks
    bool isEmpty = true;
    for (int x = 0; x < Chunk::CHUNK_SIZE && isEmpty; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE && isEmpty; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE && isEmpty; ++z) {
                if (chunk.getBlock(x, y, z) != Blocks::AIR) {
                    isEmpty = false;
                }
            }
        }
    }
    
    if (isEmpty) {
        return {}; // Return empty vector for empty chunks
    }
    
    std::vector<BlockbenchInstanceGenerator::FaceInstance> allFaces;
    
    // Use binary greedy meshing if enabled
    if (m_binaryMeshingEnabled) {
        // LOG_DEBUG("Using binary greedy meshing for chunk (%d, %d, %d)",
        //          chunk.getChunkPosition().x, chunk.getChunkPosition().y, chunk.getChunkPosition().z);
        
        // Convert chunk position to world position
        glm::ivec3 chunkWorldPos = chunk.getChunkPosition();
        
        // Generate optimized mesh using binary greedy meshing with AO
        std::optional<std::vector<BlockbenchInstanceGenerator::FaceInstance>> binaryResult;
        if (m_chunkManager) {
            LOG_INFO("MESH GENERATOR (WithNeighbors): Using binary meshing WITH AO (ChunkManager available)");
            binaryResult = HybridChunkMeshGenerator::generateOptimizedMeshWithAO(chunk, chunkWorldPos, m_chunkManager, *m_textureArray);
        } else {
            LOG_INFO("MESH GENERATOR (WithNeighbors): Using binary meshing WITHOUT AO (ChunkManager not available)");
            binaryResult = HybridChunkMeshGenerator::generateOptimizedMesh(chunk, chunkWorldPos, *m_textureArray);
        }
        
        // If binary meshing succeeded, use those faces
        if (binaryResult.has_value()) {
            allFaces = binaryResult.value();
            // LOG_DEBUG("Binary meshing generated %zu faces for chunk", allFaces.size());
        } else {
            // Binary meshing detected complex blocks, fall through to traditional meshing
            goto traditional_meshing;
        }
    } else {
        traditional_meshing:
        // Use traditional per-block meshing
        LOG_DEBUG("Using traditional meshing for chunk (%d, %d, %d)", 
                 chunk.getChunkPosition().x, chunk.getChunkPosition().y, chunk.getChunkPosition().z);
        
        // Iterate through all blocks in the chunk (xyz order)
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
                for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                    generateBlockFacesWithNeighbors(chunk, x, y, z, allFaces,
                                                  neighborXMinus, neighborXPlus,
                                                  neighborYMinus, neighborYPlus,
                                                  neighborZMinus, neighborZPlus);
                }
            }
        }
        
        LOG_DEBUG("Traditional meshing generated %zu faces for chunk", allFaces.size());
    }
    
    return allFaces;
}

FaceInstancePool::FaceInstanceBatch ChunkMeshGenerator::generateChunkMeshPooledWithNeighbors(
    const Chunk& chunk,
    const Chunk* neighborXMinus, const Chunk* neighborXPlus,
    const Chunk* neighborYMinus, const Chunk* neighborYPlus,
    const Chunk* neighborZMinus, const Chunk* neighborZPlus) {
    PROFILE_FUNCTION();
    
    auto batch = m_faceInstancePool->acquireBatch();
    
    // Early exit for empty chunks
    bool isEmpty = true;
    for (int x = 0; x < Chunk::CHUNK_SIZE && isEmpty; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE && isEmpty; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE && isEmpty; ++z) {
                if (chunk.getBlock(x, y, z) != Blocks::AIR) {
                    isEmpty = false;
                }
            }
        }
    }
    
    if (isEmpty) {
        return batch; // Return empty batch for empty chunks
    }
    
    // Estimate capacity based on chunk size (assume average 2 faces per block visible)
    batch.reserve(Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * 2);
    
    // Iterate through all blocks in the chunk (xyz order)
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
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
    if (blockType == Blocks::AIR) {
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
            // Calculate ambient occlusion for this face (requires ChunkManager for cross-chunk support)
            if (m_chunkManager) {
                glm::ivec3 chunkWorldPos = chunk.getChunkPosition() * Chunk::CHUNK_SIZE;
                face.ao = VoxelAO::calculateFaceAO(m_chunkManager, chunkWorldPos, x, y, z, face.faceDirection);
            } else {
                // No ChunkManager available - set default AO (no occlusion)
                face.ao = glm::vec4(1.0f);
            }
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
    if (blockType == Blocks::AIR) {
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
                         face.uv, face.textureLayer);
        }
    }
}

} // namespace Zerith