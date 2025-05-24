#pragma once

#include "blockbench_model.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

namespace BlockbenchParser {

// Simple JSON parsing helper functions
namespace JsonHelper {
    
    // Extract string value from JSON
    inline std::string extractString(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return "";
        
        size_t colonPos = json.find(":", keyPos);
        if (colonPos == std::string::npos) return "";
        
        size_t startQuote = json.find("\"", colonPos);
        if (startQuote == std::string::npos) return "";
        
        size_t endQuote = json.find("\"", startQuote + 1);
        if (endQuote == std::string::npos) return "";
        
        return json.substr(startQuote + 1, endQuote - startQuote - 1);
    }
    
    // Extract array of numbers [x, y, z]
    inline glm::vec3 extractVec3(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return glm::vec3(0.0f);
        
        size_t colonPos = json.find(":", keyPos);
        if (colonPos == std::string::npos) return glm::vec3(0.0f);
        
        size_t arrayStart = json.find("[", colonPos);
        if (arrayStart == std::string::npos) return glm::vec3(0.0f);
        
        size_t arrayEnd = json.find("]", arrayStart);
        if (arrayEnd == std::string::npos) return glm::vec3(0.0f);
        
        std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
        
        // Parse the three numbers
        std::istringstream iss(arrayContent);
        std::string item;
        glm::vec3 result(0.0f);
        int index = 0;
        
        while (std::getline(iss, item, ',') && index < 3) {
            // Remove whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            
            try {
                result[index] = std::stof(item);
            } catch (...) {
                result[index] = 0.0f;
            }
            index++;
        }
        
        return result;
    }
    
    // Extract array of 4 UV coordinates [u1, v1, u2, v2]
    inline glm::vec4 extractVec4(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return glm::vec4(0.0f, 0.0f, 16.0f, 16.0f);
        
        size_t colonPos = json.find(":", keyPos);
        if (colonPos == std::string::npos) return glm::vec4(0.0f, 0.0f, 16.0f, 16.0f);
        
        size_t arrayStart = json.find("[", colonPos);
        if (arrayStart == std::string::npos) return glm::vec4(0.0f, 0.0f, 16.0f, 16.0f);
        
        size_t arrayEnd = json.find("]", arrayStart);
        if (arrayEnd == std::string::npos) return glm::vec4(0.0f, 0.0f, 16.0f, 16.0f);
        
        std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
        
        // Parse the four numbers
        std::istringstream iss(arrayContent);
        std::string item;
        glm::vec4 result(0.0f, 0.0f, 16.0f, 16.0f);
        int index = 0;
        
        while (std::getline(iss, item, ',') && index < 4) {
            // Remove whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            
            try {
                result[index] = std::stof(item);
            } catch (...) {
                result[index] = (index < 2) ? 0.0f : 16.0f;
            }
            index++;
        }
        
        return result;
    }
    
    // Extract face object from JSON
    inline BlockbenchModel::Face extractFace(const std::string& json, const std::string& faceKey) {
        BlockbenchModel::Face face;
        
        std::string searchKey = "\"" + faceKey + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return face;
        
        size_t colonPos = json.find(":", keyPos);
        if (colonPos == std::string::npos) return face;
        
        size_t objStart = json.find("{", colonPos);
        if (objStart == std::string::npos) return face;
        
        // Find matching closing brace
        int braceCount = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < json.length() && braceCount > 0) {
            if (json[objEnd] == '{') braceCount++;
            else if (json[objEnd] == '}') braceCount--;
            objEnd++;
        }
        
        std::string faceJson = json.substr(objStart, objEnd - objStart);
        
        // Extract face properties
        face.texture = extractString(faceJson, "texture");
        face.cullface = extractString(faceJson, "cullface");
        
        // Extract UV coordinates if present
        face.uv = extractVec4(faceJson, "uv");
        
        return face;
    }
}

// Parse a Blockbench model from JSON string
inline BlockbenchModel::Model parseFromString(const std::string& jsonString) {
    BlockbenchModel::Model model;
    
    try {
        // Extract parent
        model.parent = JsonHelper::extractString(jsonString, "parent");
        
        // Extract elements array
        size_t elementsPos = jsonString.find("\"elements\"");
        if (elementsPos != std::string::npos) {
            size_t arrayStart = jsonString.find("[", elementsPos);
            if (arrayStart != std::string::npos) {
                size_t arrayEnd = arrayStart + 1;
                int bracketCount = 1;
                
                // Find matching closing bracket
                while (arrayEnd < jsonString.length() && bracketCount > 0) {
                    if (jsonString[arrayEnd] == '[') bracketCount++;
                    else if (jsonString[arrayEnd] == ']') bracketCount--;
                    arrayEnd++;
                }
                
                std::string elementsArray = jsonString.substr(arrayStart + 1, arrayEnd - arrayStart - 2);
                
                // Parse all elements in the array
                size_t currentPos = 0;
                while (true) {
                    size_t elementStart = elementsArray.find("{", currentPos);
                    if (elementStart == std::string::npos) break;
                    
                    size_t elementEnd = elementStart + 1;
                    int braceCount = 1;
                    
                    // Find matching closing brace
                    while (elementEnd < elementsArray.length() && braceCount > 0) {
                        if (elementsArray[elementEnd] == '{') braceCount++;
                        else if (elementsArray[elementEnd] == '}') braceCount--;
                        elementEnd++;
                    }
                    
                    std::string elementJson = elementsArray.substr(elementStart, elementEnd - elementStart);
                    
                    // Parse element
                    BlockbenchModel::Element element;
                    element.from = JsonHelper::extractVec3(elementJson, "from");
                    element.to = JsonHelper::extractVec3(elementJson, "to");
                    
                    // Parse faces
                    size_t facesPos = elementJson.find("\"faces\"");
                    if (facesPos != std::string::npos) {
                        size_t facesObjStart = elementJson.find("{", facesPos);
                        if (facesObjStart != std::string::npos) {
                            size_t facesObjEnd = facesObjStart + 1;
                            int faceBraceCount = 1;
                            
                            while (facesObjEnd < elementJson.length() && faceBraceCount > 0) {
                                if (elementJson[facesObjEnd] == '{') faceBraceCount++;
                                else if (elementJson[facesObjEnd] == '}') faceBraceCount--;
                                facesObjEnd++;
                            }
                            
                            std::string facesJson = elementJson.substr(facesObjStart, facesObjEnd - facesObjStart);
                            
                            // Extract each face
                            element.down = JsonHelper::extractFace(facesJson, "down");
                            element.up = JsonHelper::extractFace(facesJson, "up");
                            element.north = JsonHelper::extractFace(facesJson, "north");
                            element.south = JsonHelper::extractFace(facesJson, "south");
                            element.west = JsonHelper::extractFace(facesJson, "west");
                            element.east = JsonHelper::extractFace(facesJson, "east");
                        }
                    }
                    
                    model.elements.push_back(element);
                    
                    // Move to next element
                    currentPos = elementEnd;
                }
            }
        }
        
        // TODO: Extract textures object if needed
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Blockbench model: " << e.what() << std::endl;
    }
    
    return model;
}

// Parse a Blockbench model from file
inline BlockbenchModel::Model parseFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open Blockbench model file: " << filename << std::endl;
        return BlockbenchModel::Model{};
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    return parseFromString(content);
}

// Parse a Blockbench model with parent model resolution
inline BlockbenchModel::Model parseFromFileWithParents(const std::string& filename) {
    // Parse the main model
    BlockbenchModel::Model model = parseFromFile(filename);
    
    // If the model has a parent, load and merge it
    if (!model.parent.empty()) {
        std::string parentPath = "assets/" + model.parent + ".json";
        
        std::cout << "Loading parent model: " << parentPath << std::endl;
        BlockbenchModel::Model parentModel = parseFromFile(parentPath);
        
        // If the current model has no elements, inherit from parent
        if (model.elements.empty() && !parentModel.elements.empty()) {
            model.elements = parentModel.elements;
            std::cout << "Inherited " << parentModel.elements.size() << " elements from parent model" << std::endl;
        }
        
        // Merge textures from parent (parent textures are overridden by child)
        for (const auto& parentTexture : parentModel.textures) {
            if (model.textures.find(parentTexture.first) == model.textures.end()) {
                model.textures[parentTexture.first] = parentTexture.second;
            }
        }
    }
    
    return model;
}

} // namespace BlockbenchParser