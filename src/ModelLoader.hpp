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
    std::vector<glm::vec2> uvs;
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
    uint32_t textureId = 0;
    std::unordered_map<std::string, std::string> textureMap;  // Map texture references to real paths
    std::unordered_map<std::string, std::string> textureReferences;  // For resolving #references
};

class ModelLoader {
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    // Load a model from a BlockBench JSON file
    std::optional<ModelData> loadModel(const std::string& filename);

private:
    // Process a model file, handling inheritance and texture references
    bool processModelFile(const std::string& filename, ModelData& modelData);

    // Resolve all texture references in the model
    void resolveTextureReferences(ModelData& modelData);

    // Recursively resolve a single texture reference
    bool resolveTextureReference(const std::string& key, const std::string& refName, ModelData& modelData);

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