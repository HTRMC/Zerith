#include "binary_chunk_data.h"
#include <algorithm>

namespace Zerith {

// Initialize empty mask
const BinaryChunkData::BlockMask BinaryChunkData::s_emptyMask;

BinaryChunkData::BinaryChunkData(const Chunk& chunk) {
    // Scan through all blocks in the chunk and build bit masks
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                BlockType blockType = chunk.getBlock(x, y, z);
                
                // Skip air blocks (assuming air is block type 0)
                if (blockType == 0) continue;
                
                int index = coordsToIndex(x, y, z);
                
                // Set the bit for this block type at this position
                m_blockMasks[blockType].set(index);
            }
        }
    }
    
    // Build cache of active block types
    m_activeBlockTypes.reserve(m_blockMasks.size());
    for (const auto& pair : m_blockMasks) {
        m_activeBlockTypes.push_back(pair.first);
    }
    
    // Sort for consistent iteration order
    std::sort(m_activeBlockTypes.begin(), m_activeBlockTypes.end());
}

const BinaryChunkData::BlockMask& BinaryChunkData::getBlockMask(BlockType blockType) const {
    auto it = m_blockMasks.find(blockType);
    return (it != m_blockMasks.end()) ? it->second : s_emptyMask;
}

bool BinaryChunkData::hasBlockType(BlockType blockType) const {
    return m_blockMasks.find(blockType) != m_blockMasks.end();
}

const std::vector<BlockType>& BinaryChunkData::getActiveBlockTypes() const {
    return m_activeBlockTypes;
}

bool BinaryChunkData::hasBlockAt(int x, int y, int z, BlockType blockType) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
        return false;
    }
    
    auto it = m_blockMasks.find(blockType);
    if (it == m_blockMasks.end()) {
        return false;
    }
    
    int index = coordsToIndex(x, y, z);
    return it->second.test(index);
}

// Binary Greedy Mesher Implementation

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::generateQuads(
    const BinaryChunkData& chunkData,
    BlockType blockType,
    int faceDirection
) {
    const auto& blockMask = chunkData.getBlockMask(blockType);
    if (blockMask.none()) {
        return {};
    }
    
    auto slices = extractSlices(blockMask, faceDirection);
    std::vector<MeshQuad> quads;
    
    for (int sliceIndex = 0; sliceIndex < static_cast<int>(slices.size()); ++sliceIndex) {
        // Generate visible face mask for this slice
        auto visibleMask = generateVisibleFaceMask(chunkData, blockType, faceDirection, sliceIndex);
        
        // Only mesh visible faces
        auto sliceQuads = meshSlice(visibleMask, sliceIndex, faceDirection, blockType);
        quads.insert(quads.end(), sliceQuads.begin(), sliceQuads.end());
    }
    
    return quads;
}

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::generateAllQuads(
    const BinaryChunkData& chunkData
) {
    std::vector<MeshQuad> allQuads;
    
    // For each active block type
    for (BlockType blockType : chunkData.getActiveBlockTypes()) {
        // For each face direction (0=down, 1=up, 2=north, 3=south, 4=west, 5=east)
        for (int faceDir = 0; faceDir < 6; ++faceDir) {
            auto quads = generateQuads(chunkData, blockType, faceDir);
            allQuads.insert(allQuads.end(), quads.begin(), quads.end());
        }
    }
    
    return allQuads;
}

std::vector<BinaryGreedyMesher::SliceMask> 
BinaryGreedyMesher::extractSlices(
    const BinaryChunkData::BlockMask& blockMask,
    int faceDirection
) {
    constexpr int SIZE = BinaryChunkData::CHUNK_SIZE;
    std::vector<SliceMask> slices;
    
    // Extract slices based on face direction
    switch (faceDirection) {
        case 0: // Down (Y- faces, slice along XZ planes)
        case 1: // Up (Y+ faces, slice along XZ planes)
            slices.resize(SIZE);
            for (int y = 0; y < SIZE; ++y) {
                for (int x = 0; x < SIZE; ++x) {
                    for (int z = 0; z < SIZE; ++z) {
                        int blockIndex = BinaryChunkData::coordsToIndex(x, y, z);
                        if (blockMask.test(blockIndex)) {
                            slices[y].set(coords2D(x, z));
                        }
                    }
                }
            }
            break;
            
        case 2: // North (Z- faces, slice along XY planes)
        case 3: // South (Z+ faces, slice along XY planes)
            slices.resize(SIZE);
            for (int z = 0; z < SIZE; ++z) {
                for (int x = 0; x < SIZE; ++x) {
                    for (int y = 0; y < SIZE; ++y) {
                        int blockIndex = BinaryChunkData::coordsToIndex(x, y, z);
                        if (blockMask.test(blockIndex)) {
                            slices[z].set(coords2D(x, y));
                        }
                    }
                }
            }
            break;
            
        case 4: // West (X- faces, slice along YZ planes)
        case 5: // East (X+ faces, slice along YZ planes)
            slices.resize(SIZE);
            for (int x = 0; x < SIZE; ++x) {
                for (int y = 0; y < SIZE; ++y) {
                    for (int z = 0; z < SIZE; ++z) {
                        int blockIndex = BinaryChunkData::coordsToIndex(x, y, z);
                        if (blockMask.test(blockIndex)) {
                            slices[x].set(coords2D(y, z));
                        }
                    }
                }
            }
            break;
    }
    
    return slices;
}

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::meshSlice(
    const SliceMask& slice,
    int sliceIndex,
    int faceDirection,
    BlockType blockType
) {
    constexpr int SIZE = BinaryChunkData::CHUNK_SIZE;
    std::vector<MeshQuad> quads;
    auto workingSlice = slice; // Copy for modification
    
    int originalBits = static_cast<int>(slice.count());
    if (originalBits == 0) return quads; // No visible faces in this slice
    
    // Greedy meshing algorithm
    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            if (!workingSlice.test(coords2D(x, y))) {
                continue; // No block at this position
            }
            
            // Find the width of the quad by expanding horizontally
            int width = expandHorizontal(workingSlice, x, y, 1, 1);
            
            // Find the height of the quad by expanding vertically
            int height = expandVertical(workingSlice, x, y, width, 1);
            
            // Create the quad
            MeshQuad quad;
            quad.blockType = blockType;
            quad.faceDirection = faceDirection;
            
            // Set position and size based on face direction
            // Note: (x,y) in the slice corresponds to different world coordinates based on face direction
            switch (faceDirection) {
                case 0: // Down (Y- faces, sliced along XZ planes)
                case 1: // Up (Y+ faces, sliced along XZ planes)
                    // x,y in slice = x,z in world coords
                    quad.position = glm::ivec3(x, sliceIndex, y);
                    quad.size = glm::ivec3(width, 1, height);
                    break;
                case 2: // North (Z- faces, sliced along XY planes)
                case 3: // South (Z+ faces, sliced along XY planes)
                    // x,y in slice = x,y in world coords
                    quad.position = glm::ivec3(x, y, sliceIndex);
                    quad.size = glm::ivec3(width, height, 1);
                    break;
                case 4: // West (X- faces, sliced along YZ planes)
                case 5: // East (X+ faces, sliced along YZ planes)
                    // x,y in slice = y,z in world coords
                    quad.position = glm::ivec3(sliceIndex, x, y);
                    quad.size = glm::ivec3(1, width, height);
                    break;
            }
            
            quads.push_back(quad);
            
            // Clear the processed rectangle
            clearRect(workingSlice, x, y, width, height);
        }
    }
    
    
    return quads;
}

int BinaryGreedyMesher::expandHorizontal(
    SliceMask& slice,
    int startX, int startY, int width, int height
) {
    constexpr int SIZE = BinaryChunkData::CHUNK_SIZE;
    
    // Check if we can expand to the right
    while (startX + width < SIZE) {
        bool canExpand = true;
        
        // Check the new column
        for (int y = startY; y < startY + height; ++y) {
            if (!slice.test(coords2D(startX + width, y))) {
                canExpand = false;
                break;
            }
        }
        
        if (!canExpand) break;
        width++;
    }
    
    return width;
}

int BinaryGreedyMesher::expandVertical(
    SliceMask& slice,
    int startX, int startY, int width, int height
) {
    constexpr int SIZE = BinaryChunkData::CHUNK_SIZE;
    
    // Check if we can expand upward
    while (startY + height < SIZE) {
        bool canExpand = true;
        
        // Check the new row
        for (int x = startX; x < startX + width; ++x) {
            if (!slice.test(coords2D(x, startY + height))) {
                canExpand = false;
                break;
            }
        }
        
        if (!canExpand) break;
        height++;
    }
    
    return height;
}

void BinaryGreedyMesher::clearRect(
    SliceMask& slice,
    int x, int y, int width, int height
) {
    for (int dy = 0; dy < height; ++dy) {
        for (int dx = 0; dx < width; ++dx) {
            slice.reset(coords2D(x + dx, y + dy));
        }
    }
}

BinaryGreedyMesher::SliceMask BinaryGreedyMesher::generateVisibleFaceMask(
    const BinaryChunkData& chunkData,
    BlockType blockType,
    int faceDirection,
    int sliceIndex
) {
    constexpr int SIZE = BinaryChunkData::CHUNK_SIZE;
    BinaryGreedyMesher::SliceMask visibleMask;
    
    // Get face normal direction offset
    glm::ivec3 faceOffset(0);
    switch (faceDirection) {
        case 0: faceOffset = glm::ivec3(0, -1, 0); break; // Down
        case 1: faceOffset = glm::ivec3(0, 1, 0);  break; // Up
        case 2: faceOffset = glm::ivec3(0, 0, -1); break; // North
        case 3: faceOffset = glm::ivec3(0, 0, 1);  break; // South
        case 4: faceOffset = glm::ivec3(-1, 0, 0); break; // West
        case 5: faceOffset = glm::ivec3(1, 0, 0);  break; // East
    }
    
    // Check each position in the slice
    for (int u = 0; u < SIZE; ++u) {
        for (int v = 0; v < SIZE; ++v) {
            // Convert 2D slice coordinates to 3D chunk coordinates
            glm::ivec3 blockPos;
            switch (faceDirection) {
                case 0: // Down (Y- faces, slice along XZ planes)
                case 1: // Up (Y+ faces, slice along XZ planes)
                    blockPos = glm::ivec3(u, sliceIndex, v);
                    break;
                case 2: // North (Z- faces, slice along XY planes)
                case 3: // South (Z+ faces, slice along XY planes)
                    blockPos = glm::ivec3(u, v, sliceIndex);
                    break;
                case 4: // West (X- faces, slice along YZ planes)
                case 5: // East (X+ faces, slice along YZ planes)
                    blockPos = glm::ivec3(sliceIndex, u, v);
                    break;
            }
            
            // Check if this position has our block type
            if (!chunkData.hasBlockAt(blockPos.x, blockPos.y, blockPos.z, blockType)) {
                continue; // No block here
            }
            
            // Check if the adjacent position (in face direction) is empty or different block type
            glm::ivec3 adjPos = blockPos + faceOffset;
            bool faceVisible = true;
            
            // If adjacent position is within chunk bounds
            if (adjPos.x >= 0 && adjPos.x < SIZE && adjPos.y >= 0 && adjPos.y < SIZE && adjPos.z >= 0 && adjPos.z < SIZE) {
                // Face is visible if adjacent position has no solid block
                // Check all active block types to see if any occupy the adjacent position
                for (BlockType adjacentBlockType : chunkData.getActiveBlockTypes()) {
                    if (chunkData.hasBlockAt(adjPos.x, adjPos.y, adjPos.z, adjacentBlockType)) {
                        faceVisible = false; // Any solid block adjacent, face is hidden
                        break;
                    }
                }
            }
            // If adjacent position is outside chunk bounds, face is visible (chunk boundary)
            
            if (faceVisible) {
                visibleMask.set(BinaryGreedyMesher::coords2D(u, v));
            }
        }
    }
    
    
    return visibleMask;
}

} // namespace Zerith