#pragma once

#include "blockbench_model.h"
#include "blockbench_instance_generator.h"
#include "texture_array.h"
#include "chunk.h"
#include "logger.h"
#include "face_instance_pool.h"
#include "blocks.h"
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include <algorithm>

// Wrapper class to generate instances at specific positions
class BlockbenchInstanceWrapper
{
public:
    explicit BlockbenchInstanceWrapper(BlockbenchModel::Model model, Zerith::BlockType blockType,
                                       const std::shared_ptr<Zerith::TextureArray>& textureArray)
        : m_model(std::move(model)), m_blockType(blockType), m_textureArray(textureArray)
    {
        // Pre-generate base instances at origin
        m_baseInstances = BlockbenchInstanceGenerator::Generator::generateModelInstances(m_model);
        
        // Remove overlay faces for grass blocks - they'll be handled in the shader
        if (m_blockType == Zerith::Blocks::GRASS_BLOCK) {
            // Get the overlay texture layer from the texture array
            std::string overlayPath = "assets/zerith/textures/block/grass_block_side_overlay.png";
            uint32_t overlayLayer = m_textureArray->getTextureLayerByPath(overlayPath);
            
            auto it = std::remove_if(m_baseInstances.faces.begin(), m_baseInstances.faces.end(),
                [overlayLayer](const BlockbenchInstanceGenerator::FaceInstance& face) {
                    return face.textureLayer == overlayLayer;
                });
            m_baseInstances.faces.erase(it, m_baseInstances.faces.end());
        }
    }

    // Generate face instances at a specific world position (legacy method)
    std::vector<BlockbenchInstanceGenerator::FaceInstance> generateInstancesAtPosition(const glm::vec3& position) const
    {
        std::vector<BlockbenchInstanceGenerator::FaceInstance> instances;
        instances.reserve(m_baseInstances.faces.size());

        // Copy base instances and offset by position using emplace_back
        for (const auto& face : m_baseInstances.faces)
        {
            instances.emplace_back(
                face.position + position,  // position
                face.rotation,             // rotation
                face.scale,                // scale
                face.faceDirection,        // faceDirection
                face.uv,                   // uv
                face.textureLayer          // textureLayer
            );
        }

        return instances;
    }

    // Generate face instances at a specific world position using object pooling
    void generateInstancesAtPositionPooled(const glm::vec3& position, FaceInstancePool::FaceInstanceBatch& batch) const
    {
        batch.reserve(batch.size() + m_baseInstances.faces.size());

        // Copy base instances and offset by position
        for (const auto& face : m_baseInstances.faces)
        {
            batch.addFace(
                face.position + position,  // position
                face.rotation,             // rotation
                face.scale,                // scale
                face.faceDirection,        // faceDirection
                face.uv,                   // uv
                face.textureLayer          // textureLayer
            );
        }
    }

private:
    BlockbenchModel::Model m_model;
    BlockbenchInstanceGenerator::ModelInstances m_baseInstances;
    Zerith::BlockType m_blockType;
    std::shared_ptr<Zerith::TextureArray> m_textureArray;
};
