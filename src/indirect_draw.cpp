#include "indirect_draw.h"
#include <cstring>

namespace Zerith {

void IndirectDrawManager::addChunkDrawCommand(uint32_t faceCount, const float* minBounds, const float* maxBounds, uint32_t firstFaceIndex) {
    if (faceCount == 0) return;
    
    // Calculate workgroups needed for this chunk
    uint32_t workgroups = calculateWorkgroups(faceCount);
    
    // Add draw command
    m_drawCommands.emplace_back(workgroups, 1, 1);
    
    // Add chunk data for GPU culling
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

void IndirectDrawManager::clear() {
    m_drawCommands.clear();
    m_chunkData.clear();
    m_totalFaceCount = 0;
}

} // namespace Zerith