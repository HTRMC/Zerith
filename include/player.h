#pragma once

#include <glm/glm.hpp>
#include "aabb.h"

struct GLFWwindow;

namespace Zerith {

class ChunkManager;

class Player {
public:
    Player(const glm::vec3& position = glm::vec3(0.0f, 10.0f, 0.0f));
    
    void update(float deltaTime, ChunkManager* chunkManager);
    void handleInput(GLFWwindow* window, float deltaTime);
    
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getVelocity() const { return m_velocity; }
    const glm::vec3& getRotation() const { return m_rotation; }
    const AABB& getAABB() const { return m_aabb; }
    
    void setPosition(const glm::vec3& position);
    
    glm::mat4 getViewMatrix() const;
    
    bool isOnGround() const { return m_onGround; }
    void jump();
    
    float getEyeHeight() const { return m_eyeHeight; }
    
private:
    void updateAABB();
    void applyGravity(float deltaTime);
    void resolveCollisions(ChunkManager* chunkManager);
    
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    glm::vec3 m_rotation; // pitch, yaw, roll
    
    AABB m_aabb;
    
    static constexpr float PLAYER_WIDTH = 0.6f;
    static constexpr float PLAYER_HEIGHT = 1.8f;
    static constexpr float GRAVITY = 20.0f;
    static constexpr float JUMP_VELOCITY = 8.0f;
    static constexpr float MOVE_SPEED = 5.0f;
    static constexpr float MOUSE_SENSITIVITY = 0.002f;
    
    float m_eyeHeight = 1.65f;
    bool m_onGround = false;
    
    // Mouse state
    bool m_firstMouse = true;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
};

} // namespace Zerith