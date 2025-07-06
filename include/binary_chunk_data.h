#pragma once

#include <array>
#include <bitset>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>
#include "chunk.h"

namespace Zerith {

/**
 * Binary chunk data for greedy meshing optimization.
 * Splits chunk data into separate bit arrays per block type for fast bitwise operations.
 */
class BinaryChunkData {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
    
    // Use bitset for fast bitwise operations
    using BlockMask = std::bitset<CHUNK_VOLUME>;
    
    BinaryChunkData(const Chunk& chunk);
    
    // Get the block mask for a specific block type
    const BlockMask& getBlockMask(BlockType blockType) const;
    
    // Check if a block type exists in this chunk
    bool hasBlockType(BlockType blockType) const;
    
    // Get all block types present in this chunk
    const std::vector<BlockType>& getActiveBlockTypes() const;
    
    // Convert 3D coordinates to 1D bit index
    static constexpr int coordsToIndex(int x, int y, int z) {
        return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
    }
    
    // Convert 1D bit index to 3D coordinates
    static constexpr glm::ivec3 indexToCoords(int index) {
        int z = index / (CHUNK_SIZE * CHUNK_SIZE);
        int y = (index % (CHUNK_SIZE * CHUNK_SIZE)) / CHUNK_SIZE;
        int x = index % CHUNK_SIZE;
        return glm::ivec3(x, y, z);
    }
    
    // Check if a specific position has a block of given type
    bool hasBlockAt(int x, int y, int z, BlockType blockType) const;
    
private:
    // Map from block type to its bit mask
    std::unordered_map<BlockType, BlockMask> m_blockMasks;
    
    // Cache of active block types for iteration
    std::vector<BlockType> m_activeBlockTypes;
    
    // Empty mask for non-existent block types
    static const BlockMask s_emptyMask;
};

/**
 * Binary greedy mesher that operates on bit arrays for maximum performance.
 */
class BinaryGreedyMesher {
public:
    struct MeshQuad {
        glm::ivec3 position;    // Bottom-left corner position
        glm::ivec3 size;        // Width, height, depth (one dimension will be 1)
        int faceDirection;      // 0-5 (down, up, north, south, west, east)
        BlockType blockType;    // The block type this quad represents
    };
    
    // Generate mesh quads for a specific block type and face direction
    static std::vector<MeshQuad> generateQuads(
        const BinaryChunkData& chunkData,
        BlockType blockType,
        int faceDirection
    );
    
    // Generate mesh quads with neighbor chunk data for border face culling
    static std::vector<MeshQuad> generateQuadsWithNeighbors(
        const BinaryChunkData& chunkData,
        BlockType blockType,
        int faceDirection,
        const BinaryChunkData* neighborXMinus,
        const BinaryChunkData* neighborXPlus,
        const BinaryChunkData* neighborYMinus,
        const BinaryChunkData* neighborYPlus,
        const BinaryChunkData* neighborZMinus,
        const BinaryChunkData* neighborZPlus
    );
    
    // Generate all mesh quads for all block types in the chunk
    static std::vector<MeshQuad> generateAllQuads(const BinaryChunkData& chunkData);
    
    // Generate all mesh quads with neighbor chunk data for border face culling
    static std::vector<MeshQuad> generateAllQuadsWithNeighbors(
        const BinaryChunkData& chunkData,
        const BinaryChunkData* neighborXMinus,
        const BinaryChunkData* neighborXPlus,
        const BinaryChunkData* neighborYMinus,
        const BinaryChunkData* neighborYPlus,
        const BinaryChunkData* neighborZMinus,
        const BinaryChunkData* neighborZPlus
    );
    
    // Type alias for 2D slice bitset (needs to be public)
    using SliceMask = std::bitset<BinaryChunkData::CHUNK_SIZE * BinaryChunkData::CHUNK_SIZE>;
    
    // Generate visible face mask for a block type and face direction
    static SliceMask generateVisibleFaceMask(
        const BinaryChunkData& chunkData,
        BlockType blockType,
        int faceDirection,
        int sliceIndex
    );
    
    // Generate visible face mask with neighbor chunk data for border face culling
    static SliceMask generateVisibleFaceMaskWithNeighbors(
        const BinaryChunkData& chunkData,
        BlockType blockType,
        int faceDirection,
        int sliceIndex,
        const BinaryChunkData* neighborXMinus,
        const BinaryChunkData* neighborXPlus,
        const BinaryChunkData* neighborYMinus,
        const BinaryChunkData* neighborYPlus,
        const BinaryChunkData* neighborZMinus,
        const BinaryChunkData* neighborZPlus
    );
    
private:
    
    // Extract 2D slice from 3D bit mask for a specific face direction
    static std::vector<SliceMask> extractSlices(
        const BinaryChunkData::BlockMask& blockMask,
        int faceDirection
    );
    
    // Perform greedy meshing on a 2D slice
    static std::vector<MeshQuad> meshSlice(
        const SliceMask& slice,
        int sliceIndex,
        int faceDirection,
        BlockType blockType
    );
    
    // Try to expand a quad horizontally
    static int expandHorizontal(
        SliceMask& slice,
        int startX, int startY, int width, int height
    );
    
    // Try to expand a quad vertically
    static int expandVertical(
        SliceMask& slice,
        int startX, int startY, int width, int height
    );
    
    // Clear a rectangular region in the slice
    static void clearRect(
        SliceMask& slice,
        int x, int y, int width, int height
    );
    
    // Convert 2D coordinates to 1D index for slice
    static constexpr int coords2D(int x, int y) {
        return x + y * BinaryChunkData::CHUNK_SIZE;
    }
};

} // namespace Zerith