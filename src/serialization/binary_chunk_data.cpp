#include "binary_chunk_data.h"
#include <algorithm>
#include "blockbench_parser.h"
#include "blockbench_face_extractor.h"
#include "block_face_bounds.h"

namespace Zerith {

// Initialize empty mask
const BinaryChunkData::BlockMask BinaryChunkData::s_emptyMask;

BinaryChunkData::BinaryChunkData(const Chunk& chunk) {
    // Scan through all blocks in the chunk and build bit masks
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                BlockType blockType = chunk.getBlock(x, y, z);
                
                // Skip air blocks
                if (blockType == Blocks::AIR) continue;
                
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

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::generateQuadsWithBounds(
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
        
        // Use enhanced meshing with bounds awareness
        auto sliceQuads = meshSliceWithBounds(visibleMask, chunkData, sliceIndex, faceDirection, blockType);
        quads.insert(quads.end(), sliceQuads.begin(), sliceQuads.end());
    }
    
    return quads;
}

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::generateQuadsMultiElement(
    const BinaryChunkData& chunkData,
    BlockType blockType,
    int faceDirection
) {
    const auto& blockMask = chunkData.getBlockMask(blockType);
    if (blockMask.none()) {
        return {};
    }
    
    // Load the block model to get element information
    auto blockDef = Blocks::getBlock(blockType);
    if (!blockDef) {
        return {};
    }
    
    std::string modelPath = "assets/zerith/models/block/" + blockDef->getModelName() + ".json";
    
    try {
        auto model = BlockbenchParser::parseFromFileWithParents(modelPath, nullptr);
        
        std::vector<MeshQuad> allQuads;
        
        // Generate quads for each element separately
        for (size_t elementIndex = 0; elementIndex < model.elements.size(); ++elementIndex) {
            const auto& element = model.elements[elementIndex];
            
            // Get face bounds for this specific element
            auto elementBounds = BlockbenchFaceExtractor::extractFaceBounds(element, faceDirection);
            
            // Check if this element has a face in this direction
            const BlockbenchModel::Face* face = nullptr;
            switch (faceDirection) {
                case 0: face = &element.down; break;
                case 1: face = &element.up; break;
                case 2: face = &element.north; break;
                case 3: face = &element.south; break;
                case 4: face = &element.west; break;
                case 5: face = &element.east; break;
            }
            
            // Skip if this element doesn't have a face in this direction
            if (!face || face->texture.empty()) {
                continue;
            }
            
            // Generate quads for this element using enhanced bounds-aware meshing
            auto slices = extractSlices(blockMask, faceDirection);
            
            for (int sliceIndex = 0; sliceIndex < static_cast<int>(slices.size()); ++sliceIndex) {
                // Generate visible face mask for this slice
                auto visibleMask = generateVisibleFaceMask(chunkData, blockType, faceDirection, sliceIndex);
                
                // Use enhanced meshing with this element's specific bounds
                auto sliceQuads = meshSliceWithElementBounds(visibleMask, chunkData, sliceIndex, 
                                                           faceDirection, blockType, elementBounds, 
                                                           static_cast<int>(elementIndex),
                                                           element.from, element.to);
                allQuads.insert(allQuads.end(), sliceQuads.begin(), sliceQuads.end());
            }
        }
        
        return allQuads;
        
    } catch (const std::exception& e) {
        // If we can't load the model, fall back to regular bounds-aware meshing
        return generateQuadsWithBounds(chunkData, blockType, faceDirection);
    }
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
            quad.elementIndex = -1; // Single element block
            quad.elementOffset = glm::vec3(0.0f); // No offset for single element blocks
            quad.elementSize = glm::vec3(1.0f); // Full block size for single element blocks
            
            // Set face bounds for this quad
            const auto& registry = BlockFaceBoundsRegistry::getInstance();
            const auto& blockBounds = registry.getFaceBounds(blockType);
            quad.faceBounds = blockBounds.faces[faceDirection];
            
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

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::meshSliceWithBounds(
    const SliceMask& slice,
    const BinaryChunkData& chunkData,
    int sliceIndex,
    int faceDirection,
    BlockType blockType
) {
    constexpr int SIZE = BinaryChunkData::CHUNK_SIZE;
    std::vector<MeshQuad> quads;
    auto workingSlice = slice; // Copy for modification
    
    int originalBits = static_cast<int>(slice.count());
    if (originalBits == 0) return quads; // No visible faces in this slice
    
    // Enhanced greedy meshing algorithm with bounds awareness
    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            if (!workingSlice.test(coords2D(x, y))) {
                continue; // No block at this position
            }
            
            // Find the width of the quad by expanding horizontally with bounds checking
            int width = expandHorizontalWithBounds(
                workingSlice, chunkData, faceDirection, sliceIndex, x, y, 1, 1, blockType
            );
            
            // Find the height of the quad by expanding vertically with bounds checking
            int height = expandVerticalWithBounds(
                workingSlice, chunkData, faceDirection, sliceIndex, x, y, width, 1, blockType
            );
            
            // Create the quad
            MeshQuad quad;
            quad.blockType = blockType;
            quad.faceDirection = faceDirection;
            quad.elementIndex = -1; // Single element block
            quad.elementOffset = glm::vec3(0.0f); // No offset for single element blocks
            quad.elementSize = glm::vec3(1.0f); // Full block size for single element blocks
            
            // Set face bounds for this quad
            const auto& registry = BlockFaceBoundsRegistry::getInstance();
            const auto& blockBounds = registry.getFaceBounds(blockType);
            quad.faceBounds = blockBounds.faces[faceDirection];
            
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

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::meshSliceWithElementBounds(
    const SliceMask& slice,
    const BinaryChunkData& chunkData,
    int sliceIndex,
    int faceDirection,
    BlockType blockType,
    const FaceBounds& elementBounds,
    int elementIndex,
    const glm::vec3& elementFrom,
    const glm::vec3& elementTo
) {
    constexpr int SIZE = BinaryChunkData::CHUNK_SIZE;
    std::vector<MeshQuad> quads;
    auto workingSlice = slice; // Copy for modification
    
    int originalBits = static_cast<int>(slice.count());
    if (originalBits == 0) return quads; // No visible faces in this slice
    
    // Enhanced greedy meshing algorithm with element-specific bounds
    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            if (!workingSlice.test(coords2D(x, y))) {
                continue; // No block at this position
            }
            
            // Find the width of the quad by expanding horizontally with bounds checking
            int width = expandHorizontalWithBounds(
                workingSlice, chunkData, faceDirection, sliceIndex, x, y, 1, 1, blockType
            );
            
            // Find the height of the quad by expanding vertically with bounds checking
            int height = expandVerticalWithBounds(
                workingSlice, chunkData, faceDirection, sliceIndex, x, y, width, 1, blockType
            );
            
            // Create the quad
            MeshQuad quad;
            quad.blockType = blockType;
            quad.faceDirection = faceDirection;
            quad.elementIndex = elementIndex; // Set the specific element index
            
            // Use the provided element bounds instead of block bounds
            quad.faceBounds = elementBounds;
            
            // Calculate element offset and size (convert from Blockbench coordinates 0-16 to normalized 0-1)
            quad.elementOffset = elementFrom / 16.0f;
            quad.elementSize = (elementTo - elementFrom) / 16.0f;
            
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

BinaryGreedyMesher::SliceMask BinaryGreedyMesher::generateVisibleFaceMaskWithNeighbors(
    const BinaryChunkData& chunkData,
    BlockType blockType,
    int faceDirection,
    int sliceIndex,
    const BinaryChunkData* neighborXMinus,
    const BinaryChunkData* neighborXPlus,
    const BinaryChunkData* neighborYMinus,
    const BinaryChunkData* neighborYPlus,
    const BinaryChunkData* neighborZMinus,
    const BinaryChunkData* neighborZPlus,
    const Chunk* neighborChunkXMinus,
    const Chunk* neighborChunkXPlus,
    const Chunk* neighborChunkYMinus,
    const Chunk* neighborChunkYPlus,
    const Chunk* neighborChunkZMinus,
    const Chunk* neighborChunkZPlus
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
            } else {
                // Adjacent position is outside chunk bounds - check neighbor chunks
                const BinaryChunkData* neighborBinaryChunk = nullptr;
                const Chunk* neighborChunk = nullptr;
                glm::ivec3 neighborPos = adjPos;
                
                // Determine which neighbor chunk and adjust coordinates
                if (adjPos.x < 0) {
                    neighborBinaryChunk = neighborXMinus;
                    neighborChunk = neighborChunkXMinus;
                    neighborPos.x = SIZE - 1;
                } else if (adjPos.x >= SIZE) {
                    neighborBinaryChunk = neighborXPlus;
                    neighborChunk = neighborChunkXPlus;
                    neighborPos.x = 0;
                } else if (adjPos.y < 0) {
                    neighborBinaryChunk = neighborYMinus;
                    neighborChunk = neighborChunkYMinus;
                    neighborPos.y = SIZE - 1;
                } else if (adjPos.y >= SIZE) {
                    neighborBinaryChunk = neighborYPlus;
                    neighborChunk = neighborChunkYPlus;
                    neighborPos.y = 0;
                } else if (adjPos.z < 0) {
                    neighborBinaryChunk = neighborZMinus;
                    neighborChunk = neighborChunkZMinus;
                    neighborPos.z = SIZE - 1;
                } else if (adjPos.z >= SIZE) {
                    neighborBinaryChunk = neighborZPlus;
                    neighborChunk = neighborChunkZPlus;
                    neighborPos.z = 0;
                }
                
                // If no neighbor chunk exists, face is visible (edge of world)
                if (!neighborBinaryChunk && !neighborChunk) {
                    faceVisible = true;
                } else {
                    faceVisible = true;
                    
                    // First try to use binary chunk data for efficiency
                    if (neighborBinaryChunk) {
                        for (BlockType adjacentBlockType : neighborBinaryChunk->getActiveBlockTypes()) {
                            if (neighborBinaryChunk->hasBlockAt(neighborPos.x, neighborPos.y, neighborPos.z, adjacentBlockType)) {
                                faceVisible = false; // Any solid block adjacent, face is hidden
                                break;
                            }
                        }
                    } 
                    // Fallback to checking the actual chunk data (for mixed meshing scenarios)
                    else if (neighborChunk) {
                        BlockType adjacentBlock = neighborChunk->getBlock(neighborPos.x, neighborPos.y, neighborPos.z);
                        // Use proper culling logic that handles water and other complex blocks
                        faceVisible = isFaceVisibleAgainstNeighbor(blockType, faceDirection, adjacentBlock);
                    }
                }
            }
            
            if (faceVisible) {
                visibleMask.set(BinaryGreedyMesher::coords2D(u, v));
            }
        }
    }
    
    return visibleMask;
}

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::generateQuadsWithNeighbors(
    const BinaryChunkData& chunkData,
    BlockType blockType,
    int faceDirection,
    const BinaryChunkData* neighborXMinus,
    const BinaryChunkData* neighborXPlus,
    const BinaryChunkData* neighborYMinus,
    const BinaryChunkData* neighborYPlus,
    const BinaryChunkData* neighborZMinus,
    const BinaryChunkData* neighborZPlus,
    const Chunk* neighborChunkXMinus,
    const Chunk* neighborChunkXPlus,
    const Chunk* neighborChunkYMinus,
    const Chunk* neighborChunkYPlus,
    const Chunk* neighborChunkZMinus,
    const Chunk* neighborChunkZPlus
) {
    const auto& blockMask = chunkData.getBlockMask(blockType);
    if (blockMask.none()) {
        return {};
    }
    
    auto slices = extractSlices(blockMask, faceDirection);
    std::vector<MeshQuad> quads;
    
    for (int sliceIndex = 0; sliceIndex < static_cast<int>(slices.size()); ++sliceIndex) {
        // Generate visible face mask with neighbor data for this slice
        auto visibleMask = generateVisibleFaceMaskWithNeighbors(
            chunkData, blockType, faceDirection, sliceIndex,
            neighborXMinus, neighborXPlus, neighborYMinus, neighborYPlus, neighborZMinus, neighborZPlus,
            neighborChunkXMinus, neighborChunkXPlus, neighborChunkYMinus, neighborChunkYPlus, neighborChunkZMinus, neighborChunkZPlus
        );
        
        // Only mesh visible faces
        auto sliceQuads = meshSlice(visibleMask, sliceIndex, faceDirection, blockType);
        quads.insert(quads.end(), sliceQuads.begin(), sliceQuads.end());
    }
    
    return quads;
}

std::vector<BinaryGreedyMesher::MeshQuad> BinaryGreedyMesher::generateAllQuadsWithNeighbors(
    const BinaryChunkData& chunkData,
    const BinaryChunkData* neighborXMinus,
    const BinaryChunkData* neighborXPlus,
    const BinaryChunkData* neighborYMinus,
    const BinaryChunkData* neighborYPlus,
    const BinaryChunkData* neighborZMinus,
    const BinaryChunkData* neighborZPlus,
    const Chunk* neighborChunkXMinus,
    const Chunk* neighborChunkXPlus,
    const Chunk* neighborChunkYMinus,
    const Chunk* neighborChunkYPlus,
    const Chunk* neighborChunkZMinus,
    const Chunk* neighborChunkZPlus
) {
    std::vector<MeshQuad> allQuads;
    
    // For each active block type
    for (BlockType blockType : chunkData.getActiveBlockTypes()) {
        // For each face direction (0=down, 1=up, 2=north, 3=south, 4=west, 5=east)
        for (int faceDir = 0; faceDir < 6; ++faceDir) {
            auto quads = generateQuadsWithNeighbors(
                chunkData, blockType, faceDir,
                neighborXMinus, neighborXPlus, neighborYMinus, neighborYPlus, neighborZMinus, neighborZPlus,
                neighborChunkXMinus, neighborChunkXPlus, neighborChunkYMinus, neighborChunkYPlus, neighborChunkZMinus, neighborChunkZPlus
            );
            allQuads.insert(allQuads.end(), quads.begin(), quads.end());
        }
    }
    
    return allQuads;
}

bool BinaryGreedyMesher::isFaceVisibleAgainstNeighbor(
    BlockType currentBlockType,
    int currentFaceDirection,
    BlockType neighborBlockType
) {
    // If neighbor is air, face is always visible
    if (neighborBlockType == Blocks::AIR) {
        return true;
    }
    
    // Get block definitions
    auto currentBlockDef = Blocks::getBlock(currentBlockType);
    auto neighborBlockDef = Blocks::getBlock(neighborBlockType);
    
    if (!currentBlockDef || !neighborBlockDef) {
        return true; // Default to visible if we can't get block info
    }
    
    // Check if neighbor is liquid (water) by checking render layer
    RenderLayer neighborRenderLayer = neighborBlockDef->getRenderLayer();
    RenderLayer currentRenderLayer = currentBlockDef->getRenderLayer();
    
    // Special handling for liquid blocks (like water)
    if (neighborRenderLayer == RenderLayer::TRANSLUCENT) {
        // Check if this is actually a liquid by checking if it's water
        if (neighborBlockDef->getId() == "water") {
            // Solid blocks next to water are always visible
            if (currentRenderLayer != RenderLayer::TRANSLUCENT) {
                return true;
            }
        }
    }
    
    // Transparent/translucent blocks don't cull faces behind them
    if (neighborRenderLayer == RenderLayer::TRANSLUCENT || neighborRenderLayer == RenderLayer::CUTOUT) {
        return true;
    }
    
    // If neighbor block is solid and opaque, face is hidden
    return false;
}

bool BinaryGreedyMesher::canMergeFaces(
    BlockType blockType1,
    BlockType blockType2,
    int faceDirection
) {
    // Different block types can't be merged
    if (blockType1 != blockType2) {
        return false;
    }
    
    // Get block definitions
    auto blockDef1 = Blocks::getBlock(blockType1);
    auto blockDef2 = Blocks::getBlock(blockType2);
    
    if (!blockDef1 || !blockDef2) {
        return false;
    }
    
    // Allow merging of all blocks including translucent ones
    
    // Get face bounds for both blocks
    const auto& registry = BlockFaceBoundsRegistry::getInstance();
    const auto& bounds1 = registry.getFaceBounds(blockType1);
    const auto& bounds2 = registry.getFaceBounds(blockType2);
    
    // Check if face bounds are identical (epsilon comparison)
    const FaceBounds& face1 = bounds1.faces[faceDirection];
    const FaceBounds& face2 = bounds2.faces[faceDirection];
    
    constexpr float epsilon = 0.001f;
    
    bool boundsMatch = (std::abs(face1.min.x - face2.min.x) < epsilon &&
                       std::abs(face1.min.y - face2.min.y) < epsilon &&
                       std::abs(face1.max.x - face2.max.x) < epsilon &&
                       std::abs(face1.max.y - face2.max.y) < epsilon);
    
    return boundsMatch;
}

BlockType BinaryGreedyMesher::getBlockTypeAtSlicePosition(
    const BinaryChunkData& chunkData,
    int faceDirection,
    int sliceIndex,
    int u, int v
) {
    // Convert slice coordinates to world coordinates
    glm::ivec3 worldPos;
    switch (faceDirection) {
        case 0: // Down (Y- faces, slice along XZ planes)
        case 1: // Up (Y+ faces, slice along XZ planes)
            worldPos = glm::ivec3(u, sliceIndex, v);
            break;
        case 2: // North (Z- faces, slice along XY planes)
        case 3: // South (Z+ faces, slice along XY planes)
            worldPos = glm::ivec3(u, v, sliceIndex);
            break;
        case 4: // West (X- faces, slice along YZ planes)
        case 5: // East (X+ faces, slice along YZ planes)
            worldPos = glm::ivec3(sliceIndex, u, v);
            break;
    }
    
    // Check each active block type to find which one is at this position
    for (BlockType blockType : chunkData.getActiveBlockTypes()) {
        if (chunkData.hasBlockAt(worldPos.x, worldPos.y, worldPos.z, blockType)) {
            return blockType;
        }
    }
    
    return Blocks::AIR;
}

int BinaryGreedyMesher::expandHorizontalWithBounds(
    SliceMask& slice,
    const BinaryChunkData& chunkData,
    int faceDirection,
    int sliceIndex,
    int startX, int startY, int width, int height,
    BlockType blockType
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
            
            // Check bounds compatibility
            BlockType neighborBlockType = getBlockTypeAtSlicePosition(
                chunkData, faceDirection, sliceIndex, startX + width, y
            );
            
            if (!canMergeFaces(blockType, neighborBlockType, faceDirection)) {
                canExpand = false;
                break;
            }
        }
        
        if (!canExpand) break;
        width++;
    }
    
    return width;
}

int BinaryGreedyMesher::expandVerticalWithBounds(
    SliceMask& slice,
    const BinaryChunkData& chunkData,
    int faceDirection,
    int sliceIndex,
    int startX, int startY, int width, int height,
    BlockType blockType
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
            
            // Check bounds compatibility
            BlockType neighborBlockType = getBlockTypeAtSlicePosition(
                chunkData, faceDirection, sliceIndex, x, startY + height
            );
            
            if (!canMergeFaces(blockType, neighborBlockType, faceDirection)) {
                canExpand = false;
                break;
            }
        }
        
        if (!canExpand) break;
        height++;
    }
    
    return height;
}

} // namespace Zerith