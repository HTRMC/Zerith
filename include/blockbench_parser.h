#pragma once

#include "blockbench_model.h"
#include "logger.h"
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
        
        // Extract textures object
        size_t texturesPos = jsonString.find("\"textures\"");
        if (texturesPos != std::string::npos) {
            size_t objStart = jsonString.find("{", texturesPos);
            if (objStart != std::string::npos) {
                // Find matching closing brace
                int braceCount = 1;
                size_t objEnd = objStart + 1;
                while (objEnd < jsonString.length() && braceCount > 0) {
                    if (jsonString[objEnd] == '{') braceCount++;
                    else if (jsonString[objEnd] == '}') braceCount--;
                    objEnd++;
                }
                
                std::string texturesJson = jsonString.substr(objStart, objEnd - objStart);
                
                // Parse texture entries
                size_t currentPos = 0;
                while (true) {
                    // Find next texture key
                    size_t keyStart = texturesJson.find("\"", currentPos);
                    if (keyStart == std::string::npos) break;
                    keyStart++; // Skip opening quote
                    
                    size_t keyEnd = texturesJson.find("\"", keyStart);
                    if (keyEnd == std::string::npos) break;
                    
                    std::string key = texturesJson.substr(keyStart, keyEnd - keyStart);
                    
                    // Skip to colon
                    size_t colonPos = texturesJson.find(":", keyEnd);
                    if (colonPos == std::string::npos) break;
                    
                    // Find value
                    size_t valueStart = texturesJson.find("\"", colonPos);
                    if (valueStart == std::string::npos) break;
                    valueStart++; // Skip opening quote
                    
                    size_t valueEnd = texturesJson.find("\"", valueStart);
                    if (valueEnd == std::string::npos) break;
                    
                    std::string value = texturesJson.substr(valueStart, valueEnd - valueStart);
                    
                    // Store texture mapping
                    model.textures[key] = value;
                    
                    // Move to next entry
                    currentPos = valueEnd + 1;
                    
                    // Skip to next texture or end
                    size_t commaPos = texturesJson.find(",", currentPos);
                    if (commaPos == std::string::npos) break;
                    currentPos = commaPos + 1;
                }
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error parsing Blockbench model: %s", e.what());
    }
    
    return model;
}

// Parse a Blockbench model from file
inline BlockbenchModel::Model parseFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open Blockbench model file: %s", filename.c_str());
        return BlockbenchModel::Model{};
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    return parseFromString(content);
}

// Helper function to resolve texture references
inline std::string resolveTextureReference(const std::string& reference, const std::unordered_map<std::string, std::string>& textures) {
    if (reference.empty() || reference[0] != '#') {
        // Not a reference, return as-is
        return reference;
    }
    
    // Remove the # prefix
    std::string key = reference.substr(1);
    
    // Look up in texture map
    auto it = textures.find(key);
    if (it != textures.end()) {
        return it->second;
    }
    
    // Reference not found, return original
    return reference;
}

// Resolve all texture references in a model
inline void resolveModelTextures(BlockbenchModel::Model& model) {
    for (auto& element : model.elements) {
        // Resolve texture references for each face
        element.down.texture = resolveTextureReference(element.down.texture, model.textures);
        element.up.texture = resolveTextureReference(element.up.texture, model.textures);
        element.north.texture = resolveTextureReference(element.north.texture, model.textures);
        element.south.texture = resolveTextureReference(element.south.texture, model.textures);
        element.west.texture = resolveTextureReference(element.west.texture, model.textures);
        element.east.texture = resolveTextureReference(element.east.texture, model.textures);
    }
}

// Parse a Blockbench model with recursive parent model resolution
inline BlockbenchModel::Model parseFromFileWithParents(const std::string& filename) {
    // Parse the main model
    BlockbenchModel::Model model = parseFromFile(filename);
    
    // Recursively resolve parent models until we find elements
    std::string currentParent = model.parent;
    while (!currentParent.empty() && model.elements.empty()) {
        // Remove minecraft: namespace and block/ prefix if present
        std::string parentName = currentParent;
        if (parentName.find("minecraft:block/") == 0) {
            parentName = parentName.substr(16); // Remove "minecraft:block/"
        } else if (parentName.find("minecraft:") == 0) {
            parentName = parentName.substr(10); // Remove "minecraft:"
        }
        std::string parentPath = "assets/" + parentName + ".json";
        
        LOG_TRACE("Loading parent model: %s", parentPath.c_str());
        BlockbenchModel::Model parentModel = parseFromFileWithParents(parentPath);
        
        // If the current model has no elements, inherit from parent
        if (model.elements.empty() && !parentModel.elements.empty()) {
            model.elements = parentModel.elements;
            LOG_TRACE("Inherited %zu elements from parent model", parentModel.elements.size());
        }
        
        // Merge textures from parent (parent textures are overridden by child)
        for (const auto& parentTexture : parentModel.textures) {
            if (model.textures.find(parentTexture.first) == model.textures.end()) {
                model.textures[parentTexture.first] = parentTexture.second;
            }
        }
        
        // Move to next parent level
        currentParent = parentModel.parent;
    }
    
    // Resolve all texture references in the model
    resolveModelTextures(model);
    
    // Debug: Print resolved textures
    LOG_TRACE("Resolved textures for %s", filename.c_str());
    for (const auto& element : model.elements) {
        if (!element.north.texture.empty()) {
            LOG_TRACE("  North face: %s", element.north.texture.c_str());
        }
        if (!element.south.texture.empty()) {
            LOG_TRACE("  South face: %s", element.south.texture.c_str());
        }
        if (!element.west.texture.empty()) {
            LOG_TRACE("  West face: %s", element.west.texture.c_str());
        }
        if (!element.east.texture.empty()) {
            LOG_TRACE("  East face: %s", element.east.texture.c_str());
        }
        if (!element.up.texture.empty()) {
            LOG_TRACE("  Up face: %s", element.up.texture.c_str());
        }
        if (!element.down.texture.empty()) {
            LOG_TRACE("  Down face: %s", element.down.texture.c_str());
        }
    }
    
    return model;
}

} // namespace BlockbenchParser