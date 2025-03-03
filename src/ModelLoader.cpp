#include "ModelLoader.hpp"
#include "VulkanApp.hpp"
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

// Helper function to read a file into a string
static std::string readFileToString(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::optional<ModelData> ModelLoader::loadModel(const std::string& filename) {
    try {
        // Read the file
        std::string jsonContent = readFileToString(filename);
        
        // Parse the JSON
        nlohmann::json modelJson = nlohmann::json::parse(jsonContent);
        
        // Create model data
        ModelData modelData;
        modelData.name = filename;
        
        // Check if the model has elements
        if (!modelJson.contains("elements") || !modelJson["elements"].is_array()) {
            std::cerr << "Model file does not contain elements array" << std::endl;
            return std::nullopt;
        }
        
        // Process each element
        for (const auto& elementJson : modelJson["elements"]) {
            Element element;
            
            // Parse 'from' coordinates
            if (elementJson.contains("from") && elementJson["from"].is_array() && elementJson["from"].size() == 3) {
                element.from.x = elementJson["from"][0];
                element.from.y = elementJson["from"][1];
                element.from.z = elementJson["from"][2];
                // Convert from BlockBench coordinates (0-16) to normalized (-0.5 to 0.5)
                element.from = convertCoordinates(element.from);
            }
            
            // Parse 'to' coordinates
            if (elementJson.contains("to") && elementJson["to"].is_array() && elementJson["to"].size() == 3) {
                element.to.x = elementJson["to"][0];
                element.to.y = elementJson["to"][1];
                element.to.z = elementJson["to"][2];
                // Convert from BlockBench coordinates (0-16) to normalized (-0.5 to 0.5)
                element.to = convertCoordinates(element.to);
            }
            
            // Parse color
            if (elementJson.contains("color")) {
                element.color = elementJson["color"];
            }
            
            // Parse faces
            if (elementJson.contains("faces") && elementJson["faces"].is_object()) {
                for (auto& [faceName, faceData] : elementJson["faces"].items()) {
                    Face face;
                    
                    // Parse texture reference
                    if (faceData.contains("texture")) {
                        face.texture = faceData["texture"];
                    }
                    
                    // Parse UVs if present
                    if (faceData.contains("uv") && faceData["uv"].is_array()) {
                        auto uvs = parseUVs(faceData["uv"]);
                        // UVs can be used here if needed
                    }
                    
                    // Store the face
                    element.faces[faceName] = face;
                }
            }
            
            // Add the element to the model
            modelData.elements.push_back(element);
        }
        
        // Process all elements to generate vertices and indices
        for (size_t i = 0; i < modelData.elements.size(); i++) {
            processElement(modelData.elements[i], modelData, i);
        }
        
        // If we have vertices and indices, mark as loaded
        if (!modelData.vertices.empty() && !modelData.indices.empty()) {
            modelData.loaded = true;
            return modelData;
        }
        
        std::cerr << "Failed to generate geometry for model: " << filename << std::endl;
        return std::nullopt;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading model " << filename << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

glm::vec3 ModelLoader::convertCoordinates(const glm::vec3& bbCoords) {
    // Convert from BlockBench coordinates (0-16) to normalized (0.0 to 1.0)
    // Also swap Y and Z axes, with appropriate adjustments
    return glm::vec3(
        (bbCoords.x / 16.0f),   // X stays as X but normalized
        (bbCoords.z / 16.0f),   // Z becomes Y
        (bbCoords.y / 16.0f)    // Y becomes Z
    );
}

std::vector<glm::vec2> ModelLoader::parseUVs(const nlohmann::json& uvJson) {
    std::vector<glm::vec2> result;
    if (uvJson.size() == 4) {
        // BlockBench UVs are typically [x1, y1, x2, y2]
        float u1 = uvJson[0];
        float v1 = uvJson[1];
        float u2 = uvJson[2];
        float v2 = uvJson[3];
        
        // Normalize UVs to 0-1 range
        u1 /= 16.0f;
        v1 /= 16.0f;
        u2 /= 16.0f;
        v2 /= 16.0f;
        
        // Add corners in clockwise order
        result.push_back({u1, v1}); // Bottom-left
        result.push_back({u2, v1}); // Bottom-right
        result.push_back({u2, v2}); // Top-right
        result.push_back({u1, v2}); // Top-left
    }
    return result;
}

void ModelLoader::processElement(Element& element, ModelData& modelData, size_t elementIndex) {
    // Generate geometry for this element
    createElementGeometry(element, modelData);
}

void ModelLoader::createElementGeometry(const Element& element, ModelData& modelData) {
    // Use the element color as a default
    glm::vec3 defaultColor = parseColor(element.color);

    // The element.from and element.to are already converted.
    // Define the bounds:
    float x_min = element.from.x;
    float x_max = element.to.x;
    // Engine Y (forward/back): comes from BlockBench Z values.
    float y_min = element.from.y;  // Back face (north)
    float y_max = element.to.y;    // Front face (south)
    // Engine Z (vertical): comes from BlockBench Y values.
    float z_min = element.from.z;  // Bottom face (down)
    float z_max = element.to.z;    // Top face (up)

    // --- FRONT FACE (BlockBench "south" → Engine front, Y = max) ---
    if (element.faces.find("south") != element.faces.end()) {
        glm::vec3 faceColor = parseColor(element.faces.at("south").texture != "#missing" ? element.color : element.color);
        Vertex frontBottomLeft  = { { x_min, y_max, z_min }, faceColor };
        Vertex frontBottomRight = { { x_max, y_max, z_min }, faceColor };
        Vertex frontTopRight    = { { x_max, y_max, z_max }, faceColor };
        Vertex frontTopLeft     = { { x_min, y_max, z_max }, faceColor };
        uint16_t base = static_cast<uint16_t>(modelData.vertices.size());
        modelData.vertices.insert(modelData.vertices.end(), {
            frontBottomLeft, frontBottomRight, frontTopRight, frontTopLeft
        });
        modelData.indices.insert(modelData.indices.end(), {
            static_cast<uint16_t>(base),
            static_cast<uint16_t>(base + 1),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 3),
            static_cast<uint16_t>(base)
        });
    }

    // --- BACK FACE (BlockBench "north" → Engine back, Y = min) ---
    if (element.faces.find("north") != element.faces.end()) {
        glm::vec3 faceColor = parseColor(element.faces.at("north").texture != "#missing" ? element.color : element.color);
        Vertex backBottomLeft  = { { x_min, y_min, z_min }, faceColor };
        Vertex backBottomRight = { { x_max, y_min, z_min }, faceColor };
        Vertex backTopRight    = { { x_max, y_min, z_max }, faceColor };
        Vertex backTopLeft     = { { x_min, y_min, z_max }, faceColor };
        uint16_t base = static_cast<uint16_t>(modelData.vertices.size());
        modelData.vertices.insert(modelData.vertices.end(), {
            backBottomLeft, backBottomRight, backTopRight, backTopLeft
        });
        // Reverse winding order for proper face orientation:
        modelData.indices.insert(modelData.indices.end(), {
            static_cast<uint16_t>(base + 1),
            static_cast<uint16_t>(base),
            static_cast<uint16_t>(base + 3),
            static_cast<uint16_t>(base + 3),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 1)
        });
    }

    // --- RIGHT FACE (BlockBench "east" → Engine right, X = max) ---
    if (element.faces.find("east") != element.faces.end()) {
        glm::vec3 faceColor = parseColor(element.faces.at("east").texture != "#missing" ? element.color : element.color);
        Vertex frontBottomRight = { { x_max, y_max, z_min }, faceColor };
        Vertex backBottomRight  = { { x_max, y_min, z_min }, faceColor };
        Vertex backTopRight     = { { x_max, y_min, z_max }, faceColor };
        Vertex frontTopRight    = { { x_max, y_max, z_max }, faceColor };
        uint16_t base = static_cast<uint16_t>(modelData.vertices.size());
        modelData.vertices.insert(modelData.vertices.end(), {
            frontBottomRight, backBottomRight, backTopRight, frontTopRight
        });
        modelData.indices.insert(modelData.indices.end(), {
            static_cast<uint16_t>(base),
            static_cast<uint16_t>(base + 1),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 3),
            static_cast<uint16_t>(base)
        });
    }

    // --- LEFT FACE (BlockBench "west" → Engine left, X = min) ---
    if (element.faces.find("west") != element.faces.end()) {
        glm::vec3 faceColor = parseColor(element.faces.at("west").texture != "#missing" ? element.color : element.color);
        Vertex backBottomLeft  = { { x_min, y_min, z_min }, faceColor };
        Vertex frontBottomLeft = { { x_min, y_max, z_min }, faceColor };
        Vertex frontTopLeft    = { { x_min, y_max, z_max }, faceColor };
        Vertex backTopLeft     = { { x_min, y_min, z_max }, faceColor };
        uint16_t base = static_cast<uint16_t>(modelData.vertices.size());
        modelData.vertices.insert(modelData.vertices.end(), {
            backBottomLeft, frontBottomLeft, frontTopLeft, backTopLeft
        });
        modelData.indices.insert(modelData.indices.end(), {
            static_cast<uint16_t>(base),
            static_cast<uint16_t>(base + 1),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 3),
            static_cast<uint16_t>(base)
        });
    }

    // --- TOP FACE (BlockBench "up" → Engine top, Z = max) ---
    if (element.faces.find("up") != element.faces.end()) {
        glm::vec3 faceColor = parseColor(element.faces.at("up").texture != "#missing" ? element.color : element.color);
        Vertex frontTopLeft  = { { x_min, y_max, z_max }, faceColor };
        Vertex frontTopRight = { { x_max, y_max, z_max }, faceColor };
        Vertex backTopRight  = { { x_max, y_min, z_max }, faceColor };
        Vertex backTopLeft   = { { x_min, y_min, z_max }, faceColor };
        uint16_t base = static_cast<uint16_t>(modelData.vertices.size());
        modelData.vertices.insert(modelData.vertices.end(), {
            frontTopLeft, frontTopRight, backTopRight, backTopLeft
        });
        modelData.indices.insert(modelData.indices.end(), {
            static_cast<uint16_t>(base),
            static_cast<uint16_t>(base + 1),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 3),
            static_cast<uint16_t>(base)
        });
    }

    // --- BOTTOM FACE (BlockBench "down" → Engine bottom, Z = min) ---
    if (element.faces.find("down") != element.faces.end()) {
        glm::vec3 faceColor = parseColor(element.faces.at("down").texture != "#missing" ? element.color : element.color);
        Vertex frontBottomRight = { { x_max, y_max, z_min }, faceColor };
        Vertex frontBottomLeft  = { { x_min, y_max, z_min }, faceColor };
        Vertex backBottomLeft   = { { x_min, y_min, z_min }, faceColor };
        Vertex backBottomRight  = { { x_max, y_min, z_min }, faceColor };
        uint16_t base = static_cast<uint16_t>(modelData.vertices.size());
        modelData.vertices.insert(modelData.vertices.end(), {
            frontBottomRight, frontBottomLeft, backBottomLeft, backBottomRight
        });
        modelData.indices.insert(modelData.indices.end(), {
            static_cast<uint16_t>(base),
            static_cast<uint16_t>(base + 1),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 2),
            static_cast<uint16_t>(base + 3),
            static_cast<uint16_t>(base)
        });
    }
}

glm::vec3 ModelLoader::parseColor(int colorIndex) {
    // Default BlockBench color palette (simplified)
    // Returns RGB values normalized to 0-1 range
    switch (colorIndex) {
        case 0: return {0.0f, 0.0f, 0.0f}; // Black
        case 1: return {0.0f, 0.0f, 1.0f}; // Blue
        case 2: return {0.0f, 1.0f, 0.0f}; // Green
        case 3: return {0.0f, 1.0f, 1.0f}; // Cyan
        case 4: return {1.0f, 0.0f, 0.0f}; // Red
        case 5: return {1.0f, 0.0f, 1.0f}; // Magenta
        case 6: return {1.0f, 1.0f, 0.0f}; // Yellow
        case 7: return {1.0f, 1.0f, 1.0f}; // White
        case 8: return {0.5f, 0.5f, 0.5f}; // Gray
        // Add more colors as needed
        default: return {1.0f, 1.0f, 1.0f}; // Default white
    }
}