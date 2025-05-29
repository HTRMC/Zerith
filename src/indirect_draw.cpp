#include "indirect_draw.h"
#include <cstring>

namespace Zerith {

void IndirectDrawManager::addChunkData(uint32_t faceCount, const float* minBounds, const float* maxBounds, uint32_t firstFaceIndex) {
    if (faceCount == 0) return;
    
    // Add chunk data for GPU processing
    ChunkDrawData chunkData;
    std::memcpy(chunkData.minBounds, minBounds, sizeof(float) * 3);
    std::memcpy(chunkData.maxBounds, maxBounds, sizeof(float) * 3);
    chunkData.firstFaceIndex = firstFaceIndex;
    chunkData.faceCount = faceCount;
    chunkData.padding1 = 0.0f;
    chunkData.padding2 = 0.0f;
    chunkData.padding3[0] = 0;
    chunkData.padding3[1] = 0;
    
    m_chunkData.push_back(chunkData);
    m_totalFaceCount += faceCount;
}

void IndirectDrawManager::setSingleDrawCommand(uint32_t taskWorkgroups, uint32_t y, uint32_t z) {
    m_drawCommands.clear();
    m_drawCommands.emplace_back(taskWorkgroups, y, z);
}

void IndirectDrawManager::addChunkDrawCommand(uint32_t faceCount, const float* minBounds, const float* maxBounds, uint32_t firstFaceIndex) {
    // Legacy method - adds both chunk data and a draw command
    addChunkData(faceCount, minBounds, maxBounds, firstFaceIndex);
    m_drawCommands.emplace_back(1, 1, 1);
}

void IndirectDrawManager::clear() {
    m_drawCommands.clear();
    m_chunkData.clear();
    m_totalFaceCount = 0;
}

} // namespace Zerith