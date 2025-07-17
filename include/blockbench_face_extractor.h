#pragma once

#include "blockbench_model.h"
#include "block_face_bounds.h"
#include "logger.h"
#include <algorithm>

namespace BlockbenchFaceExtractor {

// Extract face bounds from a Blockbench element for a specific face
inline Zerith::FaceBounds extractFaceBounds(const BlockbenchModel::Element& element, int faceIndex) {
    // Convert from Blockbench coordinates (0-16) to normalized (0-1)
    glm::vec3 from = element.from / 16.0f;
    glm::vec3 to = element.to / 16.0f;
    
    // Ensure from < to for each axis
    for (int i = 0; i < 3; ++i) {
        if (from[i] > to[i]) {
            std::swap(from[i], to[i]);
        }
    }
    
    // Calculate 2D bounds based on face direction
    switch (faceIndex) {
        case 0: // Down face (Y-) - XZ plane
        case 1: // Up face (Y+) - XZ plane
            return Zerith::FaceBounds(from.x, from.z, to.x, to.z);
            
        case 2: // North face (Z-) - XY plane
        case 3: // South face (Z+) - XY plane
            return Zerith::FaceBounds(from.x, from.y, to.x, to.y);
            
        case 4: // West face (X-) - ZY plane
        case 5: // East face (X+) - ZY plane
            return Zerith::FaceBounds(from.z, from.y, to.z, to.y);
            
        default:
            return Zerith::FaceBounds(0.0f, 0.0f, 1.0f, 1.0f);
    }
}

// Extract all face bounds from a Blockbench model
inline Zerith::BlockFaceBounds extractBlockFaceBounds(const BlockbenchModel::Model& model) {
    Zerith::BlockFaceBounds result;
    
    // Special handling for multi-element blocks (like stairs)
    // For greedy meshing, we want representative bounds, not combined bounds
    if (model.elements.size() > 1) {
        // Use the first (largest) element's bounds as representative
        // This works well for stairs where the first element is the base slab
        if (!model.elements.empty()) {
            const auto& firstElement = model.elements[0];
            for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
                // Get the face from the element
                const BlockbenchModel::Face* face = nullptr;
                switch (faceIndex) {
                    case 0: face = &firstElement.down; break;
                    case 1: face = &firstElement.up; break;
                    case 2: face = &firstElement.north; break;
                    case 3: face = &firstElement.south; break;
                    case 4: face = &firstElement.west; break;
                    case 5: face = &firstElement.east; break;
                }
                
                // Skip faces without textures (they don't render)
                if (!face || face->texture.empty()) {
                    result.faces[faceIndex] = Zerith::FaceBounds(0.0f, 0.0f, 0.0f, 0.0f);
                    continue;
                }
                
                // Extract bounds for this face
                result.faces[faceIndex] = extractFaceBounds(firstElement, faceIndex);
            }
        }
        return result;
    }
    
    // Original logic for single-element blocks
    // Initialize with empty bounds (no coverage)
    for (auto& face : result.faces) {
        face = Zerith::FaceBounds(1.0f, 1.0f, 0.0f, 0.0f);  // Invalid bounds (min > max)
    }
    
    // For each element in the model
    for (const auto& element : model.elements) {
        // For each face
        for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
            // Get the face from the element
            const BlockbenchModel::Face* face = nullptr;
            switch (faceIndex) {
                case 0: face = &element.down; break;
                case 1: face = &element.up; break;
                case 2: face = &element.north; break;
                case 3: face = &element.south; break;
                case 4: face = &element.west; break;
                case 5: face = &element.east; break;
            }
            
            // Skip faces without textures (they don't render)
            if (!face || face->texture.empty()) {
                continue;
            }
            
            // Extract bounds for this face
            Zerith::FaceBounds faceBounds = extractFaceBounds(element, faceIndex);
            
            // Expand the face bounds to include this element's face
            // If this is the first valid face, just set it
            if (result.faces[faceIndex].min.x > result.faces[faceIndex].max.x) {
                result.faces[faceIndex] = faceBounds;
            } else {
                // Otherwise, expand to include this face
                result.faces[faceIndex].min = glm::min(result.faces[faceIndex].min, faceBounds.min);
                result.faces[faceIndex].max = glm::max(result.faces[faceIndex].max, faceBounds.max);
            }
        }
    }
    
    // For any faces that weren't set (no elements had that face), set to no coverage
    for (auto& face : result.faces) {
        if (face.min.x > face.max.x) {
            face = Zerith::FaceBounds(0.0f, 0.0f, 0.0f, 0.0f);  // No coverage
        }
    }
    
    return result;
}

// Helper to debug print face bounds
inline void printBlockFaceBounds(const Zerith::BlockFaceBounds& bounds, const std::string& blockName) {
    const char* faceNames[] = {"down", "up", "north", "south", "west", "east"};
    
    LOG_DEBUG("Face bounds for %s:", blockName.c_str());
    for (int i = 0; i < 6; ++i) {
        const auto& face = bounds.faces[i];
        if (face.area() > 0.001f) {
            LOG_DEBUG("  %s: [%.3f,%.3f] to [%.3f,%.3f] (area: %.6f, full: %s)",
                      faceNames[i], face.min.x, face.min.y, face.max.x, face.max.y,
                      face.area(), face.isFull() ? "yes" : "no");
        }
    }
}

} // namespace BlockbenchFaceExtractor