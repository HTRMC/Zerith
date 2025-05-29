#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace Zerith {

// Structure matching VkDrawMeshTasksIndirectCommandEXT
struct DrawMeshTasksIndirectCommand {
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
    
    DrawMeshTasksIndirectCommand(uint32_t x = 0, uint32_t y = 1, uint32_t z = 1)
        : groupCountX(x), groupCountY(y), groupCountZ(z) {}
};

// Per-chunk draw data for GPU culling
struct ChunkDrawData {
    alignas(16) float minBounds[3];
    float padding1;
    alignas(16) float maxBounds[3];
    float padding2;
    uint32_t firstFaceIndex;
    uint32_t faceCount;
    uint32_t padding3[2];
};

// Indirect draw manager
class IndirectDrawManager {
public:
    IndirectDrawManager() = default;
    ~IndirectDrawManager() = default;
    
    // Add chunk data without creating a draw command
    void addChunkData(uint32_t faceCount, const float* minBounds, const float* maxBounds, uint32_t firstFaceIndex);
    
    // Set a single draw command for all chunks
    void setSingleDrawCommand(uint32_t taskWorkgroups, uint32_t y = 1, uint32_t z = 1);
    
    // Add a draw command for a chunk (legacy)
    void addChunkDrawCommand(uint32_t faceCount, const float* minBounds, const float* maxBounds, uint32_t firstFaceIndex);
    
    // Clear all commands
    void clear();
    
    // Get draw commands
    const std::vector<DrawMeshTasksIndirectCommand>& getDrawCommands() const { return m_drawCommands; }
    const std::vector<ChunkDrawData>& getChunkData() const { return m_chunkData; }
    
    // Get total face count across all chunks
    uint32_t getTotalFaceCount() const { return m_totalFaceCount; }
    
    // Calculate required workgroups for face count
    static uint32_t calculateWorkgroups(uint32_t faceCount, uint32_t facesPerWorkgroup = 32) {
        return (faceCount + facesPerWorkgroup - 1) / facesPerWorkgroup;
    }
    
private:
    std::vector<DrawMeshTasksIndirectCommand> m_drawCommands;
    std::vector<ChunkDrawData> m_chunkData;
    uint32_t m_totalFaceCount = 0;
};

} // namespace Zerith