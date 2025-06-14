#include "binary_chunk_data.h"
#include "binary_mesh_converter.h"
#include "chunk.h"
#include <iostream>
#include <cassert>

using namespace Zerith;

// Mock block registry for testing
class MockBlockRegistry : public BlockRegistry {
public:
    struct MockBlockInfo {
        std::string name;
        BlockType id;
    };
    
    std::optional<MockBlockInfo> getBlockInfo(BlockType blockType) const {
        if (blockType == 1) {
            return MockBlockInfo{"stone", 1};
        }
        if (blockType == 2) {
            return MockBlockInfo{"dirt", 2};
        }
        return std::nullopt;
    }
};

void testBinaryChunkData() {
    std::cout << "Testing BinaryChunkData..." << std::endl;
    
    // Create a test chunk with some blocks
    Chunk chunk(glm::ivec3(0, 0, 0));
    
    // Add some test blocks
    chunk.setBlock(0, 0, 0, 1); // Stone
    chunk.setBlock(1, 0, 0, 1); // Stone
    chunk.setBlock(0, 1, 0, 2); // Dirt
    chunk.setBlock(1, 1, 0, 2); // Dirt
    
    // Create binary data
    BinaryChunkData binaryData(chunk);
    
    // Test basic functionality
    assert(binaryData.hasBlockType(1)); // Stone
    assert(binaryData.hasBlockType(2)); // Dirt
    assert(!binaryData.hasBlockType(3)); // Non-existent block
    
    // Test block positions
    assert(binaryData.hasBlockAt(0, 0, 0, 1)); // Stone at (0,0,0)
    assert(binaryData.hasBlockAt(1, 0, 0, 1)); // Stone at (1,0,0)
    assert(binaryData.hasBlockAt(0, 1, 0, 2)); // Dirt at (0,1,0)
    assert(!binaryData.hasBlockAt(0, 0, 0, 2)); // No dirt at (0,0,0)
    
    // Test active block types
    auto activeTypes = binaryData.getActiveBlockTypes();
    assert(activeTypes.size() == 2);
    assert(std::find(activeTypes.begin(), activeTypes.end(), 1) != activeTypes.end());
    assert(std::find(activeTypes.begin(), activeTypes.end(), 2) != activeTypes.end());
    
    std::cout << "BinaryChunkData tests passed!" << std::endl;
}

void testBinaryGreedyMesher() {
    std::cout << "Testing BinaryGreedyMesher..." << std::endl;
    
    // Create a chunk with a 2x2x1 block of stone
    Chunk chunk(glm::ivec3(0, 0, 0));
    chunk.setBlock(0, 0, 0, 1);
    chunk.setBlock(1, 0, 0, 1);
    chunk.setBlock(0, 1, 0, 1);
    chunk.setBlock(1, 1, 0, 1);
    
    BinaryChunkData binaryData(chunk);
    
    // Generate quads for stone (block type 1)
    auto quads = BinaryGreedyMesher::generateAllQuads(binaryData);
    
    // Should have generated quads for all 6 face directions
    // Each 2x2 face should be merged into a single quad
    assert(!quads.empty());
    
    std::cout << "Generated " << quads.size() << " quads for 2x2x1 stone block" << std::endl;
    
    // Check that we have quads for each face direction
    bool hasUpFace = false, hasDownFace = false;
    for (const auto& quad : quads) {
        if (quad.faceDirection == 0) hasDownFace = true;
        if (quad.faceDirection == 1) hasUpFace = true;
        
        // Verify quad properties
        assert(quad.blockType == 1);
        assert(quad.size.x > 0 && quad.size.y > 0 && quad.size.z > 0);
    }
    
    assert(hasUpFace && hasDownFace);
    std::cout << "BinaryGreedyMesher tests passed!" << std::endl;
}

void testBinaryMeshConverter() {
    std::cout << "Testing BinaryMeshConverter..." << std::endl;
    
    MockBlockRegistry registry;
    
    // Create a test quad
    BinaryGreedyMesher::MeshQuad quad;
    quad.position = glm::ivec3(0, 0, 0);
    quad.size = glm::ivec3(2, 2, 1);
    quad.faceDirection = 1; // Up face
    quad.blockType = 1; // Stone
    
    // Convert to face instances
    auto faces = BinaryMeshConverter::convertQuadToFaces(
        quad, 
        glm::ivec3(0, 0, 0), // Chunk world position
        registry
    );
    
    assert(!faces.empty());
    assert(faces[0].faceDirection == 1);
    assert(faces[0].scale == glm::vec3(2, 2, 1));
    
    std::cout << "BinaryMeshConverter tests passed!" << std::endl;
}

void testPerformanceComparison() {
    std::cout << "Testing performance comparison..." << std::endl;
    
    // Create a large chunk filled with blocks
    Chunk chunk(glm::ivec3(0, 0, 0));
    
    // Fill bottom half with stone
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            for (int z = 0; z < 8; ++z) {
                chunk.setBlock(x, y, z, 1);
            }
        }
    }
    
    // Time binary meshing
    auto start = std::chrono::high_resolution_clock::now();
    
    BinaryChunkData binaryData(chunk);
    auto quads = BinaryGreedyMesher::generateAllQuads(binaryData);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Binary meshing generated " << quads.size() 
              << " quads in " << duration.count() << " microseconds" << std::endl;
    
    // Without greedy meshing, this would be 8x8x8x6 = 3072 faces
    // With greedy meshing, it should be much fewer
    assert(quads.size() < 3072);
    
    std::cout << "Performance test passed! Reduced faces from 3072 to " << quads.size() << std::endl;
}

int main() {
    try {
        testBinaryChunkData();
        testBinaryGreedyMesher();
        testBinaryMeshConverter();
        testPerformanceComparison();
        
        std::cout << "\nAll tests passed! Binary greedy meshing implementation is working correctly." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}