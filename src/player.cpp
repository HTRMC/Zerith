#include "player.h"
#include "chunk_manager.h"
#include "logger.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Zerith {
    Player::Player(const glm::vec3 &position)
        : m_position(position), m_velocity(0.0f), m_rotation(0.0f), m_cameraFront(0.0f, 0.0f, -1.0f) {
        updateAABB();
        LOG_INFO("Player created at position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);
    }

    void Player::update(float deltaTime, ChunkManager *chunkManager) {
        applyGravity(deltaTime);

        glm::vec3 deltaPosition = m_velocity * deltaTime;
        m_position += deltaPosition;
        updateAABB();

        resolveCollisions(chunkManager);
    }

    void Player::handleInput(GLFWwindow *window, float deltaTime) {
        glm::vec3 movement(0.0f);

        // Input mapping (same as before)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            movement.x -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            movement.x += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            movement.z -= 1.0f; // Left
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            movement.z += 1.0f; // Right
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && m_onGround) {
            jump();
        }

        // ====== OLD SYSTEM APPROACH ADAPTED TO Y-UP ======

        // Calculate velocity for this frame
        float velocity = MOVE_SPEED * deltaTime;

        // Create camera front vector from rotation (like updateCameraDirection)
        glm::vec3 cameraFront;
        cameraFront.x = cos(m_rotation.y) * cos(m_rotation.x); // Adapted for Y-up
        cameraFront.y = sin(m_rotation.x); // Y is now vertical
        cameraFront.z = sin(m_rotation.y) * cos(m_rotation.x); // Adapted for Y-up
        cameraFront = glm::normalize(cameraFront);

        // Create horizontal front vector (zero out vertical component)
        glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

        // Handle edge case when looking straight up/down
        if (glm::length(horizontalFront) < 0.1f) {
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f); // Y-up instead of Z-up
            glm::vec3 right = glm::cross(cameraFront, worldUp);
            horizontalFront = glm::normalize(glm::cross(worldUp, right));
        }

        // Calculate right vector using cross product (OLD METHOD)
        glm::vec3 right = glm::normalize(glm::cross(horizontalFront, glm::vec3(0.0f, 1.0f, 0.0f)));

        // Apply movement directly to velocity (OLD METHOD)
        if (movement.z != 0.0f) {
            m_velocity.x += horizontalFront.x * movement.z * velocity;
            m_velocity.z += horizontalFront.z * movement.z * velocity;
        }

        if (movement.x != 0.0f) {
            m_velocity.x += right.x * movement.x * velocity;
            m_velocity.z += right.z * movement.x * velocity;
        }

        // Mouse handling (same as before)
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (m_firstMouse) {
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
            m_firstMouse = false;
        }

        double deltaX = mouseX - m_lastMouseX;
        double deltaY = mouseY - m_lastMouseY;
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;

        m_rotation.y += deltaX * MOUSE_SENSITIVITY;
        m_rotation.x -= deltaY * MOUSE_SENSITIVITY;

        m_rotation.x = std::clamp(m_rotation.x, -1.5f, 1.5f);
    }

    void Player::setPosition(const glm::vec3 &position) {
        m_position = position;
        updateAABB();
    }

    glm::mat4 Player::getViewMatrix() const {
        glm::vec3 eyePosition = m_position + glm::vec3(0.0f, m_eyeHeight, 0.0f);

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::rotate(view, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::translate(view, -eyePosition);

        return view;
    }

    void Player::jump() {
        if (m_onGround) {
            m_velocity.y = JUMP_VELOCITY;
            m_onGround = false;
            LOG_DEBUG("Player jumped");
        }
    }

    void Player::updateAABB() {
        glm::vec3 size(PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_WIDTH);
        m_aabb = AABB::fromCenterAndSize(m_position + glm::vec3(0.0f, PLAYER_HEIGHT * 0.5f, 0.0f), size);
    }

    void Player::applyGravity(float deltaTime) {
        m_velocity.y -= GRAVITY * deltaTime;
    }

    void Player::resolveCollisions(ChunkManager *chunkManager) {
        if (!chunkManager) return;

        m_onGround = false;

        AABB expandedAABB = m_aabb;
        expandedAABB.min -= glm::vec3(1.0f);
        expandedAABB.max += glm::vec3(1.0f);

        std::vector<AABB> blockAABBs = CollisionSystem::getBlockAABBsInRegion(expandedAABB, chunkManager);

        for (const AABB &blockAABB: blockAABBs) {
            CollisionSystem::CollisionResult collision = CollisionSystem::checkAABBCollision(m_aabb, blockAABB);

            if (collision.hasCollision) {
                if (collision.normal.y > 0.0f) {
                    m_onGround = true;
                    m_velocity.y = 0.0f;
                    m_position.y = blockAABB.max.y;
                } else if (collision.normal.y < 0.0f) {
                    m_velocity.y = 0.0f;
                    m_position.y = blockAABB.min.y - PLAYER_HEIGHT;
                }

                if (collision.normal.x != 0.0f) {
                    m_velocity.x = 0.0f;
                    if (collision.normal.x > 0.0f) {
                        m_position.x = blockAABB.max.x + PLAYER_WIDTH * 0.5f;
                    } else {
                        m_position.x = blockAABB.min.x - PLAYER_WIDTH * 0.5f;
                    }
                }

                if (collision.normal.z != 0.0f) {
                    m_velocity.z = 0.0f;
                    if (collision.normal.z > 0.0f) {
                        m_position.z = blockAABB.max.z + PLAYER_WIDTH * 0.5f;
                    } else {
                        m_position.z = blockAABB.min.z - PLAYER_WIDTH * 0.5f;
                    }
                }

                updateAABB();
            }
        }
    }
} // namespace Zerith
