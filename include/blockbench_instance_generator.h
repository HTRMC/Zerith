#pragma once

#include "blockbench_model.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace BlockbenchInstanceGenerator {

// Structure that matches the FaceInstance used in the mesh shader
struct FaceInstance {
    glm::vec3 position;
    glm::vec4 rotation; // quaternion
    
    FaceInstance(const glm::vec3& pos = glm::vec3(0.0f), const glm::vec4& rot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
        : position(pos), rotation(rot) {}
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
    glm::quat createFaceRotation(int faceIndex) {
        switch (faceIndex) {
            case 0: // Down face (Y-)
                return glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            case 1: // Up face (Y+)
                return glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            case 2: // North face (Z-)
                return glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            case 3: // South face (Z+)
                return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity (no rotation)
            case 4: // West face (X-)
                return glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            case 5: // East face (X+)
                return glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            default:
                return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity
        }
    }
    
    // Calculate face position based on element bounds and face direction
    glm::vec3 calculateFacePosition(const BlockbenchModel::Element& vulkanElement, int faceIndex) {
        glm::vec3 center = BlockbenchModel::Conversion::getElementCenter(vulkanElement);
        glm::vec3 size = BlockbenchModel::Conversion::getElementSize(vulkanElement);
        
        switch (faceIndex) {
            case 0: // Down face (Y-)
                return glm::vec3(center.x, vulkanElement.from.y, center.z);
            case 1: // Up face (Y+)
                return glm::vec3(center.x, vulkanElement.to.y, center.z);
            case 2: // North face (Z-)
                return glm::vec3(center.x, center.y, vulkanElement.from.z);
            case 3: // South face (Z+)
                return glm::vec3(center.x, center.y, vulkanElement.to.z);
            case 4: // West face (X-)
                return glm::vec3(vulkanElement.from.x, center.y, center.z);
            case 5: // East face (X+)
                return glm::vec3(vulkanElement.to.x, center.y, center.z);
            default:
                return center;
        }
    }
    
    // Check if a face should be rendered (has texture and isn't culled)
    bool shouldRenderFace(const BlockbenchModel::Face& face) {
        // A face should be rendered if it has a texture reference
        return !face.texture.empty();
    }
    
    // Get the face from an element by index
    const BlockbenchModel::Face& getFace(const BlockbenchModel::Element& element, int faceIndex) {
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
    
    // Generate face instances for a single Blockbench element
    void generateElementInstances(const BlockbenchModel::Element& bbElement, 
                                 std::vector<FaceInstance>& instances) {
        // Convert element to Vulkan coordinates
        BlockbenchModel::Element vulkanElement;
        BlockbenchModel::Conversion::convertElement(bbElement, vulkanElement);
        
        // Generate instances for each face
        for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
            const BlockbenchModel::Face& face = getFace(bbElement, faceIndex);
            
            // Only create instance if face should be rendered
            if (shouldRenderFace(face)) {
                glm::vec3 position = calculateFacePosition(vulkanElement, faceIndex);
                glm::quat rotation = createFaceRotation(faceIndex);
                
                // Convert quaternion to vec4 for shader compatibility
                glm::vec4 rotationVec4(rotation.x, rotation.y, rotation.z, rotation.w);
                
                instances.emplace_back(position, rotationVec4);
            }
        }
    }
    
    // Generate all face instances for a complete model
    ModelInstances generateModelInstances(const BlockbenchModel::Model& model) {
        ModelInstances result;
        
        // Reserve space for potential instances (6 faces per element)
        result.faces.reserve(model.elements.size() * 6);
        result.sourceElements = model.elements;
        
        // Generate instances for each element
        for (const auto& element : model.elements) {
            generateElementInstances(element, result.faces);
        }
        
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
    
    BoundingBox calculateModelBounds(const BlockbenchModel::Model& model) {
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