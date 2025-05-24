#pragma once

#include "blockbench_model.h"
#include "blockbench_instance_generator.h"
#include "texture_array.h"
#include "chunk.h"
#include <vector>
#include <glm/glm.hpp>
#include <memory>

// Wrapper class to generate instances at specific positions
class BlockbenchInstanceWrapper {
public:
    explicit BlockbenchInstanceWrapper(BlockbenchModel::Model model, MeshShader::BlockType blockType,
                                     const std::shared_ptr<MeshShader::TextureArray>& textureArray) 
        : m_model(std::move(model)), m_blockType(blockType), m_textureArray(textureArray) {
        // Pre-generate base instances at origin
        m_baseInstances = BlockbenchInstanceGenerator::Generator::generateModelInstances(m_model);
        
        // Assign texture layers based on block type
        assignTextureLayers();
    }
    
    // Generate face instances at a specific world position
    std::vector<BlockbenchInstanceGenerator::FaceInstance> generateInstancesAtPosition(const glm::vec3& position) {
        std::vector<BlockbenchInstanceGenerator::FaceInstance> instances;
        instances.reserve(m_baseInstances.faces.size());
        
        // Copy base instances and offset by position
        for (const auto& face : m_baseInstances.faces) {
            BlockbenchInstanceGenerator::FaceInstance newFace = face;
            newFace.position += position;
            instances.push_back(newFace);
        }
        
        return instances;
    }
    
private:
    BlockbenchModel::Model m_model;
    BlockbenchInstanceGenerator::ModelInstances m_baseInstances;
    MeshShader::BlockType m_blockType;
    std::shared_ptr<MeshShader::TextureArray> m_textureArray;
    
    void assignTextureLayers() {
        // Assign texture layers based on block type and face direction
        for (auto& face : m_baseInstances.faces) {
            switch (m_blockType) {
                case MeshShader::BlockType::OAK_PLANKS:
                case MeshShader::BlockType::OAK_SLAB:
                case MeshShader::BlockType::OAK_STAIRS:
                    face.textureLayer = m_textureArray->getTextureLayer("oak_planks_all");
                    break;
                    
                case MeshShader::BlockType::STONE:
                    face.textureLayer = m_textureArray->getTextureLayer("stone_all");
                    break;
                    
                case MeshShader::BlockType::GRASS_BLOCK:
                    // Different texture for each face
                    switch (face.faceDirection) {
                        case 0: // Down
                            face.textureLayer = m_textureArray->getTextureLayer("grass_bottom");
                            break;
                        case 1: // Up
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