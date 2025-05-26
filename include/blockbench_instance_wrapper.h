#pragma once

#include "blockbench_model.h"
#include "blockbench_instance_generator.h"
#include "texture_array.h"
#include "chunk.h"
#include "logger.h"
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

        // Assign texture layers based on block type
        assignTextureLayers();
    }

    // Generate face instances at a specific world position
    std::vector<BlockbenchInstanceGenerator::FaceInstance> generateInstancesAtPosition(const glm::vec3& position)
    {
        std::vector<BlockbenchInstanceGenerator::FaceInstance> instances;
        instances.reserve(m_baseInstances.faces.size());

        // Copy base instances and offset by position
        for (const auto& face : m_baseInstances.faces)
        {
            BlockbenchInstanceGenerator::FaceInstance newFace = face;
            newFace.position += position;
            instances.push_back(newFace);
        }

        return instances;
    }

private:
    BlockbenchModel::Model m_model;
    BlockbenchInstanceGenerator::ModelInstances m_baseInstances;
    Zerith::BlockType m_blockType;
    std::shared_ptr<Zerith::TextureArray> m_textureArray;

    void assignTextureLayers()
    {
        // Remove overlay faces for grass blocks - they'll be handled in the shader
        if (m_blockType == Zerith::BlockType::GRASS_BLOCK) {
            auto it = std::remove_if(m_baseInstances.faces.begin(), m_baseInstances.faces.end(),
                [](const BlockbenchInstanceGenerator::FaceInstance& face) {
                    return face.textureName.find("overlay") != std::string::npos;
                });
            m_baseInstances.faces.erase(it, m_baseInstances.faces.end());
        }
        
        // Assign texture layers based on block type and face direction
        for (auto& face : m_baseInstances.faces)
        {
            switch (m_blockType)
            {
            case Zerith::BlockType::OAK_PLANKS:
            case Zerith::BlockType::OAK_SLAB:
            case Zerith::BlockType::OAK_STAIRS:
                face.textureLayer = m_textureArray->getTextureLayer("oak_planks_all");
                break;

            case Zerith::BlockType::STONE:
                face.textureLayer = m_textureArray->getTextureLayer("stone_all");
                break;

            case Zerith::BlockType::GRASS_BLOCK:
                // Different texture for each face
                // Note: In our coordinate system, face directions might be flipped

                // Debug output for grass block faces
                LOG_TRACE("Grass block face %d texture: %s", face.faceDirection, face.textureName.c_str());

                switch (face.faceDirection)
                {
                case 0: // Down (Y-) - should show dirt
                    face.textureLayer = m_textureArray->getTextureLayer("dirt");
                    break;
                case 1: // Up (Y+) - should show grass
                    face.textureLayer = m_textureArray->getTextureLayer("grass_top");
                    break;
                default: // Sides
                    face.textureLayer = m_textureArray->getTextureLayer("grass_side");
                    break;
                }
                break;

            default:
                face.textureLayer = 0; // Default to oak planks
                break;
            }
        }
    }
};
