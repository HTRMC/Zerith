#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <unordered_map>

namespace BlockbenchModel {

// Represents a face of a Blockbench element
struct Face {
    std::string texture;    // Texture reference (e.g., "#down", "#up", etc.)
    std::string cullface;   // Cullface direction
    glm::vec4 uv;          // UV coordinates [u1, v1, u2, v2] (optional)
    
    Face() : uv(0.0f, 0.0f, 16.0f, 16.0f) {} // Default to full texture
};

// Represents a cube element in a Blockbench model
struct Element {
    glm::vec3 from;        // Start position in Blockbench coordinates (0-16)
    glm::vec3 to;          // End position in Blockbench coordinates (0-16)
    
    // Face definitions
    Face down;
    Face up;
    Face north;
    Face south;
    Face west;
    Face east;
    
    // Optional rotation (not implemented yet)
    // TODO: Add rotation support if needed
    
    Element() : from(0.0f), to(16.0f) {}
};

// Represents a complete Blockbench model
struct Model {
    std::string parent;                                    // Parent model reference
    std::unordered_map<std::string, std::string> textures; // Texture definitions
    std::vector<Element> elements;                         // Model elements
    
    Model() = default;
};

// Conversion functions for coordinate system transformation
namespace Conversion {
    
    // Convert Blockbench coordinates (16x16x16) to Vulkan coordinates (1x1x1)
    inline constexpr glm::vec3 blockbenchToVulkan(const glm::vec3& blockbenchPos) {
        return glm::vec3(
            blockbenchPos.x / 16.0f,    // X: 0-16 -> 0-1
            blockbenchPos.y / 16.0f,    // Y: 0-16 -> 0-1 
            blockbenchPos.z / 16.0f     // Z: 0-16 -> 0-1 (no flip)
        );
    }
    
    // Convert a Blockbench element to Vulkan coordinates
    inline constexpr void convertElement(const Element& bbElement, Element& vulkanElement) {
        vulkanElement = bbElement; // Copy face data
        vulkanElement.from = blockbenchToVulkan(bbElement.from);
        vulkanElement.to = blockbenchToVulkan(bbElement.to);
    }
    
    // Calculate the center position of an element in Vulkan coordinates
    inline constexpr glm::vec3 getElementCenter(const Element& vulkanElement) {
        return (vulkanElement.from + vulkanElement.to) * 0.5f;
    }
    
    // Calculate the size of an element in Vulkan coordinates
    inline constexpr glm::vec3 getElementSize(const Element& vulkanElement) {
        return glm::abs(vulkanElement.to - vulkanElement.from);
    }
    
    // Flip a model upside down by inverting Y coordinates
    inline void flipModelUpsideDown(Model& model) {
        for (auto& element : model.elements) {
            // Flip Y coordinates: new_y = 16 - old_y
            float newFromY = 16.0f - element.to.y;
            float newToY = 16.0f - element.from.y;
            
            element.from.y = newFromY;
            element.to.y = newToY;
            
            // Swap up and down faces since they're now inverted
            std::swap(element.up, element.down);
        }
    }
    
} // namespace Conversion

} // namespace BlockbenchModel