// BlockType.h
#pragma once
#include "TextureManager.h"
#include <string>
#include <map>
#include <variant>
#include <glm/glm.hpp>

enum class Axis { X, Y, Z };

enum class SlabType { BOTTOM, TOP, DOUBLE };

enum class StairShape { STRAIGHT, INNER_LEFT, INNER_RIGHT, OUTER_LEFT, OUTER_RIGHT };

enum class BlockFacing { NORTH, SOUTH, EAST, WEST };

enum class StairHalf { TOP, BOTTOM };

struct BlockProperties {
    std::map<std::string, std::variant<Axis, SlabType, StairShape, BlockFacing, StairHalf> > properties;
};

enum class BlockType {
    AIR,
    GRASS_BLOCK,
    DIRT,
    STONE,
    OAK_PLANKS,
    OAK_SLAB,
    OAK_STAIRS,
    OAK_LOG,
    GLASS
};

struct Block {
    BlockType type;
    BlockProperties properties;
    glm::vec3 color;
    bool exists;

    Block() : type(BlockType::AIR), color(1.0f), exists(false) {
    }

    Block(BlockType t) : type(t), color(1.0f), exists(true) {
        switch (t) {
            case BlockType::GRASS_BLOCK:
                break;
            case BlockType::DIRT:
                break;
            case BlockType::STONE:
                break;
            case BlockType::OAK_PLANKS:
            case BlockType::OAK_LOG:
            case BlockType::OAK_SLAB:
            case BlockType::OAK_STAIRS:
                break;
            default:
                break;
        }
    }

    std::string getModelPath() const {
        switch (type) {
            case BlockType::GRASS_BLOCK: return "block/grass_block";
            case BlockType::DIRT: return "block/dirt";
            case BlockType::STONE: return "block/stone";
            case BlockType::OAK_PLANKS: return "block/oak_planks";
            case BlockType::OAK_LOG: return "block/oak_log";
            case BlockType::OAK_SLAB: {
                if (properties.properties.count("type")) {
                    SlabType slabType = std::get<SlabType>(properties.properties.at("type"));
                    switch (slabType) {
                        case SlabType::BOTTOM: return "block/oak_slab";
                        case SlabType::TOP: return "block/oak_slab_top";
                        case SlabType::DOUBLE: return "block/oak_planks";
                    }
                }
                return "block/oak_slab"; // Default to bottom slab
            }
            case BlockType::OAK_STAIRS: return "block/oak_stairs";
            case BlockType::GLASS: return "block/glass";

            default: return "block/stone";
        }
    }

    unsigned int getTexture() const {
        switch (type) {
            case BlockType::GRASS_BLOCK:
                // Grass block needs different textures for top/sides/bottom
                return TextureManager::getTexture("block/grass_block_side"); // Need to handle top/bottom differently
            case BlockType::DIRT:
                return TextureManager::getTexture("block/dirt");
            case BlockType::STONE:
                return TextureManager::getTexture("block/stone");
            case BlockType::OAK_PLANKS:
                return TextureManager::getTexture("block/oak_planks");
            case BlockType::OAK_LOG: {
                // Logs need different textures for top/sides
                return TextureManager::getTexture("block/oak_log_side");
            }
            case BlockType::OAK_SLAB:
                return TextureManager::getTexture("block/oak_planks");
            case BlockType::OAK_STAIRS:
                return TextureManager::getTexture("block/oak_planks");
            case BlockType::GLASS:
                return TextureManager::getTexture("block/glass");

            default:
                return TextureManager::getTexture("block/stone");
        }
    }

    unsigned int getTextureForFace(const std::string &face) const {
        switch (type) {
            case BlockType::GRASS_BLOCK:
                if (face == "up")
                    return TextureManager::getTexture("block/grass_block_top");
                else if (face == "down")
                    return TextureManager::getTexture("block/dirt");
                else
                    return TextureManager::getTexture("block/grass_block_side");

            case BlockType::OAK_LOG:
                if (face == "up" || face == "down")
                    return TextureManager::getTexture("block/oak_log_top");
                else
                    return TextureManager::getTexture("block/oak_log_side");

            default:
                return getTexture(); // Use default texture for other blocks
        }
    }

    glm::mat4 getTransform() const {
        glm::mat4 transform(1.0f);

        if (type == BlockType::OAK_LOG) {
            Axis axis = std::get<Axis>(properties.properties.at("axis"));
            if (axis == Axis::X) {
                transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(0, 0, 1));
                transform = glm::translate(transform, glm::vec3(0, -1, 0));
            } else if (axis == Axis::Z) {
                transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1, 0, 0));
                transform = glm::translate(transform, glm::vec3(0, 0, -1));
            }
        } else if (type == BlockType::OAK_STAIRS) {
            BlockFacing facing = std::get<BlockFacing>(properties.properties.at("facing"));
            StairHalf half = std::get<StairHalf>(properties.properties.at("half"));

            // Apply rotation based on facing direction
            switch (facing) {
                case BlockFacing::EAST:
                    transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(0, 1, 0));
                    transform = glm::translate(transform, glm::vec3(-1, 0, 0));
                    break;
                case BlockFacing::SOUTH:
                    transform = glm::rotate(transform, glm::radians(180.0f), glm::vec3(0, 1, 0));
                    transform = glm::translate(transform, glm::vec3(-1, 0, -1));
                    break;
                case BlockFacing::WEST:
                    transform = glm::rotate(transform, glm::radians(270.0f), glm::vec3(0, 1, 0));
                    transform = glm::translate(transform, glm::vec3(0, 0, -1));
                    break;
                case BlockFacing::NORTH:
                    transform = glm::translate(transform, glm::vec3(0, 0, 0));
                    break;
            }

            // Handle upside-down stairs
            if (half == StairHalf::TOP) {
                transform = glm::rotate(transform, glm::radians(180.0f), glm::vec3(1, 0, 0));
                transform = glm::translate(transform, glm::vec3(0, -1, -1));
            }
        }

        return transform;
    }

    unsigned int getTextureIndex() const {
        switch (type) {
            case BlockType::DIRT: return 0;
            case BlockType::STONE: return 1;
            case BlockType::GRASS_BLOCK: return 2; // side texture
            case BlockType::OAK_PLANKS: return 4; // add new index
            case BlockType::OAK_LOG: return 5; // add new index
            case BlockType::OAK_SLAB: return 4; // uses planks texture
            case BlockType::OAK_STAIRS: return 4; // uses planks texture
            case BlockType::GLASS: return 8;

            default: return 0;
        }
    }

    bool isTransparent() const {
        switch (type) {
            case BlockType::AIR:
            case BlockType::GLASS:
                return true;
            default:
                return false;
        }
    }

    unsigned int getTextureIndexForFace(const std::string &face) const {
        switch (type) {
            case BlockType::GRASS_BLOCK:
                if (face == "up") return 3; // grass_block_top
                if (face == "down") return 0; // dirt
                return 2; // grass_block_side

            case BlockType::OAK_LOG:
                if (face == "up" || face == "down")
                    return 6; // oak_log_top
                return 5; // oak_log_side

            case BlockType::DIRT:
                return 0; // dirt texture

            case BlockType::STONE:
                return 1; // stone texture

            case BlockType::OAK_PLANKS:
            case BlockType::OAK_SLAB:
            case BlockType::OAK_STAIRS:
                return 4; // oak_planks texture

            case BlockType::GLASS:
                return 8; // glass texture

            default:
                return 0; // fallback to dirt texture
        }
    }

    bool hasOverlay(const std::string &face) const {
        return type == BlockType::GRASS_BLOCK &&
               face != "up" &&
               face != "down";
    }
};

SlabType determineSlabType(const glm::vec3 &hitPos, const glm::ivec3 &blockPos) {
    float hitY = hitPos.y - blockPos.y;
    return (hitY > 0.5f) ? SlabType::TOP : SlabType::BOTTOM;
}

// Helper functions to create blocks with properties
Block createOakLog(Axis axis) {
    Block block(BlockType::OAK_LOG);
    block.properties.properties["axis"] = axis;
    return block;
}

Block createOakStairs(BlockFacing facing, StairHalf half) {
    Block block(BlockType::OAK_STAIRS);
    block.properties.properties["facing"] = facing;
    block.properties.properties["half"] = half;
    return block;
}

Block createOakSlab(SlabType type) {
    Block block(BlockType::OAK_SLAB);
    block.properties.properties["type"] = type;
    return block;
}
