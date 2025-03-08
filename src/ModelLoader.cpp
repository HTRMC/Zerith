#include "ModelLoader.hpp"
#include "VulkanApp.hpp"
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <filesystem>

namespace fs = std::filesystem;

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

// Helper function to resolve resource paths
static std::string resolveResourcePath(const std::string& path) {
    // Handle Minecraft namespaced paths like "minecraft:block/oak_planks"
    std::string resolvedPath = path;

    // Replace namespace separator
    size_t colonPos = resolvedPath.find(':');
    if (colonPos != std::string::npos) {
        // Extract namespace and the rest of the path
        std::string namespace_ = resolvedPath.substr(0, colonPos);
        std::string resourcePath = resolvedPath.substr(colonPos + 1);

        // Format as assets/namespace/models/resourcePath
        resolvedPath = "assets/" + namespace_ + "/models/" + resourcePath;
    } else {
        // If no namespace, assume "minecraft" namespace
        resolvedPath = "assets/minecraft/models/" + resolvedPath;
    }

    return resolvedPath;
}

std::optional<ModelData> ModelLoader::loadModel(const std::string& filename) {
    try {
        // Handle the filename - if it doesn't start with "assets/", assume a simple path like "block/oak_stairs"
        std::string fullPath = filename;
        if (filename.find("assets/") != 0) {
            // Handle paths like "block/oak_stairs" or "minecraft:block/oak_stairs"
            fullPath = resolveResourcePath(filename) + ".json";
            std::cout << "Resolved model path: " << filename << " -> " << fullPath << std::endl;
        }

        // Create model data
        ModelData modelData;
        modelData.name = filename;

        // Process the model file and its inherited properties
        if (!processModelFile(fullPath, modelData)) {
            std::cerr << "Failed to process model file: " << fullPath << std::endl;
            return std::nullopt;
        }

        // Resolve texture references
        resolveTextureReferences(modelData);

        // If we have elements, generate vertices and indices
        if (!modelData.elements.empty()) {
            for (size_t i = 0; i < modelData.elements.size(); i++) {
                processElement(modelData.elements[i], modelData, i);
            }

            // If we have vertices and indices, mark as loaded
            if (!modelData.vertices.empty() && !modelData.indices.empty()) {
                modelData.loaded = true;
                return modelData;
            }
        } else {
            std::cerr << "Model file does not contain elements array" << std::endl;
            return std::nullopt;
        }

        std::cerr << "Failed to generate geometry for model: " << filename << std::endl;
        return std::nullopt;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading model " << filename << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool ModelLoader::processModelFile(const std::string& filename, ModelData& modelData) {
    // Read the file
    std::string jsonContent;
    try {
        std::cout << "Attempting to load model file: " << filename << std::endl;

        // Check if file exists
        if (!fs::exists(filename)) {
            std::cerr << "Model file does not exist: " << filename << std::endl;
            return false;
        }

        jsonContent = readFileToString(filename);
    } catch (const std::exception& e) {
        std::cerr << "Failed to read model file " << filename << ": " << e.what() << std::endl;
        return false;
    }

    // Parse the JSON
    nlohmann::json modelJson;
    try {
        modelJson = nlohmann::json::parse(jsonContent);
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse JSON from " << filename << ": " << e.what() << std::endl;
        return false;
    }

    // Check for parent model
    if (modelJson.contains("parent") && modelJson["parent"].is_string()) {
        std::string parentPath = modelJson["parent"].get<std::string>();
        std::string fullParentPath = resolveResourcePath(parentPath) + ".json";

        std::cout << "Loading parent model: " << parentPath << " -> " << fullParentPath << std::endl;

        // Try to load the parent model first
        ModelData parentModelData;
        if (!processModelFile(fullParentPath, parentModelData)) {
            std::cerr << "Failed to load parent model: " << fullParentPath << std::endl;
            return false;
        }

        // Copy parent properties that aren't overridden by child
        if (modelData.elements.empty()) {
            modelData.elements = parentModelData.elements;
        }

        // Merge textures - parent textures provide defaults
        for (const auto& [key, value] : parentModelData.textureMap) {
            if (modelData.textureMap.find(key) == modelData.textureMap.end()) {
                modelData.textureMap[key] = value;
            }
        }

        // Copy texture references from parent as well
        for (const auto& [key, value] : parentModelData.textureReferences) {
            if (modelData.textureReferences.find(key) == modelData.textureReferences.end()) {
                modelData.textureReferences[key] = value;
            }
        }
    }

    // Extract texture information
    if (modelJson.contains("textures") && modelJson["textures"].is_object()) {
        for (auto& [key, value] : modelJson["textures"].items()) {
            if (value.is_string()) {
                std::string texturePath = value.get<std::string>();

                // Handle texture references (e.g., "#side")
                if (!texturePath.empty() && texturePath[0] == '#') {
                    // This is a reference to another texture defined in this model
                    // Store the reference to be resolved later
                    modelData.textureReferences[key] = texturePath.substr(1);  // Remove the '#'
                    std::cout << "Texture reference: " << key << " -> #" << texturePath.substr(1) << std::endl;
                } else {
                    // This is a direct texture path
                    std::string namespace_ = "minecraft"; // Default namespace
                    std::string texPath = texturePath;

                    // Check if path has namespace
                    size_t colonPos = texturePath.find(':');
                    if (colonPos != std::string::npos) {
                        namespace_ = texturePath.substr(0, colonPos);
                        texPath = texturePath.substr(colonPos + 1);
                    }

                    // Format as assets/namespace/textures/path.png
                    std::string fullTexturePath = "assets/" + namespace_ + "/textures/" + texPath + ".png";
                    modelData.textureMap[key] = fullTexturePath;
                    std::cout << "Texture mapping: " << key << " -> " << fullTexturePath << std::endl;
                }
            }
        }
    }

    // Process elements from this model (overriding parent elements if any)
    if (modelJson.contains("elements") && modelJson["elements"].is_array()) {
        // If this model has its own elements, replace the ones from the parent
        if (!modelData.elements.empty() && !modelJson["elements"].empty()) {
            modelData.elements.clear();
        }

        for (const auto& elementJson : modelJson["elements"]) {
            Element element;

            // Parse 'from' coordinates
            if (elementJson.contains("from") && elementJson["from"].is_array() && elementJson["from"].size() == 3) {
                element.from.x = elementJson["from"][0];
                element.from.y = elementJson["from"][1];
                element.from.z = elementJson["from"][2];
                // Convert from BlockBench coordinates (0-16) to normalized (0-1)
                element.from = convertCoordinates(element.from);
            }

            // Parse 'to' coordinates
            if (elementJson.contains("to") && elementJson["to"].is_array() && elementJson["to"].size() == 3) {
                element.to.x = elementJson["to"][0];
                element.to.y = elementJson["to"][1];
                element.to.z = elementJson["to"][2];
                // Convert from BlockBench coordinates (0-16) to normalized (0-1)
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
                    if (faceData.contains("texture") && faceData["texture"].is_string()) {
                        face.texture = faceData["texture"];
                    }

                    // Parse UVs if present
                    if (faceData.contains("uv") && faceData["uv"].is_array()) {
                        face.uvs = parseUVs(faceData["uv"]);
                    }

                    // Store the face
                    element.faces[faceName] = face;
                }
            }

            // Add the element to the model
            modelData.elements.push_back(element);
        }
    }

    return true;
}

void ModelLoader::resolveTextureReferences(ModelData& modelData) {
    // First, make a copy of the references to avoid modification during iteration
    auto references = modelData.textureReferences;

    // Resolve references depth-first
    for (const auto& [key, refName] : references) {
        resolveTextureReference(key, refName, modelData);
    }
}

bool ModelLoader::resolveTextureReference(const std::string& key, const std::string& refName, ModelData& modelData) {
    // Check if this reference has already been resolved
    if (modelData.textureMap.find(key) != modelData.textureMap.end()) {
        return true;
    }

    // Check if the reference points to a direct texture path
    if (modelData.textureMap.find(refName) != modelData.textureMap.end()) {
        modelData.textureMap[key] = modelData.textureMap[refName];
        std::cout << "Resolved texture reference: " << key << " -> " << modelData.textureMap[key] << std::endl;
        return true;
    }

    // Check if the reference points to another reference
    if (modelData.textureReferences.find(refName) != modelData.textureReferences.end()) {
        std::string nextRef = modelData.textureReferences[refName];
        // Prevent infinite recursion
        if (nextRef == refName) {
            std::cerr << "Circular texture reference detected: " << key << " -> #" << refName << std::endl;
            return false;
        }
        // Resolve the nested reference
        if (resolveTextureReference(refName, nextRef, modelData)) {
            // Now the intermediate reference is resolved, so resolve this reference
            return resolveTextureReference(key, refName, modelData);
        }
    }

    std::cerr << "Could not resolve texture reference: " << key << " -> #" << refName << std::endl;
    return false;
}

glm::vec3 ModelLoader::convertCoordinates(const glm::vec3& bbCoords) {
    // Convert from BlockBench coordinates (0-16) to normalized (0.0 to 1.0)
    return glm::vec3(
        (bbCoords.x / 16.0f),   // X
        (bbCoords.z / 16.0f),   // Y
        (bbCoords.y / 16.0f)    // Z
    );
}

std::vector<glm::vec2> ModelLoader::parseUVs(const nlohmann::json& uvJson) {
    std::vector<glm::vec2> result;
    if (uvJson.size() == 4) {
        // BlockBench UVs are typically [x1, y1, x2, y2]
        float minU = uvJson[0].get<float>() / 16.0f;
        float minV = uvJson[1].get<float>() / 16.0f;
        float maxU = uvJson[2].get<float>() / 16.0f;
        float maxV = uvJson[3].get<float>() / 16.0f;

        // Add corners in counter-clockwise order for proper face orientation
        // This may need adjustment depending on your face construction
        result.push_back({minU, maxV}); // Top-left
        result.push_back({maxU, maxV}); // Top-right
        result.push_back({maxU, minV}); // Bottom-right
        result.push_back({minU, minV}); // Bottom-left
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
    float y_min = element.from.y;
    float y_max = element.to.y;
    float z_min = element.from.z;
    float z_max = element.to.z;

    // --- FRONT FACE (south) ---
    if (element.faces.find("south") != element.faces.end()) {
        const Face& face = element.faces.at("south");

        // Resolve texture reference
        std::string textureName;
        if (!face.texture.empty() && face.texture[0] == '#') {
            textureName = face.texture.substr(1); // Remove the '#'
        }

        glm::vec3 faceColor = defaultColor;

        // Get UVs or use default
        std::vector<glm::vec2> uvs = face.uvs.size() == 4 ? face.uvs : std::vector<glm::vec2>{
            {0.0f, 1.0f}, // Bottom-left
            {1.0f, 1.0f}, // Bottom-right
            {1.0f, 0.0f}, // Top-right
            {0.0f, 0.0f}  // Top-left
        };

        Vertex frontBottomLeft  = { { x_min, y_max, z_min }, faceColor, uvs[0] };
        Vertex frontBottomRight = { { x_max, y_max, z_min }, faceColor, uvs[1] };
        Vertex frontTopRight    = { { x_max, y_max, z_max }, faceColor, uvs[2] };
        Vertex frontTopLeft     = { { x_min, y_max, z_max }, faceColor, uvs[3] };

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

    // --- BACK FACE (north) ---
    if (element.faces.find("north") != element.faces.end()) {
        const Face& face = element.faces.at("north");

        // Resolve texture reference
        std::string textureName;
        if (!face.texture.empty() && face.texture[0] == '#') {
            textureName = face.texture.substr(1); // Remove the '#'
        }

        glm::vec3 faceColor = defaultColor;

        // Get UVs or use default
        std::vector<glm::vec2> uvs = face.uvs.size() == 4 ? face.uvs : std::vector<glm::vec2>{
            {1.0f, 1.0f}, // Bottom-left (reversed for back face)
            {0.0f, 1.0f}, // Bottom-right (reversed for back face)
            {0.0f, 0.0f}, // Top-right (reversed for back face)
            {1.0f, 0.0f}  // Top-left (reversed for back face)
        };

        Vertex backBottomLeft  = { { x_min, y_min, z_min }, faceColor, uvs[0] };
        Vertex backBottomRight = { { x_max, y_min, z_min }, faceColor, uvs[1] };
        Vertex backTopRight    = { { x_max, y_min, z_max }, faceColor, uvs[2] };
        Vertex backTopLeft     = { { x_min, y_min, z_max }, faceColor, uvs[3] };

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

    // --- RIGHT FACE (east) ---
    if (element.faces.find("east") != element.faces.end()) {
        const Face& face = element.faces.at("east");

        // Resolve texture reference
        std::string textureName;
        if (!face.texture.empty() && face.texture[0] == '#') {
            textureName = face.texture.substr(1); // Remove the '#'
        }

        glm::vec3 faceColor = defaultColor;

        // Get UVs or use default
        std::vector<glm::vec2> uvs = face.uvs.size() == 4 ? face.uvs : std::vector<glm::vec2>{
            {0.0f, 1.0f}, // Bottom-left
            {1.0f, 1.0f}, // Bottom-right
            {1.0f, 0.0f}, // Top-right
            {0.0f, 0.0f}  // Top-left
        };

        Vertex frontBottomRight = { { x_max, y_max, z_min }, faceColor, uvs[0] };
        Vertex backBottomRight  = { { x_max, y_min, z_min }, faceColor, uvs[1] };
        Vertex backTopRight     = { { x_max, y_min, z_max }, faceColor, uvs[2] };
        Vertex frontTopRight    = { { x_max, y_max, z_max }, faceColor, uvs[3] };

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

    // --- LEFT FACE (west) ---
    if (element.faces.find("west") != element.faces.end()) {
        const Face& face = element.faces.at("west");

        // Resolve texture reference
        std::string textureName;
        if (!face.texture.empty() && face.texture[0] == '#') {
            textureName = face.texture.substr(1); // Remove the '#'
        }

        glm::vec3 faceColor = defaultColor;

        // Get UVs or use default
        std::vector<glm::vec2> uvs = face.uvs.size() == 4 ? face.uvs : std::vector<glm::vec2>{
            {1.0f, 1.0f}, // Bottom-left (reversed for left face)
            {0.0f, 1.0f}, // Bottom-right (reversed for left face)
            {0.0f, 0.0f}, // Top-right (reversed for left face)
            {1.0f, 0.0f}  // Top-left (reversed for left face)
        };

        Vertex backBottomLeft  = { { x_min, y_min, z_min }, faceColor, uvs[0] };
        Vertex frontBottomLeft = { { x_min, y_max, z_min }, faceColor, uvs[1] };
        Vertex frontTopLeft    = { { x_min, y_max, z_max }, faceColor, uvs[2] };
        Vertex backTopLeft     = { { x_min, y_min, z_max }, faceColor, uvs[3] };

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

    // --- TOP FACE (up) ---
    if (element.faces.find("up") != element.faces.end()) {
        const Face& face = element.faces.at("up");

        // Resolve texture reference
        std::string textureName;
        if (!face.texture.empty() && face.texture[0] == '#') {
            textureName = face.texture.substr(1); // Remove the '#'
        }

        glm::vec3 faceColor = defaultColor;

        // Get UVs or use default
        std::vector<glm::vec2> uvs = face.uvs.size() == 4 ? face.uvs : std::vector<glm::vec2>{
            {0.0f, 0.0f}, // Top-left
            {1.0f, 0.0f}, // Top-right
            {1.0f, 1.0f}, // Bottom-right
            {0.0f, 1.0f}  // Bottom-left
        };

        Vertex frontTopLeft  = { { x_min, y_max, z_max }, faceColor, uvs[0] };
        Vertex frontTopRight = { { x_max, y_max, z_max }, faceColor, uvs[1] };
        Vertex backTopRight  = { { x_max, y_min, z_max }, faceColor, uvs[2] };
        Vertex backTopLeft   = { { x_min, y_min, z_max }, faceColor, uvs[3] };

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

    // --- BOTTOM FACE (down) ---
    if (element.faces.find("down") != element.faces.end()) {
        const Face& face = element.faces.at("down");

        // Resolve texture reference
        std::string textureName;
        if (!face.texture.empty() && face.texture[0] == '#') {
            textureName = face.texture.substr(1); // Remove the '#'
        }

        glm::vec3 faceColor = defaultColor;

        // Get UVs or use default
        std::vector<glm::vec2> uvs = face.uvs.size() == 4 ? face.uvs : std::vector<glm::vec2>{
            {1.0f, 0.0f}, // Bottom-right (flipped for bottom face)
            {0.0f, 0.0f}, // Bottom-left (flipped for bottom face)
            {0.0f, 1.0f}, // Top-left (flipped for bottom face)
            {1.0f, 1.0f}  // Top-right (flipped for bottom face)
        };

        Vertex frontBottomRight = { { x_max, y_max, z_min }, faceColor, uvs[0] };
        Vertex frontBottomLeft  = { { x_min, y_max, z_min }, faceColor, uvs[1] };
        Vertex backBottomLeft   = { { x_min, y_min, z_min }, faceColor, uvs[2] };
        Vertex backBottomRight  = { { x_max, y_min, z_min }, faceColor, uvs[3] };

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