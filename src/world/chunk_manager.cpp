#include "blocks.h"
#include "chunk_manager.h"
#include "logger.h"
#include "world_constants.h"
#include "profiler.h"
#include <algorithm>
#include <chrono>

namespace Zerith {

ChunkManager::ChunkManager() {
    m_meshGenerator = std::make_unique<ChunkMeshGenerator>();
    m_meshGenerator->setChunkManager(this); // Enable cross-chunk AO
    m_terrainGenerator = std::make_unique<TerrainGenerator>();
    
    // Initialize octree with generous world bounds
    // These bounds can be adjusted based on your world size requirements
    AABB worldBounds(
        glm::vec3(-1000.0f, -1000.0f, -1000.0f),
        glm::vec3(1000.0f, 1000.0f, 1000.0f)
    );
    m_chunkOctree = std::make_unique<ChunkOctree>(worldBounds);
    
    // Initialize global thread pool if not already done
    if (!g_threadPool) {
        initializeThreadPool();
    }
    
    LOG_INFO("ChunkManager initialized with render distance %d using global thread pool", 
             m_renderDistance);
}

ChunkManager::~ChunkManager() {
    // Cancel all pending tasks
    {
        std::lock_guard<std::mutex> loadLock(m_loadingMutex);
        for (const auto& [chunkPos, taskId] : m_loadingChunks) {
            g_threadPool->cancelTask(taskId);
        }
        m_loadingChunks.clear();
    }
    
    {
        std::lock_guard<std::mutex> meshLock(m_meshingMutex);
        for (const auto& [chunkPos, taskId] : m_meshingChunks) {
            g_threadPool->cancelTask(taskId);
        }
        m_meshingChunks.clear();
    }
}

void ChunkManager::updateLoadedChunks(const glm::vec3& playerPosition) {
    // Process any completed chunks first
    processCompletedChunks();
    
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
    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        for (const auto& [chunkPos, chunk] : m_chunks) {
            if (!isChunkInRange(chunkPos, playerChunkPos)) {
                chunksToUnload.push_back(chunkPos);
            }
        }
    }
    
    // Unload distant chunks
    for (const auto& chunkPos : chunksToUnload) {
        unloadChunk(chunkPos);
        chunksChanged = true;
    }
    
    // Load chunks within render distance asynchronously
    for (int dx = -m_renderDistance; dx <= m_renderDistance; ++dx) {
        for (int dy = -m_renderDistance; dy <= m_renderDistance; ++dy) {
            for (int dz = -m_renderDistance; dz <= m_renderDistance; ++dz) {
                glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(dx, dy, dz);
                
                // Skip if outside world height limits
                int worldY = chunkPos.y * Chunk::CHUNK_SIZE;
                if (worldY < WORLD_MIN_Y || worldY > WORLD_MAX_Y) {
                    continue;
                }
                
                // Skip if outside render distance
                if (!isChunkInRange(chunkPos, playerChunkPos)) {
                    continue;
                }
                
                // Check if already loaded or loading
                {
                    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
                    if (m_chunks.find(chunkPos) != m_chunks.end()) {
                        continue;
                    }
                }
                
                {
                    std::lock_guard<std::mutex> lock(m_loadingMutex);
                    if (m_loadingChunks.find(chunkPos) != m_loadingChunks.end()) {
                        continue;
                    }
                }
                
                // Calculate priority based on distance to player
                glm::ivec3 diff = chunkPos - playerChunkPos;
                int distance = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                int priority = 1000 - distance; // Closer chunks get higher priority
                
                loadChunkAsync(chunkPos, priority);
            }
        }
    }
    
    // Mark for rebuild if chunks changed
    if (chunksChanged) {
        m_needsRebuild = true;
        rebuildAllFaceInstances();
        // Only print chunk updates occasionally for performance
        static int updateCount = 0;
        if (++updateCount % 10 == 0) {
            std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
            LOG_DEBUG("Chunks loaded: %zu, Total faces: %zu", m_chunks.size(), m_allFaceInstances.size());
        }
    }
}

void ChunkManager::rebuildAllFaceInstances() {
    // Build new vector instead of clearing and rebuilding
    std::vector<BlockbenchInstanceGenerator::FaceInstance> newFaceInstances;
    
    // Lock once for the entire operation - more efficient
    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        
        // Calculate total size while we have the lock
        size_t totalFaces = 0;
        for (const auto& [chunkPos, faces] : m_chunkMeshes) {
            totalFaces += faces.size();
        }
        
        // Reserve space for better performance
        newFaceInstances.reserve(totalFaces);
        
        // Collect all face instances from all chunks
        for (const auto& [chunkPos, faces] : m_chunkMeshes) {
            newFaceInstances.insert(newFaceInstances.end(), faces.begin(), faces.end());
        }
    }
    
    // Move the new vector to replace the old one (avoids unnecessary deallocations)
    m_allFaceInstances = std::move(newFaceInstances);
    
    // Rebuild indirect commands for proper GPU indirect drawing
    rebuildIndirectCommands();
}

void ChunkManager::rebuildIndirectCommands() {
    m_indirectDrawManager.clear();
    
    // Build chunk data for all chunks
    uint32_t currentFaceIndex = 0;
    uint32_t chunkCount = 0;
    
    for (const auto& [chunkPos, faces] : m_chunkMeshes) {
        if (faces.empty()) continue;
        
        // Calculate chunk bounds in world space
        glm::vec3 chunkWorldPos = glm::vec3(chunkPos) * float(Chunk::CHUNK_SIZE);
        float minBounds[3] = {chunkWorldPos.x, chunkWorldPos.y, chunkWorldPos.z};
        float maxBounds[3] = {
            chunkWorldPos.x + Chunk::CHUNK_SIZE,
            chunkWorldPos.y + Chunk::CHUNK_SIZE,
            chunkWorldPos.z + Chunk::CHUNK_SIZE
        };
        
        // Add chunk data (but not individual draw commands)
        m_indirectDrawManager.addChunkData(
            static_cast<uint32_t>(faces.size()),
            minBounds,
            maxBounds,
            currentFaceIndex
        );
        
        currentFaceIndex += static_cast<uint32_t>(faces.size());
        chunkCount++;
    }
    
    // Create a single indirect command that launches one task workgroup per chunk
    if (chunkCount > 0) {
        m_indirectDrawManager.setSingleDrawCommand(chunkCount, 1, 1);
    }
}

Chunk* ChunkManager::getChunk(const glm::ivec3& chunkPos) {
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(chunkPos);
    return (it != m_chunks.end()) ? it->second.get() : nullptr;
}

BlockType ChunkManager::getBlock(const glm::vec3& worldPos) const {
    glm::ivec3 chunkPos = worldToChunkPos(worldPos);
    
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(chunkPos);
    if (it == m_chunks.end()) {
        return Blocks::AIR;
    }
    
    return it->second->getBlockWorld(worldPos);
}

void ChunkManager::setBlock(const glm::vec3& worldPos, BlockType type) {
    glm::ivec3 chunkPos = worldToChunkPos(worldPos);
    
    // First check if chunk is loaded (read lock)
    Chunk* chunk = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        auto it = m_chunks.find(chunkPos);
        if (it != m_chunks.end()) {
            chunk = it->second.get();
        }
    }
    
    // If chunk doesn't exist, load it synchronously
    if (!chunk) {
        LOG_DEBUG("Chunk at (%d, %d, %d) not loaded for setBlock, loading synchronously", 
                 chunkPos.x, chunkPos.y, chunkPos.z);
        loadChunk(chunkPos);
        
        // Try again after loading
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        auto it = m_chunks.find(chunkPos);
        if (it == m_chunks.end()) {
            LOG_ERROR("Failed to load chunk at (%d, %d, %d) for setBlock", 
                     chunkPos.x, chunkPos.y, chunkPos.z);
            return;
        }
        chunk = it->second.get();
    }
    
    // Use per-chunk locking for the actual block modification
    auto chunkMutex = getChunkMutex(chunkPos);
    glm::ivec3 localPos;
    {
        std::lock_guard<std::mutex> chunkLock(*chunkMutex);
        localPos = chunk->worldToLocal(worldPos);
        chunk->setBlock(localPos.x, localPos.y, localPos.z, type);
        LOG_DEBUG("Block %s at world pos (%.1f, %.1f, %.1f) chunk pos (%d, %d, %d) local pos (%d, %d, %d)", 
                 type == Blocks::AIR ? "destroyed" : "placed",
                 worldPos.x, worldPos.y, worldPos.z,
                 chunkPos.x, chunkPos.y, chunkPos.z,
                 localPos.x, localPos.y, localPos.z);
    }
    
    // Regenerate mesh for this chunk
    regenerateChunkMesh(chunkPos);
    
    // Also regenerate neighboring chunks if block is on edge
    if (localPos.x == 0) regenerateChunkMesh(chunkPos + glm::ivec3(-1, 0, 0));
    if (localPos.x == Chunk::CHUNK_SIZE - 1) regenerateChunkMesh(chunkPos + glm::ivec3(1, 0, 0));
    if (localPos.y == 0) regenerateChunkMesh(chunkPos + glm::ivec3(0, -1, 0));
    if (localPos.y == Chunk::CHUNK_SIZE - 1) regenerateChunkMesh(chunkPos + glm::ivec3(0, 1, 0));
    if (localPos.z == 0) regenerateChunkMesh(chunkPos + glm::ivec3(0, 0, -1));
    if (localPos.z == Chunk::CHUNK_SIZE - 1) regenerateChunkMesh(chunkPos + glm::ivec3(0, 0, 1));
    
    // Mark for rebuild and rebuild the combined face instances
    m_needsRebuild = true;
    rebuildAllFaceInstances();
}

size_t ChunkManager::getTotalFaceCount() const {
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
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
    LOG_INFO("CHUNK MANAGER: loadChunk called for (%d, %d, %d)", chunkPos.x, chunkPos.y, chunkPos.z);
    // Create new chunk
    auto chunk = std::make_unique<Chunk>(chunkPos);
    
    // Generate terrain
    generateTerrain(*chunk);
    
    // Get neighboring chunks for proper face culling
    const Chunk* neighborXMinus = nullptr;
    const Chunk* neighborXPlus = nullptr;
    const Chunk* neighborYMinus = nullptr;
    const Chunk* neighborYPlus = nullptr;
    const Chunk* neighborZMinus = nullptr;
    const Chunk* neighborZPlus = nullptr;
    
    auto neighborIt = m_chunks.find(chunkPos + glm::ivec3(-1, 0, 0));
    if (neighborIt != m_chunks.end()) neighborXMinus = neighborIt->second.get();
    
    neighborIt = m_chunks.find(chunkPos + glm::ivec3(1, 0, 0));
    if (neighborIt != m_chunks.end()) neighborXPlus = neighborIt->second.get();
    
    neighborIt = m_chunks.find(chunkPos + glm::ivec3(0, -1, 0));
    if (neighborIt != m_chunks.end()) neighborYMinus = neighborIt->second.get();
    
    neighborIt = m_chunks.find(chunkPos + glm::ivec3(0, 1, 0));
    if (neighborIt != m_chunks.end()) neighborYPlus = neighborIt->second.get();
    
    neighborIt = m_chunks.find(chunkPos + glm::ivec3(0, 0, -1));
    if (neighborIt != m_chunks.end()) neighborZMinus = neighborIt->second.get();
    
    neighborIt = m_chunks.find(chunkPos + glm::ivec3(0, 0, 1));
    if (neighborIt != m_chunks.end()) neighborZPlus = neighborIt->second.get();
    
    // Generate layered mesh with neighbor awareness
    LOG_INFO("CHUNK MANAGER: Using generateLayeredChunkMeshWithNeighbors for chunk at (%d,%d,%d)", 
             chunkPos.x, chunkPos.y, chunkPos.z);
    auto layeredMesh = m_meshGenerator->generateLayeredChunkMeshWithNeighbors(*chunk,
                                                                               neighborXMinus, neighborXPlus,
                                                                               neighborYMinus, neighborYPlus,
                                                                               neighborZMinus, neighborZPlus);
    
    // Flatten the layered mesh into a single vector for compatibility
    // Order: opaque first, then cutout, then translucent (for proper rendering order)
    std::vector<BlockbenchInstanceGenerator::FaceInstance> faces;
    const auto& opaque = layeredMesh.getOpaqueFaces();
    const auto& cutout = layeredMesh.getCutoutFaces();
    const auto& translucent = layeredMesh.getTranslucentFaces();
    
    LOG_INFO("CHUNK (%d,%d,%d): Generated %zu opaque, %zu cutout, %zu translucent faces", 
             chunkPos.x, chunkPos.y, chunkPos.z, opaque.size(), cutout.size(), translucent.size());
    
    faces.reserve(opaque.size() + cutout.size() + translucent.size());
    faces.insert(faces.end(), opaque.begin(), opaque.end());
    faces.insert(faces.end(), cutout.begin(), cutout.end());
    faces.insert(faces.end(), translucent.begin(), translucent.end());
    
    m_chunkMeshes[chunkPos] = std::move(faces);
    
    // Store chunk
    m_chunks[chunkPos] = std::move(chunk);
    
    // Add to octree for spatial queries
    m_chunkOctree->addChunk(m_chunks[chunkPos].get());
}

void ChunkManager::unloadChunk(const glm::ivec3& chunkPos) {
    LOG_TRACE("Unloading chunk at (%d, %d, %d)", chunkPos.x, chunkPos.y, chunkPos.z);
    
    // Cancel any pending async tasks for this chunk first
    {
        std::lock_guard<std::mutex> loadLock(m_loadingMutex);
        auto loadIt = m_loadingChunks.find(chunkPos);
        if (loadIt != m_loadingChunks.end()) {
            g_threadPool->cancelTask(loadIt->second);
            m_loadingChunks.erase(loadIt);
        }
    }
    
    {
        std::lock_guard<std::mutex> meshLock(m_meshingMutex);
        auto meshIt = m_meshingChunks.find(chunkPos);
        if (meshIt != m_meshingChunks.end()) {
            g_threadPool->cancelTask(meshIt->second);
            m_meshingChunks.erase(meshIt);
        }
    }
    
    // Now safely remove the chunk data
    std::unique_lock<std::shared_mutex> lock(m_chunksMutex);
    
    // Remove from octree before erasing from the map
    auto it = m_chunks.find(chunkPos);
    if (it != m_chunks.end()) {
        m_chunkOctree->removeChunk(it->second.get());
        m_chunks.erase(it);
    }
    
    // Remove mesh data
    auto meshIt = m_chunkMeshes.find(chunkPos);
    if (meshIt != m_chunkMeshes.end()) {
        m_chunkMeshes.erase(meshIt);
    }
    
    // Clean up per-chunk mutex
    removeChunkMutex(chunkPos);
}

void ChunkManager::generateTerrain(Chunk& chunk) {
    m_terrainGenerator->generateTerrain(chunk);
}

bool ChunkManager::isChunkInRange(const glm::ivec3& chunkPos, const glm::ivec3& centerChunkPos) const {
    glm::ivec3 diff = chunkPos - centerChunkPos;
    
    // Check spherical distance for all dimensions
    int distSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    if (distSquared > m_renderDistance * m_renderDistance) {
        return false;
    }
    
    return true;
}


void ChunkManager::loadChunkAsync(const glm::ivec3& chunkPos, int priority) {
    // Calculate task priority based on distance
    TaskPriority taskPriority = TaskPriority::Normal;
    if (priority > 900) taskPriority = TaskPriority::Critical;
    else if (priority > 700) taskPriority = TaskPriority::High;
    else if (priority > 500) taskPriority = TaskPriority::Normal;
    else if (priority > 300) taskPriority = TaskPriority::Low;
    else taskPriority = TaskPriority::Idle;
    
    // Submit chunk loading task and get the real task ID
    auto [future, taskId] = g_threadPool->submitTaskWithId(
        [this, chunkPos]() -> int {
            auto chunkData = loadChunkBackground(chunkPos);
            
            // Add to completed chunks queue
            {
                std::lock_guard<std::mutex> lock(m_completedMutex);
                m_completedChunks.push({chunkPos, std::move(chunkData)});
            }
            
            // Remove from loading map
            {
                std::lock_guard<std::mutex> lock(m_loadingMutex);
                m_loadingChunks.erase(chunkPos);
            }
            return 0;
        },
        taskPriority,
        "LoadChunk_" + std::to_string(chunkPos.x) + "_" + std::to_string(chunkPos.y) + "_" + std::to_string(chunkPos.z)
    );
    
    // Track the loading task with the real task ID
    {
        std::lock_guard<std::mutex> lock(m_loadingMutex);
        m_loadingChunks[chunkPos] = taskId;
    }
}


std::vector<BlockbenchInstanceGenerator::FaceInstance> ChunkManager::generateMeshForChunk(
    const glm::ivec3& chunkPos, Chunk* chunk) {
    PROFILE_FUNCTION();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    if (!chunk) {
        return {};
    }
    
    // Get neighboring chunks for proper face culling
    const Chunk* neighborXMinus = nullptr;
    const Chunk* neighborXPlus = nullptr;
    const Chunk* neighborYMinus = nullptr;
    const Chunk* neighborYPlus = nullptr;
    const Chunk* neighborZMinus = nullptr;
    const Chunk* neighborZPlus = nullptr;
    
    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        
        auto neighborIt = m_chunks.find(chunkPos + glm::ivec3(-1, 0, 0));
        if (neighborIt != m_chunks.end()) neighborXMinus = neighborIt->second.get();
        
        neighborIt = m_chunks.find(chunkPos + glm::ivec3(1, 0, 0));
        if (neighborIt != m_chunks.end()) neighborXPlus = neighborIt->second.get();
        
        neighborIt = m_chunks.find(chunkPos + glm::ivec3(0, -1, 0));
        if (neighborIt != m_chunks.end()) neighborYMinus = neighborIt->second.get();
        
        neighborIt = m_chunks.find(chunkPos + glm::ivec3(0, 1, 0));
        if (neighborIt != m_chunks.end()) neighborYPlus = neighborIt->second.get();
        
        neighborIt = m_chunks.find(chunkPos + glm::ivec3(0, 0, -1));
        if (neighborIt != m_chunks.end()) neighborZMinus = neighborIt->second.get();
        
        neighborIt = m_chunks.find(chunkPos + glm::ivec3(0, 0, 1));
        if (neighborIt != m_chunks.end()) neighborZPlus = neighborIt->second.get();
    }
    
    // Generate mesh with neighbor awareness
    auto result = m_meshGenerator->generateChunkMeshWithNeighbors(
        *chunk,
        neighborXMinus, neighborXPlus,
        neighborYMinus, neighborYPlus,
        neighborZMinus, neighborZPlus);
    
    auto end = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration<float, std::milli>(end - start).count();
    
    // Call timing callback if set
    if (m_meshGenCallback) {
        m_meshGenCallback(duration);
    }
    
    return result;
}

std::unique_ptr<ChunkData> ChunkManager::loadChunkBackground(const glm::ivec3& chunkPos) {
    LOG_TRACE("Loading chunk at (%d, %d, %d) in background", chunkPos.x, chunkPos.y, chunkPos.z);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto chunkData = std::make_unique<ChunkData>();
    
    // Create new chunk
    chunkData->chunk = std::make_unique<Chunk>(chunkPos);
    
    // Generate terrain
    generateTerrain(*chunkData->chunk);
    
    chunkData->ready = true;
    
    auto end = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration<float, std::milli>(end - start).count();
    
    // Call timing callback if set
    if (m_chunkGenCallback) {
        m_chunkGenCallback(duration);
    }
    
    return chunkData;
}

void ChunkManager::processCompletedChunks() {
    // Process completed chunk loads (terrain generation)
    {
        std::lock_guard<std::mutex> completedLock(m_completedMutex);
        
        std::vector<glm::ivec3> chunksToRegenerate;
        
        // Process all completed chunks
        while (!m_completedChunks.empty()) {
            auto [chunkPos, chunkData] = std::move(m_completedChunks.front());
            m_completedChunks.pop();
            
            // Check if this chunk position is still needed (not unloaded while loading)
            bool shouldProcess = false;
            {
                std::shared_lock<std::shared_mutex> readLock(m_chunksMutex);
                // Only process if chunk isn't already loaded and is still within render distance
                if (m_chunks.find(chunkPos) == m_chunks.end()) {
                    glm::ivec3 diff = chunkPos - m_lastPlayerChunkPos;
                    int distSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                    shouldProcess = (distSquared <= m_renderDistance * m_renderDistance);
                }
            }
            
            if (!shouldProcess) {
                LOG_TRACE("Discarding completed chunk at (%d, %d, %d) - no longer needed", 
                         chunkPos.x, chunkPos.y, chunkPos.z);
                continue;
            }
            
            // Move to main chunk storage
            {
                std::unique_lock<std::shared_mutex> chunksLock(m_chunksMutex);
                // Double-check under write lock
                if (m_chunks.find(chunkPos) != m_chunks.end()) {
                    continue; // Already loaded by another thread
                }
                
                m_chunks[chunkPos] = std::move(chunkData->chunk);
                
                // Add the chunk to the octree for spatial queries
                m_chunkOctree->addChunk(m_chunks[chunkPos].get());
                
                // Check which neighboring chunks exist and need regeneration
                glm::ivec3 neighbors[6] = {
                    chunkPos + glm::ivec3(-1, 0, 0),
                    chunkPos + glm::ivec3(1, 0, 0),
                    chunkPos + glm::ivec3(0, -1, 0),
                    chunkPos + glm::ivec3(0, 1, 0),
                    chunkPos + glm::ivec3(0, 0, -1),
                    chunkPos + glm::ivec3(0, 0, 1)
                };
                
                for (const auto& neighborPos : neighbors) {
                    if (m_chunks.find(neighborPos) != m_chunks.end()) {
                        chunksToRegenerate.push_back(neighborPos);
                    }
                }
            }
            
            // Queue for mesh generation with high priority
            queueMeshGeneration(chunkPos, 1000);
        }
        
        // Queue neighbor chunks for mesh regeneration
        for (const auto& chunkPos : chunksToRegenerate) {
            queueMeshGeneration(chunkPos, 500);
        }
    }
    
    // Process completed mesh generations
    {
        std::lock_guard<std::mutex> completedMeshLock(m_completedMeshMutex);
        
        bool meshesUpdated = false;
        
        // Process all completed meshes
        while (!m_completedMeshes.empty()) {
            auto completedMesh = std::move(m_completedMeshes.front());
            m_completedMeshes.pop();
            
            // Check if chunk still exists before updating mesh
            {
                std::unique_lock<std::shared_mutex> chunksLock(m_chunksMutex);
                auto chunkIt = m_chunks.find(completedMesh.chunkPos);
                if (chunkIt != m_chunks.end()) {
                    // Chunk still exists, update its mesh
                    m_chunkMeshes[completedMesh.chunkPos] = std::move(completedMesh.faces);
                    meshesUpdated = true;
                } else {
                    LOG_TRACE("Discarding completed mesh for chunk at (%d, %d, %d) - chunk was unloaded", 
                             completedMesh.chunkPos.x, completedMesh.chunkPos.y, completedMesh.chunkPos.z);
                }
            }
        }
        
        // Rebuild if needed
        if (meshesUpdated) {
            m_needsRebuild = true;
            rebuildAllFaceInstances();
        }
    }
}

void ChunkManager::regenerateChunkMesh(const glm::ivec3& chunkPos) {
    // Check if chunk exists and queue it for mesh generation
    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        if (m_chunks.find(chunkPos) == m_chunks.end()) {
            return;
        }
    }
    
    queueMeshGeneration(chunkPos, 500);
}

void ChunkManager::queueMeshGeneration(const glm::ivec3& chunkPos, int priority) {
    // Check if already meshing
    {
        std::lock_guard<std::mutex> lock(m_meshingMutex);
        if (m_meshingChunks.find(chunkPos) != m_meshingChunks.end()) {
            return;
        }
    }
    
    // Calculate task priority based on distance
    TaskPriority taskPriority = TaskPriority::Normal;
    if (priority > 900) taskPriority = TaskPriority::High;
    else if (priority > 700) taskPriority = TaskPriority::Normal;
    else if (priority > 500) taskPriority = TaskPriority::Low;
    else taskPriority = TaskPriority::Idle;
    
    // Submit mesh generation task and get the real task ID
    auto [future, taskId] = g_threadPool->submitTaskWithId(
        [this, chunkPos]() -> int {
            // Get the chunk to generate mesh for
            Chunk* chunk = nullptr;
            {
                std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
                auto it = m_chunks.find(chunkPos);
                if (it == m_chunks.end()) {
                    // Chunk was unloaded, remove from meshing map and return
                    std::lock_guard<std::mutex> meshLock(m_meshingMutex);
                    m_meshingChunks.erase(chunkPos);
                    return 0;
                }
                chunk = it->second.get();
            }
            
            // Generate mesh
            auto faces = generateMeshForChunk(chunkPos, chunk);
            
            // Add to completed meshes queue
            {
                std::lock_guard<std::mutex> lock(m_completedMeshMutex);
                CompletedMesh completedMesh;
                completedMesh.chunkPos = chunkPos;
                completedMesh.faces = std::move(faces);
                m_completedMeshes.push(std::move(completedMesh));
            }
            
            // Remove from meshing map
            {
                std::lock_guard<std::mutex> lock(m_meshingMutex);
                m_meshingChunks.erase(chunkPos);
            }
            return 0;
        },
        taskPriority,
        "MeshGen_" + std::to_string(chunkPos.x) + "_" + std::to_string(chunkPos.y) + "_" + std::to_string(chunkPos.z)
    );
    
    // Track the meshing task with the real task ID
    {
        std::lock_guard<std::mutex> lock(m_meshingMutex);
        m_meshingChunks[chunkPos] = taskId;
    }
}

std::vector<Chunk*> ChunkManager::getChunksInRegion(const AABB& region) const {
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
    return m_chunkOctree->getChunksInRegion(region);
}

std::vector<Chunk*> ChunkManager::getChunksAlongRay(const glm::vec3& origin, 
                                                  const glm::vec3& direction, 
                                                  float maxDistance) const {
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
    return m_chunkOctree->getChunksAlongRay(origin, direction, maxDistance);
}

std::shared_ptr<std::mutex> ChunkManager::getChunkMutex(const glm::ivec3& chunkPos) const {
    std::lock_guard<std::mutex> lock(m_chunkMutexesMutex);
    auto it = m_chunkMutexes.find(chunkPos);
    if (it != m_chunkMutexes.end()) {
        return it->second;
    }
    
    auto chunkMutex = std::make_shared<std::mutex>();
    m_chunkMutexes[chunkPos] = chunkMutex;
    return chunkMutex;
}

void ChunkManager::removeChunkMutex(const glm::ivec3& chunkPos) {
    std::lock_guard<std::mutex> lock(m_chunkMutexesMutex);
    m_chunkMutexes.erase(chunkPos);
}

} // namespace Zerith