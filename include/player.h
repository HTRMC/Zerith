#pragma once

#include <glm/glm.hpp>
#include "aabb.h"
#include "chunk.h"

struct GLFWwindow;

namespace Zerith {

class ChunkManager;

class Player {
public:
    Player(const glm::vec3& position = glm::vec3(0.0f, 10.0f, 0.0f));
    
    void update(float deltaTime, ChunkManager* chunkManager);
    void handleInput(GLFWwindow* window, float deltaTime, ChunkManager* chunkManager);
    
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getVelocity() const { return m_velocity; }
    const glm::vec3& getRotation() const { return m_rotation; }
    const glm::vec3& getCameraFront() const { return m_cameraFront; }
    const AABB& getAABB() const { return m_aabb; }
    
    void setPosition(const glm::vec3& position);
    
    glm::mat4 getViewMatrix() const;
    
    bool isOnGround() const { return m_onGround; }
    void jump();
    
    float getEyeHeight() const { return m_eyeHeight; }
    
    void setSelectedBlockType(BlockType type) { m_selectedBlockType = type; }
    BlockType getSelectedBlockType() const { return m_selectedBlockType; }
    
private:
    void updateAABB();
    void applyGravity(float deltaTime);
    void resolveCollisions(ChunkManager* chunkManager);
    void resolveCollisionsAxis(ChunkManager* chunkManager, int axis);
    void handleBlockInteraction(GLFWwindow* window, ChunkManager* chunkManager);

    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_rotation; // pitch, yaw, roll
    glm::vec3 m_cameraFront;
    
    AABB m_aabb;
    
    static constexpr float PLAYER_WIDTH = 0.6f;
    static constexpr float PLAYER_HEIGHT = 1.8f;
    static constexpr float GRAVITY = 20.0f;
    static constexpr float JUMP_VELOCITY = 8.0f;
    static constexpr float MOVE_SPEED = 5.0f;
    static constexpr float MOUSE_SENSITIVITY = 0.002f;
    
    float m_eyeHeight = 1.65f;
    bool m_onGround = false;
    
    // Fly mode state
    bool m_isFlying = false;
    bool m_spacePressed = false;
    double m_lastSpacePress = 0.0;
    static constexpr double DOUBLE_PRESS_TIME = 0.3; // 300ms window for double press
    
    // Mouse state
    bool m_firstMouse = true;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    
    // Movement state
    bool m_isMoving = false;
    
    // Block interaction state
    BlockType m_selectedBlockType = BlockType::STONE;
    bool m_leftMousePressed = false;
    bool m_rightMousePressed = false;
    static constexpr float BLOCK_REACH = 5.0f;
};

} // namespace Zerith