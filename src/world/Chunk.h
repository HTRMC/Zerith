// Chunk.h
#pragma once
#include <vector>
#include "blocks/BlockType.h"
#include "blocks/BlockModel.h"
#include <glm/glm.hpp>

std::string loadFileContent(const std::string &path) {
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
    static BlockModel &getModel(const std::string &path) {
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
    std::vector<uint8_t> blocks; // 1D array to store block data
    std::unordered_map<size_t, BlockProperties> blockProperties; // Store properties for special blocks
    glm::ivec2 position; // Chunk position in world coordinates (x, z)

    unsigned int opaqueVAO = 0;
    unsigned int opaqueVBO = 0;
    int opaqueVertexCount = 0;

    unsigned int transparentVAO = 0;
    unsigned int transparentVBO = 0;
    int transparentVertexCount = 0;

    bool needsRemesh = true;

    // Helper function to convert 3D coordinates to 1D index
    static inline size_t getIndex(int x, int y, int z) {
        return (x * CHUNK_SIZE * CHUNK_SIZE) + (y * CHUNK_SIZE) + z;
    }

    // Helper function to get block at position
    Block getBlock(int x, int y, int z) const {
        size_t index = getIndex(x, y, z);
        uint8_t blockData = blocks[index];
        Block block(static_cast<BlockType>(blockData & 0x7F)); // Use 7 bits for block type
        block.exists = (blockData & 0x80) != 0; // Use highest bit for exists flag

        // Get properties if they exist
        auto propIt = blockProperties.find(index);
        if (propIt != blockProperties.end()) {
            block.properties = propIt->second;
        }

        return block;
    }

    // Helper function to set block at position
    void setBlock(int x, int y, int z, const Block& block) {
        size_t index = getIndex(x, y, z);
        uint8_t blockData = static_cast<uint8_t>(static_cast<int>(block.type) & 0x7F);
        if (block.exists) {
            blockData |= 0x80; // Set exists flag in highest bit
        }
        blocks[index] = blockData;

        // Handle properties
        if (!block.properties.properties.empty()) {
            blockProperties[index] = block.properties;
        } else {
            blockProperties.erase(index);
        }
    }

    Chunk() : position(0, 0) {
        initialize();
    }

    Chunk(int x, int z) : position(x, z) {
        initialize();
    }

    ~Chunk() {
        if (opaqueVAO != 0) {
            glDeleteVertexArrays(1, &opaqueVAO);
            glDeleteBuffers(1, &opaqueVBO);
        }
        if (transparentVAO != 0) {
            glDeleteVertexArrays(1, &transparentVAO);
            glDeleteBuffers(1, &transparentVBO);
        }
    }

    void initialize() {
        blocks.resize(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, 0);
    }

    void generateTerrain() {
        // Basic terrain generation
        for (int y = 0; y <= 2; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    Block block(BlockType::STONE);
                    block.exists = true;
                    setBlock(x, y, z, block);
                }
            }
        }

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                Block dirtBlock(BlockType::DIRT);
                dirtBlock.exists = true;
                setBlock(x, 3, z, dirtBlock);

                Block grassBlock(BlockType::GRASS_BLOCK);
                grassBlock.exists = true;
                setBlock(x, 4, z, grassBlock);
            }
        }
        needsRemesh = true;
    }

    bool isBlockFaceVisible(int x, int y, int z, const std::string &face) {
        // First check if we're at chunk borders
        if (face == "west" && x == 0) return true;
        if (face == "east" && x == CHUNK_SIZE - 1) return true;
        if (face == "down" && y == 0) return true;
        if (face == "up" && y == CHUNK_SIZE - 1) return true;
        if (face == "north" && z == 0) return true;
        if (face == "south" && z == CHUNK_SIZE - 1) return true;

        // Get coordinates of the neighbor block based on face
        int nx = x, ny = y, nz = z;
        if (face == "west") nx--;
        if (face == "east") nx++;
        if (face == "down") ny--;
        if (face == "up") ny++;
        if (face == "north") nz--;
        if (face == "south") nz++;

        // Check if neighbor coordinates are valid
        if (nx < 0 || nx >= CHUNK_SIZE ||
            ny < 0 || ny >= CHUNK_SIZE ||
            nz < 0 || nz >= CHUNK_SIZE) {
            return true;
        }

        Block currentBlock = getBlock(x, y, z);
        Block neighborBlock = getBlock(nx, ny, nz);

        // If there's no neighbor block or it doesn't exist, the face is visible
        if (!neighborBlock.exists) return true;

        if (currentBlock.isTransparent() || neighborBlock.isTransparent()) {
            // If both blocks are glass, only show faces between different block types
            if (currentBlock.type == BlockType::GLASS &&
                neighborBlock.type == BlockType::GLASS) {
                return false;
                }
            return true;
        }

        // Handle stairs
        if (currentBlock.type == BlockType::OAK_STAIRS && currentBlock.properties.properties.count("facing")) {
            BlockFacing facing = std::get<BlockFacing>(currentBlock.properties.properties.at("facing"));
            StairHalf half = std::get<StairHalf>(currentBlock.properties.properties.at("half"));

            // When next to another stair
            if (neighborBlock.type == BlockType::OAK_STAIRS) {
                BlockFacing neighborFacing = std::get<BlockFacing>(neighborBlock.properties.properties.at("facing"));
                StairHalf neighborHalf = std::get<StairHalf>(neighborBlock.properties.properties.at("half"));

                // If the stairs are at the same height (both top or both bottom)
                if (half == neighborHalf) {
                    // For vertical faces
                    if (face == "north" || face == "south" || face == "east" || face == "west") {
                        // If stairs are facing the same direction or opposite directions
                        if (facing == neighborFacing ||
                            (facing == BlockFacing::NORTH && neighborFacing == BlockFacing::SOUTH) ||
                            (facing == BlockFacing::SOUTH && neighborFacing == BlockFacing::NORTH) ||
                            (facing == BlockFacing::EAST && neighborFacing == BlockFacing::WEST) ||
                            (facing == BlockFacing::WEST && neighborFacing == BlockFacing::EAST)) {
                            return false;
                        }
                    }

                    // For top/bottom faces
                    if (face == "up" && half == StairHalf::TOP) return false;
                    if (face == "down" && half == StairHalf::BOTTOM) return false;
                }

                return true;
            }

            // When next to a slab
            if (neighborBlock.type == BlockType::OAK_SLAB) {
                SlabType neighborType = std::get<SlabType>(neighborBlock.properties.properties.at("type"));

                if (neighborType == SlabType::BOTTOM && half == StairHalf::BOTTOM && face == "down") return false;
                if (neighborType == SlabType::TOP && half == StairHalf::TOP && face == "up") return false;

                return true;
            }

            // When next to a full block
            if (neighborBlock.type != BlockType::OAK_SLAB &&
                neighborBlock.type != BlockType::OAK_STAIRS) {
                if (face == "north" && facing == BlockFacing::NORTH) return false;
                if (face == "south" && facing == BlockFacing::SOUTH) return false;
                if (face == "east" && facing == BlockFacing::EAST) return false;
                if (face == "west" && facing == BlockFacing::WEST) return false;

                if (face == "down" && half == StairHalf::BOTTOM) return false;
                if (face == "up" && half == StairHalf::TOP) return false;

                return true;
            }
        }

        // Handle slabs
        if (currentBlock.type == BlockType::OAK_SLAB && currentBlock.properties.properties.count("type")) {
            SlabType currentType = std::get<SlabType>(currentBlock.properties.properties.at("type"));

            // When next to stairs
            if (neighborBlock.type == BlockType::OAK_STAIRS) {
                BlockFacing neighborFacing = std::get<BlockFacing>(neighborBlock.properties.properties.at("facing"));
                StairHalf neighborHalf = std::get<StairHalf>(neighborBlock.properties.properties.at("half"));

                // Cull faces that are completely covered by the stair
                if (currentType == SlabType::BOTTOM && neighborHalf == StairHalf::BOTTOM) {
                    if (face == "north" && neighborFacing == BlockFacing::SOUTH) return false;
                    if (face == "south" && neighborFacing == BlockFacing::NORTH) return false;
                    if (face == "east" && neighborFacing == BlockFacing::WEST) return false;
                    if (face == "west" && neighborFacing == BlockFacing::EAST) return false;
                } else if (currentType == SlabType::TOP && neighborHalf == StairHalf::TOP) {
                    if (face == "north" && neighborFacing == BlockFacing::SOUTH) return false;
                    if (face == "south" && neighborFacing == BlockFacing::NORTH) return false;
                    if (face == "east" && neighborFacing == BlockFacing::WEST) return false;
                    if (face == "west" && neighborFacing == BlockFacing::EAST) return false;
                }

                return true;
            }

            // Handle double slabs
            if (currentType == SlabType::DOUBLE) {
                if (neighborBlock.type == BlockType::OAK_SLAB ||
                    neighborBlock.type == BlockType::OAK_STAIRS) {
                    return true;
                }
            }

            // Handle single slab faces
            if ((currentType == SlabType::BOTTOM && face == "up") ||
                (currentType == SlabType::TOP && face == "down")) {
                return true;
            }

            // Handle slab next to slab
            if (neighborBlock.type == BlockType::OAK_SLAB) {
                SlabType neighborType = std::get<SlabType>(neighborBlock.properties.properties.at("type"));

                // Cull faces when slabs align
                if (currentType == neighborType) {
                    if (face == "north" || face == "south" || face == "east" || face == "west") {
                        return false;
                    }
                }
            }

            if (face == "north" || face == "south" || face == "east" || face == "west") {
                if (neighborBlock.type != BlockType::OAK_SLAB &&
                    neighborBlock.type != BlockType::OAK_STAIRS) {
                    return !neighborBlock.exists;
                }
                return true;
            }
        }

        // Handle blocks next to slabs
        if (neighborBlock.type == BlockType::OAK_SLAB) {
            SlabType neighborType = std::get<SlabType>(neighborBlock.properties.properties.at("type"));

            if (neighborType == SlabType::TOP) {
                if (face == "up") return true;
                if (face == "down") return false;
                return true;
            } else if (neighborType == SlabType::BOTTOM) {
                if (face == "up") return false;
                if (face == "down") return true;
                return true;
            } else if (neighborType == SlabType::DOUBLE) {
                return false;
            }
        }

        // Default case for regular blocks
        return !neighborBlock.exists;
    }

    void addFaceVertices(std::vector<float> &vertices, int x, int y, int z, const std::string &face) {
        Block block = getBlock(x, y, z);
        BlockModel &model = BlockModelManager::getModel(block.getModelPath());

        float textureIndex = block.getTextureIndexForFace(face); // This is correct

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

        std::vector<float> opaqueVertices;
        std::vector<float> transparentVertices;

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    Block block = getBlock(x, y, z);
                    if (!block.exists) continue;

                    // Choose which vertex array to add to based on block type
                    std::vector<float>& vertices = block.isTransparent() ?
                        transparentVertices : opaqueVertices;

                    if (isBlockFaceVisible(x, y, z, "west"))
                        addFaceVertices(vertices, x, y, z, "west");
                    if (isBlockFaceVisible(x, y, z, "east"))
                        addFaceVertices(vertices, x, y, z, "east");
                    if (isBlockFaceVisible(x, y, z, "down"))
                        addFaceVertices(vertices, x, y, z, "down");
                    if (isBlockFaceVisible(x, y, z, "up"))
                        addFaceVertices(vertices, x, y, z, "up");
                    if (isBlockFaceVisible(x, y, z, "north"))
                        addFaceVertices(vertices, x, y, z, "north");
                    if (isBlockFaceVisible(x, y, z, "south"))
                        addFaceVertices(vertices, x, y, z, "south");
                }
            }
        }

        // Generate and update opaque mesh
        if (opaqueVAO == 0) {
            glGenVertexArrays(1, &opaqueVAO);
            glGenBuffers(1, &opaqueVBO);
        }

        glBindVertexArray(opaqueVAO);
        glBindBuffer(GL_ARRAY_BUFFER, opaqueVBO);
        glBufferData(GL_ARRAY_BUFFER, opaqueVertices.size() * sizeof(float),
                    opaqueVertices.data(), GL_STATIC_DRAW);

        // Set up vertex attributes for opaque mesh
        setupVertexAttributes();
        opaqueVertexCount = opaqueVertices.size() / 10;  // 10 floats per vertex

        // Generate and update transparent mesh
        if (transparentVAO == 0) {
            glGenVertexArrays(1, &transparentVAO);
            glGenBuffers(1, &transparentVBO);
        }

        glBindVertexArray(transparentVAO);
        glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
        glBufferData(GL_ARRAY_BUFFER, transparentVertices.size() * sizeof(float),
                    transparentVertices.data(), GL_STATIC_DRAW);

        // Set up vertex attributes for transparent mesh
        setupVertexAttributes();
        transparentVertexCount = transparentVertices.size() / 10;

        needsRemesh = false;
    }

private:
    void setupVertexAttributes() {
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                             (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                             (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Face index attribute
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                             (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);

        // Texture index attribute
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 10 * sizeof(float),
                             (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(4);
    }
};
