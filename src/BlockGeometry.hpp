#pragma once
#include <vector>
#include <array>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    float textureIndex;
};

enum class BlockType {
    DIRT,
    GRASS,
    STONE
};

class BlockGeometry {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int GRID_SIZE = 16;
    static constexpr int NUM_BLOCKS = GRID_SIZE * GRID_SIZE * GRID_SIZE;

    static constexpr float TEXTURE_DIRT = 0.0f;
    static constexpr float TEXTURE_GRASS_TOP = 1.0f;
    static constexpr float TEXTURE_GRASS_SIDE = 2.0f;
    static constexpr float TEXTURE_GRASS_SIDE_OVERLAY = 3.0f;
    static constexpr float TEXTURE_STONE = 4.0f;
    static constexpr float TEXTURE_MISSING = 5.0f;

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
        return z >= 0 && z < CHUNK_SIZE;
    }

    static std::vector<Vertex> generateGeometry(BlockType blockType = BlockType::DIRT) {
        std::vector<Vertex> vertices;
        vertices.reserve(NUM_BLOCKS * 36);

        for (int x = 0; x < GRID_SIZE; x++) {
            for (int y = 0; y < GRID_SIZE; y++) {
                for (int z = 0; z < GRID_SIZE; z++) {
                    addBlockGeometry(vertices, x, y, z, blockType);
                }
            }
        }

        return vertices;
    }
    static std::vector<Vertex> generateChunkGeometry(int chunkX, int chunkY) {
        std::vector<Vertex> vertices;
        vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 36);

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    // You could implement different patterns of blocks here
                    BlockType blockType = (z > CHUNK_SIZE/2) ? BlockType::GRASS :
                                        (z > CHUNK_SIZE/4) ? BlockType::DIRT : BlockType::STONE;
                    addBlockGeometry(vertices,
                        x + chunkX,
                        y + chunkY,
                        z,
                        blockType);
                }
            }
        }

        return vertices;
    }

private:
    static void addBlockGeometry(std::vector<Vertex>& vertices, int x, int y, int z, BlockType blockType) {
        // Only add faces that are exposed
        if (!blockExists(x, y, z + 1)) addFace(vertices, x, y, z, 0, blockType); // Front (Z+)
        if (!blockExists(x, y, z - 1)) addFace(vertices, x, y, z, 1, blockType); // Back (Z-)
        if (!blockExists(x, y + 1, z)) addFace(vertices, x, y, z, 2, blockType); // Top (Y+)
        if (!blockExists(x, y - 1, z)) addFace(vertices, x, y, z, 3, blockType); // Bottom (Y-)
        if (!blockExists(x + 1, y, z)) addFace(vertices, x, y, z, 4, blockType); // Right (X+)
        if (!blockExists(x - 1, y, z)) addFace(vertices, x, y, z, 5, blockType); // Left (X-)
    }

    static void addFace(std::vector<Vertex>& vertices, int x, int y, int z, int faceIndex, BlockType blockType) {
        glm::vec3 offset(x, y, z);
        glm::vec3 color = COLORS[faceIndex];

        // Determine texture indices based on block type and face
        float textureIndex = TEXTURE_MISSING; // Default fallback

        switch (blockType) {
            case BlockType::DIRT:
                textureIndex = TEXTURE_DIRT;
            break;
            case BlockType::GRASS:
                if (faceIndex == 2) // Top face
                    textureIndex = TEXTURE_GRASS_TOP;
                else if (faceIndex == 3) // Bottom face
                    textureIndex = TEXTURE_DIRT;
                else // Side faces
                    textureIndex = TEXTURE_GRASS_SIDE;
            break;
            case BlockType::STONE:
                textureIndex = TEXTURE_STONE;
            break;
        }

        for (size_t i = 0; i < 6; i++) {
            vertices.push_back({
                FACE_VERTICES[faceIndex][i] + offset,
                color,
                FACE_TEX_COORDS[faceIndex][i],
                textureIndex
            });
        }

        // Add overlay vertices for grass block sides
        if (blockType == BlockType::GRASS && faceIndex != 2 && faceIndex != 3) {
            for (size_t i = 0; i < 6; i++) {
                vertices.push_back({
                    FACE_VERTICES[faceIndex][i] + offset,
                    color,
                    FACE_TEX_COORDS[faceIndex][i],
                    TEXTURE_GRASS_SIDE_OVERLAY
                });
            }
        }
    }
};
