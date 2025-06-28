#pragma once

#include "binary_chunk_data.h"
#include "blockbench_instance_generator.h"
#include "blockbench_model.h"
#include "blocks.h"
#include "texture_array.h"
#include "chunk.h"
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
        TextureArray& textureArray
    );
    
    /**
     * Convert all mesh quads to face instances for rendering.
     */
    static std::vector<FaceInstance> convertAllQuads(
        const std::vector<MeshQuad>& quads,
        const glm::ivec3& chunkWorldPos,
        TextureArray& textureArray
    );
    
    /**
     * Convert all mesh quads to face instances with AO calculation.
     */
    static std::vector<FaceInstance> convertAllQuadsWithAO(
        const std::vector<MeshQuad>& quads,
        const glm::ivec3& chunkWorldPos,
        const Chunk& chunk,
        TextureArray& textureArray
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
        int faceDirection
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

    /**
     * Get the default UV coordinates for a block face.
     */
    static glm::vec4 getDefaultFaceUV();
    
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
};

/**
 * Integration class that combines binary meshing with the existing chunk mesh generator.
 */
class HybridChunkMeshGenerator {
public:
    using FaceInstance = BlockbenchInstanceGenerator::FaceInstance;
    
    /**
     * Generate mesh using binary greedy meshing for simple blocks.
     * Returns std::nullopt if complex blocks are detected and traditional meshing should be used.
     */
    static std::optional<std::vector<FaceInstance>> generateOptimizedMesh(
        const Chunk& chunk,
        const glm::ivec3& chunkWorldPos,
        TextureArray& textureArray
    );
    
private:
    /**
     * Determine if a block type can use binary meshing.
     * Simple full-cube blocks can use binary meshing,
     * while complex blocks (stairs, slabs) need traditional meshing.
     */
    static bool canUseBinaryMeshing(
        BlockType blockType
    );
    
    /**
     * Generate mesh for complex blocks using traditional method.
     */
    static std::vector<FaceInstance> generateComplexBlockMesh(
        const Chunk& chunk,
        const glm::ivec3& chunkWorldPos,
        const Blocks& blocks,
        const std::vector<BlockType>& complexBlockTypes
    );
    
    /**
     * Check if a model represents a full cube (from 0,0,0 to 16,16,16).
     */
    static bool isFullCubeModel(const BlockbenchModel::Model& model);
};

} // namespace Zerith