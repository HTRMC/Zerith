#pragma once

#include "binary_chunk_data.h"
#include "blockbench_instance_generator.h"
#include "block_registry.h"
#include <vector>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Zerith {

/**
 * Converts binary greedy mesh quads back to the existing FaceInstance format.
 * This allows the optimized binary meshing to integrate with the current rendering system.
 */
class BinaryMeshConverter {
public:
    using FaceInstance = BlockbenchInstanceGenerator::FaceInstance;
    using MeshQuad = BinaryGreedyMesher::MeshQuad;
    
    /**
     * Convert a single mesh quad to face instances.
     * May generate multiple face instances if the block type has complex geometry.
     */
    static std::vector<FaceInstance> convertQuadToFaces(
        const MeshQuad& quad,
        const glm::ivec3& chunkWorldPos,
        const BlockRegistry& blockRegistry
    );
    
    /**
     * Convert all mesh quads to face instances for rendering.
     */
    static std::vector<FaceInstance> convertAllQuads(
        const std::vector<MeshQuad>& quads,
        const glm::ivec3& chunkWorldPos,
        const BlockRegistry& blockRegistry
    );
    
    /**
     * Get UV coordinates for a quad based on its size and block texture.
     * Handles texture scaling for merged faces.
     */
    static glm::vec4 calculateQuadUV(
        const MeshQuad& quad,
        const BlockDefinition& blockDef,
        int faceDirection
    );
    
    /**
     * Get texture information for a block type and face direction.
     */
    static std::string getBlockTexture(
        BlockType blockType,
        int faceDirection,
        const BlockRegistry& blockRegistry
    );
    
    /**
     * Calculate world position for a quad face.
     */
    static glm::vec3 calculateQuadWorldPosition(
        const MeshQuad& quad,
        const glm::ivec3& chunkWorldPos
    );
    
    /**
     * Calculate scale for a quad face based on its dimensions.
     */
    static glm::vec3 calculateQuadScale(const MeshQuad& quad);
    
    /**
     * Get the rotation quaternion for a face direction.
     */
    static glm::vec4 getFaceRotation(int faceDirection);
    
private:
    /**
     * Handle texture tiling for large merged faces.
     * Ensures textures tile properly across the merged surface.
     */
    static glm::vec4 adjustUVForTiling(
        const glm::vec4& baseUV,
        const glm::ivec3& quadSize,
        int faceDirection
    );
    
    /**
     * Get the default UV coordinates for a block face.
     */
    static glm::vec4 getDefaultFaceUV();
};

/**
 * Integration class that combines binary meshing with the existing chunk mesh generator.
 */
class HybridChunkMeshGenerator {
public:
    using FaceInstance = BlockbenchInstanceGenerator::FaceInstance;
    
    /**
     * Generate mesh using binary greedy meshing for simple blocks
     * and fall back to traditional meshing for complex blocks.
     */
    static std::vector<FaceInstance> generateOptimizedMesh(
        const Chunk& chunk,
        const glm::ivec3& chunkWorldPos,
        const BlockRegistry& blockRegistry
    );
    
private:
    /**
     * Determine if a block type can use binary meshing.
     * Simple full-cube blocks can use binary meshing,
     * while complex blocks (stairs, slabs) need traditional meshing.
     */
    static bool canUseBinaryMeshing(
        BlockType blockType,
        const BlockRegistry& blockRegistry
    );
    
    /**
     * Generate mesh for complex blocks using traditional method.
     */
    static std::vector<FaceInstance> generateComplexBlockMesh(
        const Chunk& chunk,
        const glm::ivec3& chunkWorldPos,
        const BlockRegistry& blockRegistry,
        const std::vector<BlockType>& complexBlockTypes
    );
};

} // namespace Zerith