#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "BlockType.hpp"
#include "ChunkStorage.hpp"

class Player {
public:
    // Player dimensions
    static constexpr float WIDTH = 0.6f;  // Player width (X axis)
    static constexpr float DEPTH = 0.6f;  // Player depth (Y axis)
    static constexpr float HEIGHT = 1.8f; // Player height (Z axis)
    static constexpr float EYE_HEIGHT = 1.62f; // Camera height from player base

    Player(const glm::vec3& position)
        : position(position)
        , velocity(0.0f)
        , onGround(false) {
        updateBoundingBox();
    }

    void update(float deltaTime, const std::vector<ChunkStorage::ChunkPositionData>& chunks) {
        // Store old position for collision response
        glm::vec3 oldPosition = position;

        // Apply velocity
        position += velocity * deltaTime;

        // Update bounding box with new position
        updateBoundingBox();

        // Check collisions with nearby blocks
        resolveCollisions(chunks);

        // Apply gravity
        if (!onGround) {
            velocity.z -= 20.0f * deltaTime; // Gravity acceleration
        }

        // Terminal velocity
        velocity.z = glm::max(velocity.z, -50.0f);
    }

    void move(const glm::vec3& direction, float deltaTime) {
        float speed = 4.317f;
        velocity.x = direction.x * speed;
        velocity.y = direction.y * speed;

        // Only allow jumping when on ground
        if (direction.z > 0 && onGround) {
            velocity.z = 8.0f; // Jump velocity
            onGround = false;
        }
    }

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getEyePosition() const { return position + glm::vec3(0.0f, 0.0f, EYE_HEIGHT); }

private:
    glm::vec3 position;    // Position of player's feet
    glm::vec3 velocity;    // Current velocity
    bool onGround;         // Whether player is on ground

    // Bounding box corners (in world space)
    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;

    void updateBoundingBox() {
        // Update bounding box based on current position
        boundingBoxMin = position + glm::vec3(-WIDTH/2, -DEPTH/2, 0);
        boundingBoxMax = position + glm::vec3(WIDTH/2, DEPTH/2, HEIGHT);
    }

    bool checkBlockCollision(const glm::vec3& blockPos) {
        // Simple AABB vs AABB collision check
        return (boundingBoxMin.x < blockPos.x + 1.0f &&
                boundingBoxMax.x > blockPos.x &&
                boundingBoxMin.y < blockPos.y + 1.0f &&
                boundingBoxMax.y > blockPos.y &&
                boundingBoxMin.z < blockPos.z + 1.0f &&
                boundingBoxMax.z > blockPos.z);
    }

    void resolveCollisions(const std::vector<ChunkStorage::ChunkPositionData>& chunks) {
        // Get blocks in potential collision range
        glm::ivec3 minBlock = glm::floor(boundingBoxMin);
        glm::ivec3 maxBlock = glm::ceil(boundingBoxMax);

        onGround = false;

        // Check each potentially colliding block
        for (int x = minBlock.x; x <= maxBlock.x; x++) {
            for (int y = minBlock.y; y <= maxBlock.y; y++) {
                for (int z = minBlock.z; z <= maxBlock.z; z++) {
                    glm::vec3 blockPos(x, y, z);
                    BlockType blockType = ChunkStorage::getBlockAtWorld(blockPos);

                    if (ChunkStorage::isBlockSolid(blockType)) {
                        if (checkBlockCollision(blockPos)) {
                            // Calculate penetration depth for each axis
                            float penX = (velocity.x > 0) ?
                                (boundingBoxMax.x - blockPos.x) :
                                (blockPos.x + 1.0f - boundingBoxMin.x);
                            float penY = (velocity.y > 0) ?
                                (boundingBoxMax.y - blockPos.y) :
                                (blockPos.y + 1.0f - boundingBoxMin.y);
                            float penZ = (velocity.z > 0) ?
                                (boundingBoxMax.z - blockPos.z) :
                                (blockPos.z + 1.0f - boundingBoxMin.z);

                            // Find minimum penetration axis
                            if (penX < penY && penX < penZ) {
                                position.x += (velocity.x > 0) ? -penX : penX;
                                velocity.x = 0;
                            } else if (penY < penX && penY < penZ) {
                                position.y += (velocity.y > 0) ? -penY : penY;
                                velocity.y = 0;
                            } else {
                                position.z += (velocity.z > 0) ? -penZ : penZ;
                                if (velocity.z < 0) {
                                    onGround = true;
                                }
                                velocity.z = 0;
                            }
                            updateBoundingBox();
                        }
                    }
                }
            }
        }
    }
};