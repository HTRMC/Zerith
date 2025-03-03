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
    // Get the current vertex count (offset for indices)
    uint16_t baseVertex = static_cast<uint16_t>(modelData.vertices.size());
    
    // Create colors based on element's color or face colors
    glm::vec3 defaultColor = parseColor(element.color);
    
    // Create a cube from the element's bounds
    // Define the 8 corners of the cube
    std::vector<glm::vec3> corners = {
        {element.from.x, element.from.y, element.from.z}, // 0: bottom-left-front
        {element.to.x, element.from.y, element.from.z},   // 1: bottom-right-front
        {element.to.x, element.to.y, element.from.z},     // 2: top-right-front
        {element.from.x, element.to.y, element.from.z},   // 3: top-left-front
        {element.from.x, element.from.y, element.to.z},   // 4: bottom-left-back
        {element.to.x, element.from.y, element.to.z},     // 5: bottom-right-back
        {element.to.x, element.to.y, element.to.z},       // 6: top-right-back
        {element.from.x, element.to.y, element.to.z}      // 7: top-left-back
    };
    
    // Add vertices for each corner of the cube
    for (const auto& corner : corners) {
        Vertex vertex;
        vertex.pos = corner;
        vertex.color = defaultColor;
        modelData.vertices.push_back(vertex);
    }
    
    // Add indices for each face (2 triangles per face)
    // Check if the face is defined in the element and use its specific color if available
    
    // Front face (z = min)
    if (element.faces.find("north") != element.faces.end()) {
        glm::vec3 color = element.faces.at("north").texture != "#missing" ? 
                          parseColor(element.color) : defaultColor;
        
        // Update vertex colors for this face
        modelData.vertices[baseVertex + 0].color = color;
        modelData.vertices[baseVertex + 1].color = color;
        modelData.vertices[baseVertex + 2].color = color;
        modelData.vertices[baseVertex + 3].color = color;
        
        // Add indices
        modelData.indices.push_back(baseVertex + 0);
        modelData.indices.push_back(baseVertex + 1);
        modelData.indices.push_back(baseVertex + 2);
        modelData.indices.push_back(baseVertex + 2);
        modelData.indices.push_back(baseVertex + 3);
        modelData.indices.push_back(baseVertex + 0);
    }
    
    // Right face (x = max)
    if (element.faces.find("east") != element.faces.end()) {
        glm::vec3 color = element.faces.at("east").texture != "#missing" ? 
                          parseColor(element.color) : defaultColor;
        
        // Update vertex colors for this face
        modelData.vertices[baseVertex + 1].color = color;
        modelData.vertices[baseVertex + 5].color = color;
        modelData.vertices[baseVertex + 6].color = color;
        modelData.vertices[baseVertex + 2].color = color;
        
        // Add indices
        modelData.indices.push_back(baseVertex + 1);
        modelData.indices.push_back(baseVertex + 5);
        modelData.indices.push_back(baseVertex + 6);
        modelData.indices.push_back(baseVertex + 6);
        modelData.indices.push_back(baseVertex + 2);
        modelData.indices.push_back(baseVertex + 1);
    }
    
    // Back face (z = max)
    if (element.faces.find("south") != element.faces.end()) {
        glm::vec3 color = element.faces.at("south").texture != "#missing" ? 
                          parseColor(element.color) : defaultColor;
        
        // Update vertex colors for this face
        modelData.vertices[baseVertex + 5].color = color;
        modelData.vertices[baseVertex + 4].color = color;
        modelData.vertices[baseVertex + 7].color = color;
        modelData.vertices[baseVertex + 6].color = color;
        
        // Add indices
        modelData.indices.push_back(baseVertex + 5);
        modelData.indices.push_back(baseVertex + 4);
        modelData.indices.push_back(baseVertex + 7);
        modelData.indices.push_back(baseVertex + 7);
        modelData.indices.push_back(baseVertex + 6);
        modelData.indices.push_back(baseVertex + 5);
    }
    
    // Left face (x = min)
    if (element.faces.find("west") != element.faces.end()) {
        glm::vec3 color = element.faces.at("west").texture != "#missing" ? 
                          parseColor(element.color) : defaultColor;
        
        // Update vertex colors for this face
        modelData.vertices[baseVertex + 4].color = color;
        modelData.vertices[baseVertex + 0].color = color;
        modelData.vertices[baseVertex + 3].color = color;
        modelData.vertices[baseVertex + 7].color = color;
        
        // Add indices
        modelData.indices.push_back(baseVertex + 4);
        modelData.indices.push_back(baseVertex + 0);
        modelData.indices.push_back(baseVertex + 3);
        modelData.indices.push_back(baseVertex + 3);
        modelData.indices.push_back(baseVertex + 7);
        modelData.indices.push_back(baseVertex + 4);
    }
    
    // Top face (y = max)
    if (element.faces.find("up") != element.faces.end()) {
        glm::vec3 color = element.faces.at("up").texture != "#missing" ? 
                          parseColor(element.color) : defaultColor;
        
        // Update vertex colors for this face
        modelData.vertices[baseVertex + 3].color = color;
        modelData.vertices[baseVertex + 2].color = color;
        modelData.vertices[baseVertex + 6].color = color;
        modelData.vertices[baseVertex + 7].color = color;
        
        // Add indices
        modelData.indices.push_back(baseVertex + 3);
        modelData.indices.push_back(baseVertex + 2);
        modelData.indices.push_back(baseVertex + 6);
        modelData.indices.push_back(baseVertex + 6);
        modelData.indices.push_back(baseVertex + 7);
        modelData.indices.push_back(baseVertex + 3);
    }
    
    // Bottom face (y = min)
    if (element.faces.find("down") != element.faces.end()) {
        glm::vec3 color = element.faces.at("down").texture != "#missing" ? 
                          parseColor(element.color) : defaultColor;
        
        // Update vertex colors for this face
        modelData.vertices[baseVertex + 4].color = color;
        modelData.vertices[baseVertex + 5].color = color;
        modelData.vertices[baseVertex + 1].color = color;
        modelData.vertices[baseVertex + 0].color = color;
        
        // Add indices
        modelData.indices.push_back(baseVertex + 4);
        modelData.indices.push_back(baseVertex + 5);
        modelData.indices.push_back(baseVertex + 1);
        modelData.indices.push_back(baseVertex + 1);
        modelData.indices.push_back(baseVertex + 0);
        modelData.indices.push_back(baseVertex + 4);
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