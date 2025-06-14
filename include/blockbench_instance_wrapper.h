#pragma once

#include "blockbench_model.h"
#include "blockbench_instance_generator.h"
#include "texture_array.h"
#include "chunk.h"
#include "logger.h"
#include "face_instance_pool.h"
#include "block_registry.h"
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include <algorithm>

#include "block_types.h"

// Wrapper class to generate instances at specific positions
class BlockbenchInstanceWrapper
{
public:
    explicit BlockbenchInstanceWrapper(BlockbenchModel::Model model, Zerith::BlockType blockType,
                                       const std::shared_ptr<Zerith::TextureArray>& textureArray)
        : m_model(std::move(model)), m_blockType(blockType), m_textureArray(textureArray)
    {
        // Auto-register textures from model
        registerModelTextures();
        
        // Pre-generate base instances at origin
        m_baseInstances = BlockbenchInstanceGenerator::Generator::generateModelInstances(m_model);

        // Assign texture layers based on block type
        assignTextureLayers();
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
                face.textureLayer,         // textureLayer
                face.textureName           // textureName
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
                face.textureLayer,         // textureLayer
                face.textureName           // textureName
            );
        }
    }

private:
    BlockbenchModel::Model m_model;
    BlockbenchInstanceGenerator::ModelInstances m_baseInstances;
    Zerith::BlockType m_blockType;
    std::shared_ptr<Zerith::TextureArray> m_textureArray;
    
    void registerModelTextures()
    {
        // Auto-register all textures referenced in the model
        for (const auto& [key, texture] : m_model.textures)
        {
            // Skip texture references (they should have been resolved during parsing)
            if (texture.empty() || texture[0] == '#') {
                LOG_DEBUG("Skipping texture reference: %s = %s", key.c_str(), texture.c_str());
                continue;
            }
            
            // Convert minecraft texture path to asset path
            // e.g., "minecraft:block/stone" -> "assets/stone.png"
            std::string texturePath = texture;
            
            // Remove namespace prefixes if present
            if (texturePath.find("zerith:block/") == 0) {
                texturePath = texturePath.substr(13); // Remove "zerith:block/"
            } else if (texturePath.find("zerith:") == 0) {
                texturePath = texturePath.substr(7); // Remove "zerith:"
            } else if (texturePath.find("minecraft:block/") == 0) {
                texturePath = texturePath.substr(16); // Remove "minecraft:block/"
            } else if (texturePath.find("minecraft:") == 0) {
                texturePath = texturePath.substr(10); // Remove "minecraft:"
            } else if (texturePath.find("block/") == 0) {
                texturePath = texturePath.substr(6); // Remove "block/"
            }
            
            // Add assets/zerith/textures/block/ prefix and .png extension
            texturePath = "assets/zerith/textures/block/" + texturePath + ".png";
            
            // Register the texture (will return existing layer if already registered)
            m_textureArray->getOrRegisterTexture(texturePath);
            
            LOG_DEBUG("Auto-registered texture: %s", texturePath.c_str());
        }
        
        // Also register textures from resolved faces
        for (const auto& element : m_model.elements) {
            // Check each face individually
            auto registerFaceTexture = [this](const BlockbenchModel::Face& face, const std::string& faceName) {
                if (!face.texture.empty() && face.texture[0] != '#') {
                    std::string texturePath = face.texture;
                    
                    // Remove namespace prefixes if present
                    if (texturePath.find("zerith:block/") == 0) {
                        texturePath = texturePath.substr(13); // Remove "zerith:block/"
                    } else if (texturePath.find("zerith:") == 0) {
                        texturePath = texturePath.substr(7); // Remove "zerith:"
                    } else if (texturePath.find("minecraft:block/") == 0) {
                        texturePath = texturePath.substr(16); // Remove "minecraft:block/"
                    } else if (texturePath.find("minecraft:") == 0) {
                        texturePath = texturePath.substr(10); // Remove "minecraft:"
                    } else if (texturePath.find("block/") == 0) {
                        texturePath = texturePath.substr(6); // Remove "block/"
                    }
                    
                    // Add assets/zerith/textures/block/ prefix and .png extension
                    texturePath = "assets/zerith/textures/block/" + texturePath + ".png";
                    
                    // Register the texture (will return existing layer if already registered)
                    m_textureArray->getOrRegisterTexture(texturePath);
                    
                    LOG_DEBUG("Auto-registered %s face texture: %s", faceName.c_str(), texturePath.c_str());
                }
            };
            
            registerFaceTexture(element.down, "down");
            registerFaceTexture(element.up, "up");
            registerFaceTexture(element.north, "north");
            registerFaceTexture(element.south, "south");
            registerFaceTexture(element.west, "west");
            registerFaceTexture(element.east, "east");
        }
    }

    void assignTextureLayers()
    {
        // Remove overlay faces for grass blocks - they'll be handled in the shader
        if (m_blockType == Zerith::BlockTypes::GRASS_BLOCK) {
            auto it = std::remove_if(m_baseInstances.faces.begin(), m_baseInstances.faces.end(),
                [](const BlockbenchInstanceGenerator::FaceInstance& face) {
                    return face.textureName.find("overlay") != std::string::npos;
                });
            m_baseInstances.faces.erase(it, m_baseInstances.faces.end());
        }
        
        // Assign texture layers based on the resolved texture names from the model
        for (auto& face : m_baseInstances.faces)
        {
            // Use the texture name that was resolved from the model
            std::string textureName = face.textureName;
            
            // Remove namespace prefixes if present
            if (textureName.find("zerith:block/") == 0) {
                textureName = textureName.substr(13); // Remove "zerith:block/"
            } else if (textureName.find("zerith:") == 0) {
                textureName = textureName.substr(7); // Remove "zerith:"
            } else if (textureName.find("minecraft:block/") == 0) {
                textureName = textureName.substr(16); // Remove "minecraft:block/"
            } else if (textureName.find("minecraft:") == 0) {
                textureName = textureName.substr(10); // Remove "minecraft:"
            } else if (textureName.find("block/") == 0) {
                textureName = textureName.substr(6); // Remove "block/"
            }
            
            // Skip texture references (should have been resolved)
            if (!textureName.empty() && textureName[0] != '#') {
                face.textureLayer = m_textureArray->getTextureLayer(textureName);
                LOG_TRACE("Assigned texture layer for %s: %s -> layer %d", 
                    toString(m_blockType).c_str(), textureName.c_str(), face.textureLayer);
            } else {
                // Fallback for unresolved textures
                LOG_WARN("Unresolved texture reference for block %s: %s", 
                    toString(m_blockType).c_str(), textureName.c_str());
                face.textureLayer = 0; // Default texture
            }
        }
    }
    
    // Helper function to convert BlockType to string for logging
    std::string toString(Zerith::BlockType type) const {
        auto& registry = Zerith::BlockRegistry::getInstance();
        auto block = registry.getBlock(type);
        return block ? block->getId() : "UNKNOWN";
    }
};
