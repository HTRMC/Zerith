#include "model_loader.h"
#include <fstream>
#include <iostream>
#include <filesystem>

ModelLoader::ModelLoader(const std::string& assetsPath) : assetsPath(assetsPath) {}

std::optional<BlockModel> ModelLoader::loadModel(const std::string& modelPath) {
    // Check cache first
    if (modelCache.find(modelPath) != modelCache.end()) {
        return modelCache[modelPath];
    }
    
    // Load the model file
    auto model = parseModelFile(getModelFilePath(modelPath));
    if (!model.has_value()) {
        return std::nullopt;
    }
    
    // Resolve parent hierarchy
    resolveParentHierarchy(*model, modelPath);
    
    // Cache the model
    modelCache[modelPath] = *model;
    
    return model;
}

glm::vec3 ModelLoader::blockbenchToVulkan(const glm::vec3& blockbenchCoords) {
    return blockbenchCoords / 16.0f;
}

float ModelLoader::blockbenchToVulkan(float blockbenchValue) {
    return blockbenchValue / 16.0f;
}

std::optional<BlockModel> ModelLoader::parseModelFile(const std::string& filePath) {
    // Check if file exists
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Model file not found: " << filePath << std::endl;
        return std::nullopt;
    }
    
    // Read file content
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open model file: " << filePath << std::endl;
        return std::nullopt;
    }
    
    std::string jsonContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Parse JSON using simdjson
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(jsonContent).get(doc);
    
    if (error) {
        std::cerr << "Failed to parse JSON file: " << filePath << " - " << error << std::endl;
        return std::nullopt;
    }
    
    BlockModel model;
    
    // Parse parent
    simdjson::dom::element parentElement;
    if (!doc["parent"].get(parentElement)) {
        std::string_view parentView;
        if (!parentElement.get(parentView)) {
            model.parent = std::string(parentView);
        }
    }
    
    // Parse textures
    simdjson::dom::element texturesElement;
    if (!doc["textures"].get(texturesElement)) {
        simdjson::dom::object texturesObject;
        if (!texturesElement.get(texturesObject)) {
            model.textures = parseTextures(texturesObject);
        }
    }
    
    // Parse elements
    simdjson::dom::element elementsElement;
    if (!doc["elements"].get(elementsElement)) {
        simdjson::dom::array elementsArray;
        if (!elementsElement.get(elementsArray)) {
            model.elements = parseElements(elementsArray);
        }
    }
    
    return model;
}

void ModelLoader::resolveParentHierarchy(BlockModel& model, const std::string& currentModelPath) {
    if (!model.parent.has_value()) {
        return; // No parent to resolve
    }
    
    std::string parentPath = *model.parent;
    
    // Handle minecraft: prefix by removing it
    if (parentPath.starts_with("minecraft:")) {
        parentPath = parentPath.substr(10); // Remove "minecraft:"
    }
    
    // Load parent model
    auto parentModel = loadModel(parentPath);
    if (!parentModel.has_value()) {
        std::cerr << "Failed to load parent model: " << parentPath << " for model: " << currentModelPath << std::endl;
        return;
    }
    
    // Inherit from parent (child overrides parent)
    
    // Inherit textures (child textures override parent textures)
    for (const auto& [key, value] : parentModel->textures) {
        if (model.textures.find(key) == model.textures.end()) {
            model.textures[key] = value;
        }
    }
    
    // Inherit elements if current model has no elements
    if (model.elements.empty()) {
        model.elements = parentModel->elements;
    }
}

std::vector<Element> ModelLoader::parseElements(simdjson::dom::array elementsArray) {
    std::vector<Element> elements;
    
    for (auto elementValue : elementsArray) {
        simdjson::dom::object elementObject;
        if (elementValue.get(elementObject)) {
            continue; // Skip invalid elements
        }
        
        Element element;
        
        // Parse "from" coordinates
        simdjson::dom::element fromElement;
        if (!elementObject["from"].get(fromElement)) {
            simdjson::dom::array fromArray;
            if (!fromElement.get(fromArray)) {
                auto fromIter = fromArray.begin();
                double x, y, z;
                if (fromIter != fromArray.end() && !(*fromIter).get(x)) fromIter++;
                if (fromIter != fromArray.end() && !(*fromIter).get(y)) fromIter++;
                if (fromIter != fromArray.end() && !(*fromIter).get(z)) fromIter++;
                element.from = glm::vec3(x, y, z);
            }
        }
        
        // Parse "to" coordinates
        simdjson::dom::element toElement;
        if (!elementObject["to"].get(toElement)) {
            simdjson::dom::array toArray;
            if (!toElement.get(toArray)) {
                auto toIter = toArray.begin();
                double x, y, z;
                if (toIter != toArray.end() && !(*toIter).get(x)) toIter++;
                if (toIter != toArray.end() && !(*toIter).get(y)) toIter++;
                if (toIter != toArray.end() && !(*toIter).get(z)) toIter++;
                element.to = glm::vec3(x, y, z);
            }
        }
        
        // Parse faces
        simdjson::dom::element facesElement;
        if (!elementObject["faces"].get(facesElement)) {
            simdjson::dom::object facesObject;
            if (!facesElement.get(facesObject)) {
                for (auto [faceKey, faceValue] : facesObject) {
                    simdjson::dom::object faceObject;
                    if (!faceValue.get(faceObject)) {
                        std::string faceKeyStr(faceKey);
                        element.faces[faceKeyStr] = parseFace(faceObject);
                    }
                }
            }
        }
        
        elements.push_back(element);
    }
    
    return elements;
}

std::unordered_map<std::string, std::string> ModelLoader::parseTextures(simdjson::dom::object texturesObject) {
    std::unordered_map<std::string, std::string> textures;
    
    for (auto [key, value] : texturesObject) {
        std::string_view textureValue;
        if (!value.get(textureValue)) {
            std::string keyStr(key);
            textures[keyStr] = std::string(textureValue);
        }
    }
    
    return textures;
}

Face ModelLoader::parseFace(simdjson::dom::object faceObject) {
    Face face;
    
    // Parse texture reference
    simdjson::dom::element textureElement;
    if (!faceObject["texture"].get(textureElement)) {
        std::string_view textureView;
        if (!textureElement.get(textureView)) {
            face.texture = std::string(textureView);
        }
    }
    
    // Parse cullface (optional)
    simdjson::dom::element cullfaceElement;
    if (!faceObject["cullface"].get(cullfaceElement)) {
        std::string_view cullfaceView;
        if (!cullfaceElement.get(cullfaceView)) {
            face.cullface = std::string(cullfaceView);
        }
    }
    
    // Parse UV coordinates (optional, defaults to 0,0,16,16 if not specified)
    simdjson::dom::element uvElement;
    if (!faceObject["uv"].get(uvElement)) {
        simdjson::dom::array uvArray;
        if (!uvElement.get(uvArray)) {
            auto uvIter = uvArray.begin();
            double u1 = 0, v1 = 0, u2 = 16, v2 = 16;
            if (uvIter != uvArray.end() && !(*uvIter).get(u1)) uvIter++;
            if (uvIter != uvArray.end() && !(*uvIter).get(v1)) uvIter++;
            if (uvIter != uvArray.end() && !(*uvIter).get(u2)) uvIter++;
            if (uvIter != uvArray.end() && !(*uvIter).get(v2)) uvIter++;
            
            // Convert to normalized UV coordinates (0-1 range)
            face.uv[0] = glm::vec2(u1 / 16.0f, v1 / 16.0f); // Top-left
            face.uv[1] = glm::vec2(u2 / 16.0f, v1 / 16.0f); // Top-right
            face.uv[2] = glm::vec2(u2 / 16.0f, v2 / 16.0f); // Bottom-right
            face.uv[3] = glm::vec2(u1 / 16.0f, v2 / 16.0f); // Bottom-left
        }
    } else {
        // Default UV coordinates for full texture
        face.uv[0] = glm::vec2(0.0f, 0.0f); // Top-left
        face.uv[1] = glm::vec2(1.0f, 0.0f); // Top-right
        face.uv[2] = glm::vec2(1.0f, 1.0f); // Bottom-right
        face.uv[3] = glm::vec2(0.0f, 1.0f); // Bottom-left
    }
    
    return face;
}

std::string ModelLoader::getModelFilePath(const std::string& modelPath) {
    return assetsPath + "/models/" + modelPath + ".json";
}