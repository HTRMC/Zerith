#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <simdjson.h>
#include <glm/glm.hpp>

struct Face {
    std::string texture;
    std::optional<std::string> cullface;
    glm::vec2 uv[4]; // UV coordinates for the 4 vertices of the face
};

struct Element {
    glm::vec3 from;
    glm::vec3 to;
    std::unordered_map<std::string, Face> faces; // down, up, north, south, west, east
};

struct BlockModel {
    std::optional<std::string> parent;
    std::unordered_map<std::string, std::string> textures;
    std::vector<Element> elements;
};

class ModelLoader {
public:
    ModelLoader(const std::string& assetsPath);
    
    // Load a model by path (e.g., "block/oak_planks")
    std::optional<BlockModel> loadModel(const std::string& modelPath);
    
    // Convert blockbench coordinates (0-16) to Vulkan coordinates (0-1)
    static glm::vec3 blockbenchToVulkan(const glm::vec3& blockbenchCoords);
    
    // Convert a single blockbench coordinate value to Vulkan
    static float blockbenchToVulkan(float blockbenchValue);

private:
    std::string assetsPath;
    std::unordered_map<std::string, BlockModel> modelCache;
    
    // Parse a JSON model file
    std::optional<BlockModel> parseModelFile(const std::string& filePath);
    
    // Resolve parent hierarchy for a model
    void resolveParentHierarchy(BlockModel& model, const std::string& currentModelPath);
    
    // Parse elements from JSON
    std::vector<Element> parseElements(simdjson::dom::array elementsArray);
    
    // Parse textures from JSON
    std::unordered_map<std::string, std::string> parseTextures(simdjson::dom::object texturesObject);
    
    // Parse a face from JSON
    Face parseFace(simdjson::dom::object faceObject);
    
    // Get the full file path for a model
    std::string getModelFilePath(const std::string& modelPath);
};