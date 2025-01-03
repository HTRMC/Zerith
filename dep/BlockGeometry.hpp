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
    GRASS_BLOCK,
    STONE
};

static const std::vector<glm::vec3> VERTICES = {
    {0.0f, 0.0f, 0.0f}, // 0
    {0.0f, 0.0f, 1.0f}, // 1
    {0.0f, 1.0f, 0.0f}, // 2
    {0.0f, 1.0f, 1.0f}, // 3
    {1.0f, 0.0f, 0.0f}, // 4
    {1.0f, 0.0f, 1.0f}, // 5
    {1.0f, 1.0f, 0.0f}, // 6
    {1.0f, 1.0f, 1.0f}  // 7
};

static const std::array<std::array<uint32_t, 6>, 6> FACE_INDICES = {{
    {0, 2, 1, 1, 2, 3},  // Front (-Z)
    {1, 3, 5, 5, 3, 7},  // Right (+X)
    {4, 5, 6, 6, 5, 7},  // Back (+Z)
    {0, 1, 4, 4, 1, 5},  // Left (-X)
    {2, 6, 3, 3, 6, 7},  // Top (+Y)
    {0, 4, 2, 2, 4, 6}   // Bottom (-Y)
}};

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
        if (z < 0 || z >= CHUNK_SIZE) return false;
        return (x + y + z) % 2 == 0;  // Match the checker pattern from generateChunkGeometry
    }

    static std::vector<Vertex> generateGeometry(BlockType blockType = BlockType::DIRT) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        vertices.reserve(NUM_BLOCKS * 8); // 8 vertices per block
        indices.reserve(NUM_BLOCKS * 36); // 6 faces * 6 indices per face

        for (int x = 0; x < GRID_SIZE; x++) {
            for (int y = 0; y < GRID_SIZE; y++) {
                for (int z = 0; z < GRID_SIZE; z++) {
                    addBlockGeometry(vertices, indices, x, y, z, blockType);
                }
            }
        }

        return vertices;
    }

    static std::pair<std::vector<Vertex>, std::vector<uint32_t>> generateChunkGeometry(int chunkX, int chunkY) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 8);
        indices.reserve(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 36);

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    if ((x + y + z) % 2 == 0) {
                        BlockType blockType = (z > CHUNK_SIZE/2) ? BlockType::GRASS_BLOCK :
                                            (z > CHUNK_SIZE/4) ? BlockType::DIRT : BlockType::STONE;
                        addBlockGeometry(vertices, indices,
                            x + chunkX,
                            y + chunkY,
                            z,
                            blockType);
                    }
                }
            }
        }

        return {vertices, indices};
    }

private:
    static void addBlockGeometry(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int x, int y, int z, BlockType blockType) {
        // Only add faces that are exposed
        if (!blockExists(x, y, z + 1)) addFace(vertices, indices, x, y, z, 0, blockType); // Front (Z+)
        if (!blockExists(x, y, z - 1)) addFace(vertices, indices, x, y, z, 1, blockType); // Back (Z-)
        if (!blockExists(x, y + 1, z)) addFace(vertices, indices, x, y, z, 2, blockType); // Top (Y+)
        if (!blockExists(x, y - 1, z)) addFace(vertices, indices, x, y, z, 3, blockType); // Bottom (Y-)
        if (!blockExists(x + 1, y, z)) addFace(vertices, indices, x, y, z, 4, blockType); // Right (X+)
        if (!blockExists(x - 1, y, z)) addFace(vertices, indices, x, y, z, 5, blockType); // Left (X-)
    }

    static void addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                   int x, int y, int z, int faceIndex, BlockType blockType) {
        glm::vec3 offset(x, y, z);
        glm::vec3 color = COLORS[faceIndex];
        float textureIndex;

        switch (blockType) {
            case BlockType::DIRT:
                textureIndex = TEXTURE_DIRT;
            break;
            case BlockType::GRASS_BLOCK:
                textureIndex = (faceIndex == 2) ? TEXTURE_GRASS_TOP :
                    (faceIndex == 3) ? TEXTURE_DIRT : TEXTURE_GRASS_SIDE;
            break;
            case BlockType::STONE:
                textureIndex = TEXTURE_STONE;
            break;
            default:
                textureIndex = TEXTURE_MISSING;
        }

        uint32_t baseIndex = vertices.size();
        for (const auto& pos : VERTICES) {
            vertices.push_back({
                pos + offset,
                color,
                FACE_TEX_COORDS[faceIndex][vertices.size() % 6],
                textureIndex
            });
        }

        for (uint32_t idx : FACE_INDICES[faceIndex]) {
            indices.push_back(baseIndex + idx);
        }

        if (blockType == BlockType::GRASS_BLOCK && faceIndex != 2 && faceIndex != 3) {
            // Add overlay vertices with same indices but different texture
            for (size_t i = 0; i < 6; i++) {
                vertices.push_back({
                    VERTICES[FACE_INDICES[faceIndex][i]] + offset,
                    color,
                    FACE_TEX_COORDS[faceIndex][i],
                    TEXTURE_GRASS_SIDE_OVERLAY
                });
                indices.push_back(baseIndex + vertices.size() - 1);
            }
        }
    }
};
