#pragma once

#include "blockbench_model.h"
#include "logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

namespace BlockbenchInstanceGenerator {

// Structure that matches the FaceInstance used in the mesh shader
struct FaceInstance {
    glm::vec3 position;
    glm::vec4 rotation; // quaternion
    glm::vec3 scale;    // face scaling (width, height, depth)
    int faceDirection;   // 0=down, 1=up, 2=north, 3=south, 4=west, 5=east
    glm::vec4 uv;       // UV coordinates [minU, minV, maxU, maxV]
    uint32_t textureLayer; // Texture array layer index
    std::string textureName; // For debugging
    
    FaceInstance(const glm::vec3& pos = glm::vec3(0.0f), const glm::vec4& rot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
                 const glm::vec3& scl = glm::vec3(1.0f), int dir = -1, const glm::vec4& uvCoords = glm::vec4(0.0f, 0.0f, 16.0f, 16.0f),
                 uint32_t layer = 0, const std::string& tex = "")
        : position(pos), rotation(rot), scale(scl), faceDirection(dir), uv(uvCoords), textureLayer(layer), textureName(tex) {}
};

// Structure to hold all face instances for a complete model
struct ModelInstances {
    std::vector<FaceInstance> faces;
    
    // Optional: store element information for debugging
    std::vector<BlockbenchModel::Element> sourceElements;
    
    ModelInstances() = default;
};

// Helper functions for generating face instances from Blockbench elements
namespace Generator {
    
    // Create a quaternion for a specific face orientation
    inline glm::quat createFaceRotation(int faceIndex) {
        switch (faceIndex) {
            case 0: // Down face (Y-)
                return glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            case 1: // Up face (Y+)
                return glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            case 2: // North face (Z-)
                return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity (no rotation)
            case 3: // South face (Z+)
                return glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            case 4: // West face (X-)
                return glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            case 5: // East face (X+)
                return glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            default:
                return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity
        }
    }
    
    // Calculate face position based on element bounds and face direction
    // Positions faces at their corner origins, similar to the original hardcoded instances
    inline glm::vec3 calculateFacePosition(const BlockbenchModel::Element& vulkanElement, int faceIndex) {
        switch (faceIndex) {
            case 0: // Down face (Y-) - position at bottom corner of element
                return glm::vec3(vulkanElement.from.x, vulkanElement.from.y, vulkanElement.to.z);
            case 1: // Up face (Y+) - position at top corner of element  
                return glm::vec3(vulkanElement.from.x, vulkanElement.to.y, vulkanElement.from.z);
            case 2: // North face (Z-) - position at north corner of element
                return glm::vec3(vulkanElement.from.x, vulkanElement.from.y, vulkanElement.from.z);
            case 3: // South face (Z+) - position at south corner of element
                return glm::vec3(vulkanElement.to.x, vulkanElement.from.y, vulkanElement.to.z);
            case 4: // West face (X-) - position at west corner of element
                return glm::vec3(vulkanElement.from.x, vulkanElement.from.y, vulkanElement.to.z);
            case 5: // East face (X+) - position at east corner of element
                return glm::vec3(vulkanElement.to.x, vulkanElement.from.y, vulkanElement.from.z);
            default:
                return vulkanElement.from; // Default to element origin
        }
    }
    
    // Calculate face scale based on element dimensions and face direction
    inline glm::vec3 calculateFaceScale(const BlockbenchModel::Element& vulkanElement, int faceIndex) {
        glm::vec3 size = BlockbenchModel::Conversion::getElementSize(vulkanElement);
        
        switch (faceIndex) {
            case 0: // Down face (Y-) - X,Z plane
            case 1: // Up face (Y+) - X,Z plane
                return glm::vec3(size.x, size.z, 1.0f);
            case 2: // North face (Z-) - X,Y plane
            case 3: // South face (Z+) - X,Y plane
                return glm::vec3(size.x, size.y, 1.0f);
            case 4: // West face (X-) - Z,Y plane
            case 5: // East face (X+) - Z,Y plane
                return glm::vec3(size.z, size.y, 1.0f);
            default:
                return glm::vec3(1.0f);
        }
    }
    
    // Check if a face should be rendered (has texture and isn't culled)
    inline bool shouldRenderFace(const BlockbenchModel::Face& face) {
        // A face should be rendered if it has a texture reference
        return !face.texture.empty();
    }
    
    // Get the face from an element by index
    inline const BlockbenchModel::Face& getFace(const BlockbenchModel::Element& element, int faceIndex) {
        switch (faceIndex) {
            case 0: return element.down;   // Y-
            case 1: return element.up;     // Y+
            case 2: return element.north;  // Z-
            case 3: return element.south;  // Z+
            case 4: return element.west;   // X-
            case 5: return element.east;   // X+
            default: return element.down;
        }
    }
    
    // Face names for debugging
    inline const char* getFaceName(int faceIndex) {
        switch (faceIndex) {
            case 0: return "down";
            case 1: return "up";
            case 2: return "north";
            case 3: return "south";
            case 4: return "west";
            case 5: return "east";
            default: return "unknown";
        }
    }
    
    // Get readable rotation description
    inline std::string getRotationDescription(int faceIndex) {
        switch (faceIndex) {
            case 0: return "-90° around X (down face)";
            case 1: return "+90° around X (up face)";
            case 2: return "180° around Y (north face)";
            case 3: return "0° (south face, no rotation)";
            case 4: return "+90° around Y (west face)";
            case 5: return "-90° around Y (east face)";
            default: return "unknown rotation";
        }
    }

    // Generate face instances for a single Blockbench element
    inline void generateElementInstances(const BlockbenchModel::Element& bbElement, 
                                 std::vector<FaceInstance>& instances) {
        // Convert element to Vulkan coordinates
        BlockbenchModel::Element vulkanElement;
        BlockbenchModel::Conversion::convertElement(bbElement, vulkanElement);
        
        // Debug output commented out for performance
        // std::cout << "Processing Blockbench element:" << std::endl;
        // std::cout << "  Original BB coords: from(" << bbElement.from.x << ", " << bbElement.from.y << ", " << bbElement.from.z 
        //           << ") to(" << bbElement.to.x << ", " << bbElement.to.y << ", " << bbElement.to.z << ")" << std::endl;
        // std::cout << "  Vulkan coords: from(" << vulkanElement.from.x << ", " << vulkanElement.from.y << ", " << vulkanElement.from.z 
        //           << ") to(" << vulkanElement.to.x << ", " << vulkanElement.to.y << ", " << vulkanElement.to.z << ")" << std::endl;
        
        // Generate instances for each face
        for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
            const BlockbenchModel::Face& face = getFace(bbElement, faceIndex);
            
            // Only create instance if face should be rendered
            if (shouldRenderFace(face)) {
                glm::vec3 position = calculateFacePosition(vulkanElement, faceIndex);
                glm::quat rotation = createFaceRotation(faceIndex);
                glm::vec3 scale = calculateFaceScale(vulkanElement, faceIndex);
                
                // Debug output for overlay textures
                if (face.texture.find("overlay") != std::string::npos) {
                    LOG_TRACE("  Creating overlay face: %s with texture: %s", 
                              getFaceName(faceIndex), face.texture.c_str());
                }
                
                // Convert quaternion to vec4 for shader compatibility
                glm::vec4 rotationVec4(rotation.x, rotation.y, rotation.z, rotation.w);
                
                // Debug output commented out for performance
                // std::cout << "  Creating face instance: " << getFaceName(faceIndex) 
                //           << " (direction=" << faceIndex << ") at position (" 
                //           << position.x << ", " << position.y << ", " << position.z 
                //           << ") scale (" << scale.x << ", " << scale.y << ", " << scale.z
                //           << ") rotation quat(" << rotation.x << ", " << rotation.y << ", " << rotation.z << ", " << rotation.w
                //           << ") UV[" << face.uv.x << ", " << face.uv.y << ", " << face.uv.z << ", " << face.uv.w 
                //           << "] with texture: " << face.texture << std::endl;
                
                instances.emplace_back(position, rotationVec4, scale, faceIndex, face.uv, 0, face.texture);
            } else {
                // Debug output commented out for performance
                // std::cout << "  Skipping face: " << getFaceName(faceIndex) 
                //           << " (no texture or empty texture)" << std::endl;
            }
        }
    }
    
    // Generate all face instances for a complete model
    inline ModelInstances generateModelInstances(const BlockbenchModel::Model& model) {
        ModelInstances result;
        
        // Reserve space for potential instances (6 faces per element)
        result.faces.reserve(model.elements.size() * 6);
        result.sourceElements = model.elements;
        
        LOG_TRACE("Generating instances for model with %zu elements", model.elements.size());
        
        // Generate instances for each element
        for (size_t i = 0; i < model.elements.size(); ++i) {
            LOG_TRACE("Processing element %zu", i);
            generateElementInstances(model.elements[i], result.faces);
        }
        
        LOG_TRACE("Generated %zu face instances total", result.faces.size());
        
        return result;
    }
    
    // Calculate total bounding box for all elements in Vulkan coordinates
    struct BoundingBox {
        glm::vec3 min;
        glm::vec3 max;
        
        BoundingBox() : min(FLT_MAX), max(-FLT_MAX) {}
        
        void expand(const glm::vec3& point) {
            min = glm::min(min, point);
            max = glm::max(max, point);
        }
        
        glm::vec3 center() const {
            return (min + max) * 0.5f;
        }
        
        glm::vec3 size() const {
            return max - min;
        }
    };
    
    inline BoundingBox calculateModelBounds(const BlockbenchModel::Model& model) {
        BoundingBox bounds;
        
        for (const auto& bbElement : model.elements) {
            BlockbenchModel::Element vulkanElement;
            BlockbenchModel::Conversion::convertElement(bbElement, vulkanElement);
            
            bounds.expand(vulkanElement.from);
            bounds.expand(vulkanElement.to);
        }
        
        return bounds;
    }
    
} // namespace Generator

} // namespace BlockbenchInstanceGenerator