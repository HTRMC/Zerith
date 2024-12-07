// Chunk.h
#pragma once
#include <vector>
#include "blocks/BlockType.h"
#include "blocks/BlockModel.h"
#include <glm/glm.hpp>

std::string loadFileContent(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

class BlockModelManager {
public:
    static BlockModel& getModel(const std::string& path) {
        static std::map<std::string, BlockModel> modelCache;
        static std::map<std::string, BlockModel> loadedModels;

        auto it = modelCache.find(path);
        if (it != modelCache.end()) {
            return it->second;
        }

        std::string fullPath = "assets/minecraft/models/" + path + ".json";
        std::string jsonContent = loadFileContent(fullPath);
        BlockModel model = BlockModel::loadFromJson(jsonContent, loadedModels);
        return modelCache.emplace(path, std::move(model)).first->second;
    }
};

class Chunk {
public:
    static const int CHUNK_SIZE = 16;
    std::vector<std::vector<std::vector<Block>>> blocks;
    glm::ivec2 position; // Chunk position in world coordinates (x, z)
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
    bool needsRemesh = true;

    Chunk() : position(0, 0) {
        initialize();
    }

    Chunk(int x, int z) : position(x, z) {
        initialize();
    }

    ~Chunk() {
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }
    }

    void initialize() {
        blocks.resize(CHUNK_SIZE,
            std::vector<std::vector<Block>>(CHUNK_SIZE,
                std::vector<Block>(CHUNK_SIZE, Block())
            )
        );
    }

    void generateTerrain() {
        // Basic terrain generation
        for (int y = 0; y <= 2; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    blocks[x][y][z] = Block(BlockType::STONE);
                    blocks[x][y][z].exists = true;
                }
            }
        }

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                blocks[x][3][z] = Block(BlockType::DIRT);
                blocks[x][3][z].exists = true;
                blocks[x][4][z] = Block(BlockType::GRASS_BLOCK);
                blocks[x][4][z].exists = true;
            }
        }
        needsRemesh = true;
    }

    bool isBlockFaceVisible(int x, int y, int z, const std::string& face) {
        // Check if the adjacent block in the given direction exists
        if (face == "west")  return x == 0 || !blocks[x-1][y][z].exists;
        if (face == "east")  return x == CHUNK_SIZE-1 || !blocks[x+1][y][z].exists;
        if (face == "down")  return y == 0 || !blocks[x][y-1][z].exists;
        if (face == "up")    return y == CHUNK_SIZE-1 || !blocks[x][y+1][z].exists;
        if (face == "north") return z == 0 || !blocks[x][y][z-1].exists;
        if (face == "south") return z == CHUNK_SIZE-1 || !blocks[x][y][z+1].exists;
        return false;
    }

    void addFaceVertices(std::vector<float>& vertices, int x, int y, int z, const std::string& face) {
        const Block& block = blocks[x][y][z];
        BlockModel& model = BlockModelManager::getModel(block.getModelPath());

        float textureIndex = block.getTextureIndexForFace(face);  // This is correct

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
        transform = transform * block.getTransform();

        auto faceVertices = model.generateFaceVertices(face, transform);

        // Add base texture vertices
        for (size_t i = 0; i < faceVertices.size(); i += 9) {
            vertices.insert(vertices.end(), faceVertices.begin() + i, faceVertices.begin() + i + 9);
            vertices.push_back(textureIndex);
        }

        // If this face needs an overlay, add another set of vertices
        if (block.hasOverlay(face)) {
            // Add overlay vertices with overlay texture index (7)
            for (size_t i = 0; i < faceVertices.size(); i += 9) {
                vertices.insert(vertices.end(), faceVertices.begin() + i, faceVertices.begin() + i + 9);
                vertices.push_back(7.0f); // Overlay texture index
            }
        }
    }

    void generateMesh() {
        if (!needsRemesh) return;

        std::vector<float> vertices;

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    if (!blocks[x][y][z].exists) continue;

                    if (isBlockFaceVisible(x, y, z, "west"))  addFaceVertices(vertices, x, y, z, "west");
                    if (isBlockFaceVisible(x, y, z, "east"))  addFaceVertices(vertices, x, y, z, "east");
                    if (isBlockFaceVisible(x, y, z, "down"))  addFaceVertices(vertices, x, y, z, "down");
                    if (isBlockFaceVisible(x, y, z, "up"))    addFaceVertices(vertices, x, y, z, "up");
                    if (isBlockFaceVisible(x, y, z, "north")) addFaceVertices(vertices, x, y, z, "north");
                    if (isBlockFaceVisible(x, y, z, "south")) addFaceVertices(vertices, x, y, z, "south");
                }
            }
        }

        if (VAO == 0) {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
        }

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // In generateMesh()
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) 0); // position
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) (3 * sizeof(float))); // color
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) (6 * sizeof(float))); // texcoord
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) (8 * sizeof(float))); // face index
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void *) (9 * sizeof(float))); // texture index

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);

        vertexCount = vertices.size() / 10;
        needsRemesh = false;
    }
};