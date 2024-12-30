#pragma once
#include <vector>
#include <array>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

class BlockGeometry {
public:
    static constexpr int GRID_SIZE = 16;
    static constexpr int NUM_BLOCKS = GRID_SIZE * GRID_SIZE * GRID_SIZE;

    // Colors for each face direction
    static constexpr std::array<glm::vec3, 6> COLORS = {{
        {1.0f, 0.0f, 0.0f}, // Front - Red
        {0.0f, 1.0f, 0.0f}, // Back - Green
        {0.0f, 0.0f, 1.0f}, // Top - Blue
        {1.0f, 1.0f, 0.0f}, // Bottom - Yellow
        {1.0f, 0.0f, 1.0f}, // Right - Magenta
        {0.0f, 1.0f, 1.0f}  // Left - Cyan
    }};

    // Vertices for a single face (two triangles)
    static const std::array<std::array<glm::vec3, 6>, 6> FACE_VERTICES;

    static constexpr std::array<std::array<glm::vec2, 6>, 6> FACE_TEX_COORDS = {{
        // Front face (+Z)
        {{
            {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f},
            {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}
        }},
        // Back face (-Z)
        {{
            {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}
        }},
        // Top face (+Y)
        {{
            {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}
        }},
        // Bottom face (-Y)
        {{
            {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f},
            {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}
        }},
        // Right face (+X)
        {{
            {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
            {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}
        }},
        // Left face (-X)
        {{
            {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f},
            {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}
        }}
    }};

    static bool blockExists(int x, int y, int z) {
        return x >= 0 && x < GRID_SIZE &&
               y >= 0 && y < GRID_SIZE &&
               z >= 0 && z < GRID_SIZE;
    }

    static std::vector<Vertex> generateGeometry() {
        std::vector<Vertex> vertices;
        vertices.reserve(NUM_BLOCKS * 36); // Maximum possible vertices

        for (int x = 0; x < GRID_SIZE; x++) {
            for (int y = 0; y < GRID_SIZE; y++) {
                for (int z = 0; z < GRID_SIZE; z++) {
                    addBlockGeometry(vertices, x, y, z);
                }
            }
        }

        return vertices;
    }

private:
    static void addBlockGeometry(std::vector<Vertex>& vertices, int x, int y, int z) {
        // Check each face direction
        // Front face (+Z)
        if (!blockExists(x, y, z + 1)) {
            addFace(vertices, x, y, z, 0);
        }
        // Back face (-Z)
        if (!blockExists(x, y, z - 1)) {
            addFace(vertices, x, y, z, 1);
        }
        // Top face (+Y)
        if (!blockExists(x, y + 1, z)) {
            addFace(vertices, x, y, z, 2);
        }
        // Bottom face (-Y)
        if (!blockExists(x, y - 1, z)) {
            addFace(vertices, x, y, z, 3);
        }
        // Right face (+X)
        if (!blockExists(x + 1, y, z)) {
            addFace(vertices, x, y, z, 4);
        }
        // Left face (-X)
        if (!blockExists(x - 1, y, z)) {
            addFace(vertices, x, y, z, 5);
        }
    }

    static void addFace(std::vector<Vertex>& vertices, int x, int y, int z, int faceIndex) {
        glm::vec3 offset(x, y, z);
        glm::vec3 color = COLORS[faceIndex];

        for (size_t i = 0; i < 6; i++) {
            vertices.push_back({
                FACE_VERTICES[faceIndex][i] + offset,
                color,
                FACE_TEX_COORDS[faceIndex][i]
            });
        }
    }
};
