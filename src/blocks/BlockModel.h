// BlockModel.h
#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

struct BlockFace {
    std::string texture;
    bool cullface;
    glm::vec2 uv[4]; // UV coordinates for the face
};

struct BlockElement {
    glm::vec3 from; // Start position (in pixels)
    glm::vec3 to; // End position (in pixels)
    std::map<std::string, BlockFace> faces;
};

class BlockModel {
public:
    std::string parent;
    std::vector<BlockElement> elements;
    std::map<std::string, std::string> textures;

    static std::string resolveParentPath(const std::string& parentName) {
        // Convert Minecraft-style path to file system path
        std::string path = parentName;
        if (path.find("minecraft:") == 0) {
            path = path.substr(10); // Remove "minecraft:"
        }
        return "assets/minecraft/models/" + path + ".json";
    }

    static std::string resolveTextureVariable(const std::string& texture, const std::map<std::string, std::string>& textureMap) {
        if (texture.empty() || texture[0] != '#') return texture;

        std::string key = texture.substr(1); // Remove '#'
        auto it = textureMap.find(key);
        if (it != textureMap.end()) {
            // Recursively resolve texture references
            return resolveTextureVariable(it->second, textureMap);
        }
        return texture;
    }

    // Load model from JSON file
    static BlockModel loadFromJson(const std::string& jsonContent, std::map<std::string, BlockModel>& loadedModels) {
        using json = nlohmann::json;
        BlockModel model;

        try {
            auto j = json::parse(jsonContent);

            // Parse textures if they exist
            if (j.contains("textures")) {
                for (const auto& [key, value] : j["textures"].items()) {
                    model.textures[key] = value.get<std::string>();
                }
            }

            // Load parent first if it exists
            if (j.contains("parent")) {
                std::string parentName = j["parent"].get<std::string>();
                model.parent = parentName;

                // Check if parent is already loaded
                auto it = loadedModels.find(parentName);
                if (it == loadedModels.end()) {
                    // Load parent model
                    std::string parentPath = resolveParentPath(parentName);
                    std::ifstream parentFile(parentPath);
                    if (parentFile.is_open()) {
                        std::string parentContent((std::istreambuf_iterator<char>(parentFile)),
                                               std::istreambuf_iterator<char>());
                        BlockModel parentModel = loadFromJson(parentContent, loadedModels);

                        // Store the loaded parent model
                        loadedModels[parentName] = parentModel;

                        // Inherit elements from parent
                        model.elements = parentModel.elements;

                        // Merge textures, with child overriding parent
                        for (const auto& [key, value] : parentModel.textures) {
                            if (model.textures.find(key) == model.textures.end()) {
                                model.textures[key] = value;
                            }
                        }
                    }
                } else {
                    // Use cached parent model
                    const BlockModel& parentModel = it->second;
                    model.elements = parentModel.elements;

                    // Merge textures
                    for (const auto& [key, value] : parentModel.textures) {
                        if (model.textures.find(key) == model.textures.end()) {
                            model.textures[key] = value;
                        }
                    }
                }
            }

            // Parse elements if they exist in this model
            if (j.contains("elements")) {
                model.elements.clear(); // Clear inherited elements if we have our own
                for (const auto& elem : j["elements"]) {
                    BlockElement element;

                    auto from = elem["from"];
                    element.from = glm::vec3(
                        from[0].get<float>() / 16.0f,
                        from[1].get<float>() / 16.0f,
                        from[2].get<float>() / 16.0f
                    );

                    auto to = elem["to"];
                    element.to = glm::vec3(
                        to[0].get<float>() / 16.0f,
                        to[1].get<float>() / 16.0f,
                        to[2].get<float>() / 16.0f
                    );

                    if (elem.contains("faces")) {
                        auto faces = elem["faces"];
                        for (const auto& [faceName, faceData] : faces.items()) {
                            BlockFace face;
                            std::string rawTexture = faceData["texture"].get<std::string>();
                            face.texture = resolveTextureVariable(rawTexture, model.textures);
                            face.cullface = faceData.contains("cullface");

                            if (faceData.contains("uv")) {
                                auto uv = faceData["uv"];
                                face.uv[0] = glm::vec2(uv[0].get<float>() / 16.0f, uv[1].get<float>() / 16.0f);
                                face.uv[1] = glm::vec2(uv[2].get<float>() / 16.0f, uv[1].get<float>() / 16.0f);
                                face.uv[2] = glm::vec2(uv[2].get<float>() / 16.0f, uv[3].get<float>() / 16.0f);
                                face.uv[3] = glm::vec2(uv[0].get<float>() / 16.0f, uv[3].get<float>() / 16.0f);
                            } else {
                                setDefaultUVs(face, faceName, element.from, element.to);
                            }

                            element.faces[faceName] = face;
                        }
                    }

                    model.elements.push_back(element);
                }
            }

        } catch (const std::exception& e) {
            std::cerr << "Error parsing block model JSON: " << e.what() << std::endl;
        }

        return model;
    }

    // Generate vertex data for rendering
    std::vector<float> generateVertexData() const {
        std::vector<float> vertices;

        for (const auto &element: elements) {
            for (const auto &[faceName, face]: element.faces) {
                auto faceVertices = generateFaceVertices(element, faceName, face);
                vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());
            }
        }

        return vertices;
    }

    std::vector<float> generateFaceVertices(const std::string& face, const glm::mat4& transform) const {
        std::vector<float> vertices;

        for (const auto& element : elements) {
            auto it = element.faces.find(face);
            if (it != element.faces.end()) {
                auto baseVertices = generateFaceVertices(element, face, it->second);

                // Transform vertices by the provided matrix
                for (size_t i = 0; i < baseVertices.size(); i += 9) {
                    glm::vec4 pos(baseVertices[i], baseVertices[i+1], baseVertices[i+2], 1.0f);
                    pos = transform * pos;
                    baseVertices[i] = pos.x;
                    baseVertices[i+1] = pos.y;
                    baseVertices[i+2] = pos.z;
                }

                vertices.insert(vertices.end(), baseVertices.begin(), baseVertices.end());
            }
        }

        return vertices;
    }

private:
    static void setDefaultUVs(BlockFace &face, const std::string &faceName,
                              const glm::vec3 &from, const glm::vec3 &to) {
        // Set default UVs based on the face and element dimensions
        if (faceName == "north" || faceName == "south") {
            face.uv[0] = glm::vec2(from.x, from.y);
            face.uv[1] = glm::vec2(to.x, from.y);
            face.uv[2] = glm::vec2(to.x, to.y);
            face.uv[3] = glm::vec2(from.x, to.y);
        } else if (faceName == "east" || faceName == "west") {
            face.uv[0] = glm::vec2(from.z, from.y);
            face.uv[1] = glm::vec2(to.z, from.y);
            face.uv[2] = glm::vec2(to.z, to.y);
            face.uv[3] = glm::vec2(from.z, to.y);
        } else {
            // up/down
            face.uv[0] = glm::vec2(from.x, from.z);
            face.uv[1] = glm::vec2(to.x, from.z);
            face.uv[2] = glm::vec2(to.x, to.z);
            face.uv[3] = glm::vec2(from.x, to.z);
        }
    }

    static std::vector<float> generateFaceVertices(const BlockElement &element,
                                                   const std::string &faceName,
                                                   const BlockFace &face) {
        std::vector<float> vertices;
        float faceIndex = getFaceIndex(faceName);
        const glm::vec3 color(1.0f);

        if (faceName == "north") {
            // Front face (Z-)
            vertices.insert(vertices.end(), {
                                // First triangle
                                element.from.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex,
                                element.to.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[1].x,
                                face.uv[1].y, faceIndex,
                                element.to.x, element.to.y, element.from.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                // Second triangle
                                element.to.x, element.to.y, element.from.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                element.from.x, element.to.y, element.from.z, color.r, color.g, color.b, face.uv[3].x,
                                face.uv[3].y, faceIndex,
                                element.from.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex
                            });
        } else if (faceName == "south") {
            // Back face (Z+)
            vertices.insert(vertices.end(), {
                                // First triangle
                                element.from.x, element.from.y, element.to.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex,
                                element.from.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[3].x,
                                face.uv[3].y, faceIndex,
                                element.to.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                // Second triangle
                                element.to.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                element.to.x, element.from.y, element.to.z, color.r, color.g, color.b, face.uv[1].x,
                                face.uv[1].y, faceIndex,
                                element.from.x, element.from.y, element.to.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex
                            });
        } else if (faceName == "east") {
            // Right face (X+)
            vertices.insert(vertices.end(), {
                                // First triangle
                                element.to.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex,
                                element.to.x, element.from.y, element.to.z, color.r, color.g, color.b, face.uv[1].x,
                                face.uv[1].y, faceIndex,
                                element.to.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                // Second triangle
                                element.to.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                element.to.x, element.to.y, element.from.z, color.r, color.g, color.b, face.uv[3].x,
                                face.uv[3].y, faceIndex,
                                element.to.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex
                            });
        } else if (faceName == "west") {
            // Left face (X-)
            vertices.insert(vertices.end(), {
                                // First triangle
                                element.from.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex,
                                element.from.x, element.to.y, element.from.z, color.r, color.g, color.b, face.uv[3].x,
                                face.uv[3].y, faceIndex,
                                element.from.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                // Second triangle
                                element.from.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                element.from.x, element.from.y, element.to.z, color.r, color.g, color.b, face.uv[1].x,
                                face.uv[1].y, faceIndex,
                                element.from.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex
                            });
        } else if (faceName == "up") {
            // Top face (Y+)
            vertices.insert(vertices.end(), {
                                // First triangle
                                element.from.x, element.to.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex,
                                element.to.x, element.to.y, element.from.z, color.r, color.g, color.b, face.uv[1].x,
                                face.uv[1].y, faceIndex,
                                element.to.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                // Second triangle
                                element.to.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                element.from.x, element.to.y, element.to.z, color.r, color.g, color.b, face.uv[3].x,
                                face.uv[3].y, faceIndex,
                                element.from.x, element.to.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex
                            });
        } else if (faceName == "down") {
            // Bottom face (Y-)
            vertices.insert(vertices.end(), {
                                // First triangle
                                element.from.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex,
                                element.from.x, element.from.y, element.to.z, color.r, color.g, color.b, face.uv[3].x,
                                face.uv[3].y, faceIndex,
                                element.to.x, element.from.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                // Second triangle
                                element.to.x, element.from.y, element.to.z, color.r, color.g, color.b, face.uv[2].x,
                                face.uv[2].y, faceIndex,
                                element.to.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[1].x,
                                face.uv[1].y, faceIndex,
                                element.from.x, element.from.y, element.from.z, color.r, color.g, color.b, face.uv[0].x,
                                face.uv[0].y, faceIndex
                            });
        }

        return vertices;
    }

    static float getFaceIndex(const std::string &faceName) {
        if (faceName == "up") return 0.0f;
        if (faceName == "down") return 1.0f;
        if (faceName == "north") return 2.0f;
        if (faceName == "south") return 3.0f;
        if (faceName == "east") return 4.0f;
        if (faceName == "west") return 5.0f;
        return 0.0f;
    }

    static void addQuad(std::vector<float> &vertices,
                        const glm::vec3 &min,
                        const glm::vec3 &max,
                        const glm::vec2 uv[4],
                        float faceIndex) {
        // Format: position(3), color(3), texcoord(2), faceindex(1)
        const glm::vec3 color(1.0f);

        // First triangle
        vertices.insert(vertices.end(), {
                            min.x, min.y, min.z, color.r, color.g, color.b, uv[0].x, uv[0].y, faceIndex,
                            max.x, min.y, min.z, color.r, color.g, color.b, uv[1].x, uv[1].y, faceIndex,
                            max.x, max.y, min.z, color.r, color.g, color.b, uv[2].x, uv[2].y, faceIndex
                        });

        // Second triangle
        vertices.insert(vertices.end(), {
                            max.x, max.y, min.z, color.r, color.g, color.b, uv[2].x, uv[2].y, faceIndex,
                            min.x, max.y, min.z, color.r, color.g, color.b, uv[3].x, uv[3].y, faceIndex,
                            min.x, min.y, min.z, color.r, color.g, color.b, uv[0].x, uv[0].y, faceIndex
                        });
    }

    static BlockModel loadFromJson(const std::string& jsonContent) {
        std::map<std::string, BlockModel> loadedModels;
        return loadFromJson(jsonContent, loadedModels);
    }
};
