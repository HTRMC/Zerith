// Quad.hpp
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    uint16_t textureID;
};

class Quad {
public:
    // Constants for the grid
    static const int GRID_SIZE = 16;
    static const int INSTANCE_COUNT = GRID_SIZE * GRID_SIZE;

    static std::vector<Vertex> getQuadVertices() {
        return {
                {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 0},  // Bottom-left
                {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, 0},  // Top-left
                {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, 0},  // Bottom-right
                {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 0}   // Top-right
        };
    }

    static std::vector<uint32_t> getQuadIndices() {
        return {
            0, 1, 2, 3  // Triangle strip order
        };
    }

    static std::vector<glm::mat4> generateInstanceTransforms() {
        std::vector<glm::mat4> transforms;
        transforms.reserve(INSTANCE_COUNT);

        // Generate transforms for each quad in the 16x16 grid
        for (int x = 0; x < GRID_SIZE; x++) {
            for (int y = 0; y < GRID_SIZE; y++) {
                glm::mat4 transform = glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3(static_cast<float>(x), static_cast<float>(y), 0.0f)
                );
                transforms.push_back(transform);
            }
        }

        return transforms;
    }
};