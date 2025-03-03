#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <optional>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

// Forward declarations
struct Vertex;

// Model data structures
struct Face {
    std::vector<uint16_t> indices;
    glm::vec3 color;
    std::string texture;
};

struct Element {
    glm::vec3 from;
    glm::vec3 to;
    int color;
    std::unordered_map<std::string, Face> faces;
};

struct ModelData {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    bool loaded = false;
    std::string name;
    std::vector<Element> elements;
};

class ModelLoader {
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    // Load a model from a BlockBench JSON file
    std::optional<ModelData> loadModel(const std::string& filename);

private:
    // Convert BlockBench coordinates to our coordinate system
    glm::vec3 convertCoordinates(const glm::vec3& bbCoords);
    
    // Process a single BlockBench element
    void processElement(Element& element, ModelData& modelData, size_t elementIndex);
    
    // Create vertices and indices from a BlockBench element
    void createElementGeometry(const Element& element, ModelData& modelData);
    
    // Parse a color index to RGB
    glm::vec3 parseColor(int colorIndex);
    
    // Parse UV coordinates
    std::vector<glm::vec2> parseUVs(const nlohmann::json& uvJson);
};