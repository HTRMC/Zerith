#include "chunk_manager.h"
#include "logger.h"
#include <algorithm>

namespace MeshShader {

ChunkManager::ChunkManager() {
    m_meshGenerator = std::make_unique<ChunkMeshGenerator>();
    LOG_INFO("ChunkManager initialized with render distance %d", m_renderDistance);
}

void ChunkManager::updateLoadedChunks(const glm::vec3& playerPosition) {
    // Get the chunk position the player is in
    glm::ivec3 playerChunkPos = worldToChunkPos(playerPosition);
    
    // Only update if player moved to a different chunk
    if (playerChunkPos == m_lastPlayerChunkPos) {
        return;
    }
    
    m_lastPlayerChunkPos = playerChunkPos;
    bool chunksChanged = false;
    
    // Find chunks to unload (outside render distance)
    std::vector<glm::ivec3> chunksToUnload;
    for (const auto& [chunkPos, chunk] : m_chunks) {
        if (!isChunkInRange(chunkPos, playerChunkPos)) {
            chunksToUnload.push_back(chunkPos);
        }
    }
    
    // Unload distant chunks
    for (const auto& chunkPos : chunksToUnload) {
        unloadChunk(chunkPos);
        chunksChanged = true;
    }
    
    // Load chunks within render distance
    for (int dx = -m_renderDistance; dx <= m_renderDistance; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) { // Limit vertical range
            for (int dz = -m_renderDistance; dz <= m_renderDistance; ++dz) {
                glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(dx, dy, dz);
                
                // Skip if outside render distance or already loaded
                if (!isChunkInRange(chunkPos, playerChunkPos) || m_chunks.find(chunkPos) != m_chunks.end()) {
                    continue;
                }
                
                loadChunk(chunkPos);
                chunksChanged = true;
            }
        }
    }
    
    // Rebuild combined face instances if chunks changed
    if (chunksChanged) {
        rebuildAllFaceInstances();
        // Only print chunk updates occasionally for performance
        static int updateCount = 0;
        if (++updateCount % 10 == 0) {
            LOG_DEBUG("Chunks loaded: %zu, Total faces: %zu", m_chunks.size(), m_allFaceInstances.size());
        }
    }
}

void ChunkManager::rebuildAllFaceInstances() {
    m_allFaceInstances.clear();
    
    // Reserve space for better performance
    size_t totalFaces = getTotalFaceCount();
    m_allFaceInstances.reserve(totalFaces);
    
    // Collect all face instances from all chunks
    for (const auto& [chunkPos, faces] : m_chunkMeshes) {
        m_allFaceInstances.insert(m_allFaceInstances.end(), faces.begin(), faces.end());
    }
}

Chunk* ChunkManager::getChunk(const glm::ivec3& chunkPos) {
    auto it = m_chunks.find(chunkPos);
    return (it != m_chunks.end()) ? it->second.get() : nullptr;
}

BlockType ChunkManager::getBlock(const glm::vec3& worldPos) const {
    glm::ivec3 chunkPos = worldToChunkPos(worldPos);
    
    auto it = m_chunks.find(chunkPos);
    if (it == m_chunks.end()) {
        return BlockType::AIR;
    }
    
    return it->second->getBlockWorld(worldPos);
}

void ChunkManager::setBlock(const glm::vec3& worldPos, BlockType type) {
    glm::ivec3 chunkPos = worldToChunkPos(worldPos);
    
    auto it = m_chunks.find(chunkPos);
    if (it == m_chunks.end()) {
        return;
    }
    
    glm::ivec3 localPos = it->second->worldToLocal(worldPos);
    it->second->setBlock(localPos.x, localPos.y, localPos.z, type);
    
    // Regenerate mesh for this chunk
    regenerateChunkMesh(chunkPos);
    
    // Also regenerate neighboring chunks if block is on edge
    if (localPos.x == 0) regenerateChunkMesh(chunkPos + glm::ivec3(-1, 0, 0));
    if (localPos.x == Chunk::CHUNK_SIZE - 1) regenerateChunkMesh(chunkPos + glm::ivec3(1, 0, 0));
    if (localPos.y == 0) regenerateChunkMesh(chunkPos + glm::ivec3(0, -1, 0));
    if (localPos.y == Chunk::CHUNK_SIZE - 1) regenerateChunkMesh(chunkPos + glm::ivec3(0, 1, 0));
    if (localPos.z == 0) regenerateChunkMesh(chunkPos + glm::ivec3(0, 0, -1));
    if (localPos.z == Chunk::CHUNK_SIZE - 1) regenerateChunkMesh(chunkPos + glm::ivec3(0, 0, 1));
    
    // Rebuild the combined face instances
    rebuildAllFaceInstances();
}

size_t ChunkManager::getTotalFaceCount() const {
    size_t total = 0;
    for (const auto& [chunkPos, faces] : m_chunkMeshes) {
        total += faces.size();
    }
    return total;
}

glm::ivec3 ChunkManager::worldToChunkPos(const glm::vec3& worldPos) const {
    return glm::ivec3(
        static_cast<int>(std::floor(worldPos.x / Chunk::CHUNK_SIZE)),
        static_cast<int>(std::floor(worldPos.y / Chunk::CHUNK_SIZE)),
        static_cast<int>(std::floor(worldPos.z / Chunk::CHUNK_SIZE))
    );
}

void ChunkManager::loadChunk(const glm::ivec3& chunkPos) {
    LOG_TRACE("Loading chunk at (%d, %d, %d)", chunkPos.x, chunkPos.y, chunkPos.z);
    // Create new chunk
    auto chunk = std::make_unique<Chunk>(chunkPos);
    
    // Generate terrain
    generateTerrain(*chunk);
    
    // Generate mesh
    auto faces = m_meshGenerator->generateChunkMesh(*chunk);
    m_chunkMeshes[chunkPos] = std::move(faces);
    
    // Store chunk
    m_chunks[chunkPos] = std::move(chunk);
}

void ChunkManager::unloadChunk(const glm::ivec3& chunkPos) {
    LOG_TRACE("Unloading chunk at (%d, %d, %d)", chunkPos.x, chunkPos.y, chunkPos.z);
    m_chunks.erase(chunkPos);
    m_chunkMeshes.erase(chunkPos);
}

void ChunkManager::generateTerrain(Chunk& chunk) {
    // Simple terrain generation for testing
    // Later this can be replaced with Perlin noise
    
    glm::ivec3 chunkWorldPos = chunk.getChunkPosition() * Chunk::CHUNK_SIZE;
    
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int z = 0; z < Chunk::CHUNK_SIZE; ++z) {
            int worldX = chunkWorldPos.x + x;
            int worldZ = chunkWorldPos.z + z;
            
            // Simple height function
            int height = 4 + static_cast<int>(
                2.0f * std::sin(worldX * 0.1f) * std::cos(worldZ * 0.1f)
            );
            
            // Ensure height is relative to chunk
            int relativeHeight = height - chunkWorldPos.y;
            
            for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
                int worldY = chunkWorldPos.y + y;
                
                if (worldY < height - 3) {
                    // Deep underground - stone
                    chunk.setBlock(x, y, z, BlockType::STONE);
                } else if (worldY < height) {
                    // Near surface - dirt (we'll use stone for now since we don't have dirt)
                    chunk.setBlock(x, y, z, BlockType::STONE);
                } else if (worldY == height) {
                    // Surface - grass
                    chunk.setBlock(x, y, z, BlockType::GRASS_BLOCK);
                }
                // Air above height
            }
            
            // Add some random stairs on the surface
            if (relativeHeight >= 0 && relativeHeight < Chunk::CHUNK_SIZE - 1 && (worldX + worldZ) % 7 == 0) {
                chunk.setBlock(x, relativeHeight + 1, z, BlockType::OAK_STAIRS);
            }
        }
    }
}

bool ChunkManager::isChunkInRange(const glm::ivec3& chunkPos, const glm::ivec3& centerChunkPos) const {
    glm::ivec3 diff = chunkPos - centerChunkPos;
    
    // Check horizontal distance (circular)
    int horizontalDist = diff.x * diff.x + diff.z * diff.z;
    if (horizontalDist > m_renderDistance * m_renderDistance) {
        return false;
    }
    
    // Check vertical distance (limited range)
    if (std::abs(diff.y) > 2) {
        return false;
    }
    
    return true;
}

void ChunkManager::regenerateChunkMesh(const glm::ivec3& chunkPos) {
    auto it = m_chunks.find(chunkPos);
    if (it == m_chunks.end()) {
        return;
    }
    
    // Regenerate mesh for this chunk
    auto faces = m_meshGenerator->generateChunkMesh(*it->second);
    m_chunkMeshes[chunkPos] = std::move(faces);
}

} // namespace MeshShader