#include "block_types.h"
#include "player.h"
#include "chunk_manager.h"
#include "raycast.h"
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
        
        // Update which block we're looking at
        if (chunkManager) {
            // Calculate camera direction
            glm::vec3 cameraFront;
            cameraFront.x = cos(m_rotation.y) * cos(m_rotation.x);
            cameraFront.y = sin(m_rotation.x);
            cameraFront.z = sin(m_rotation.y) * cos(m_rotation.x);
            cameraFront = glm::normalize(cameraFront);
            
            // Get eye position
            glm::vec3 eyePosition = m_position + glm::vec3(0.0f, m_eyeHeight, 0.0f);
            
            // Perform raycast to find the block we're looking at
            auto hit = Raycast::cast(eyePosition, cameraFront, BLOCK_REACH, chunkManager);
            
            if (hit.has_value()) {
                m_hasLookedAtBlock = true;
                m_lookedAtBlockPos = hit->blockPos;
            } else {
                m_hasLookedAtBlock = false;
            }
        } else {
            m_hasLookedAtBlock = false;
        }
        
        // Calculate desired movement
        glm::vec3 deltaPosition = m_velocity * deltaTime;
        
        // Apply movement with collision detection
        if (chunkManager && glm::length(deltaPosition) > 0.0f) {
            // Move and resolve collisions in each axis separately for better sliding
            // This prevents getting stuck on edges
            
            // Move X
            if (std::abs(deltaPosition.x) > 0.0f) {
                m_position.x += deltaPosition.x;
                updateAABB();
                resolveCollisionsAxis(chunkManager, 0); // X axis
            }
            
            // Move Y
            if (std::abs(deltaPosition.y) > 0.0f) {
                m_position.y += deltaPosition.y;
                updateAABB();
                resolveCollisionsAxis(chunkManager, 1); // Y axis
            }
            
            // Move Z
            if (std::abs(deltaPosition.z) > 0.0f) {
                m_position.z += deltaPosition.z;
                updateAABB();
                resolveCollisionsAxis(chunkManager, 2); // Z axis
            }
        } else {
            // No collision detection
            m_position += deltaPosition;
            updateAABB();
        }
    }

    void Player::handleInput(GLFWwindow *window, float deltaTime, ChunkManager* chunkManager) {
        glm::vec3 movement(0.0f);

        // Input mapping (same as before)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            movement.z += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            movement.z -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            movement.x -= 1.0f; // Left
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            movement.x += 1.0f; // Right
        }

        // Handle space key for jumping and fly toggle
        bool spaceCurrentlyPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        double currentTime = glfwGetTime();
        
        if (spaceCurrentlyPressed && !m_spacePressed) {
            // Space key just pressed
            if (currentTime - m_lastSpacePress < DOUBLE_PRESS_TIME) {
                // Double press detected - toggle fly mode
                m_isFlying = !m_isFlying;
                if (m_isFlying) {
                    m_velocity.y = 0.0f; // Stop falling when entering fly mode
                    LOG_INFO("Fly mode enabled");
                } else {
                    LOG_INFO("Fly mode disabled");
                }
            } else {
                // Single press
                if (m_isFlying) {
                    // In fly mode, space moves up
                    m_velocity.y = m_flySpeed;
                } else if (m_onGround) {
                    // Normal jump when on ground and not flying
                    jump();
                }
            }
            m_lastSpacePress = currentTime;
        }
        
        m_spacePressed = spaceCurrentlyPressed;
        
        // Handle continuous vertical movement in fly mode
        if (m_isFlying) {
            if (spaceCurrentlyPressed) {
                // Continuous upward movement while space is held
                m_velocity.y = m_flySpeed;
            } else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                // Continuous downward movement while shift is held
                m_velocity.y = -m_flySpeed;
            } else {
                // Stop vertical movement when neither key is pressed
                m_velocity.y = 0.0f;
            }
        }

        // Calculate camera direction vectors
        glm::vec3 cameraFront;
        cameraFront.x = cos(m_rotation.y) * cos(m_rotation.x);
        cameraFront.y = sin(m_rotation.x);
        cameraFront.z = sin(m_rotation.y) * cos(m_rotation.x);
        cameraFront = glm::normalize(cameraFront);

        // Create horizontal front vector (zero out vertical component)
        glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

        // Handle edge case when looking straight up/down
        if (glm::length(horizontalFront) < 0.1f) {
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::cross(cameraFront, worldUp);
            horizontalFront = glm::normalize(glm::cross(worldUp, right));
        }

        // Calculate right vector using cross product
        glm::vec3 right = glm::normalize(glm::cross(horizontalFront, glm::vec3(0.0f, 1.0f, 0.0f)));

        // Track if player is trying to move
        m_isMoving = (glm::length(movement) > 0.0f);
        
        // Normalize movement input
        if (m_isMoving) {
            movement = glm::normalize(movement);
        }

        // Apply movement with acceleration for smooth start/stop
        const float ACCELERATION = m_onGround ? 100.0f : 20.0f; // Much faster acceleration on ground
        const float DECELERATION = m_onGround ? 15.0f : 5.0f; // Deceleration when stopping
        const float MAX_SPEED = m_isFlying ? m_flySpeed : MOVE_SPEED;
        
        if (m_isFlying) {
            // In fly mode, use horizontal direction vectors for horizontal movement
            glm::vec3 targetVelocity(0.0f);
            
            if (movement.z != 0.0f) {
                targetVelocity += horizontalFront * movement.z * MAX_SPEED;
            }
            if (movement.x != 0.0f) {
                targetVelocity += right * movement.x * MAX_SPEED;
            }
            
            // Preserve the vertical velocity that was set by space/shift keys
            float preservedVerticalVelocity = m_velocity.y;
            
            // Smooth interpolation to target velocity (horizontal only)
            glm::vec3 velocityDiff = targetVelocity - m_velocity;
            velocityDiff.y = 0.0f; // Don't change vertical component
            glm::vec3 acceleration = velocityDiff * ACCELERATION * deltaTime;
            
            // Apply acceleration
            m_velocity += acceleration;
            
            // Restore the vertical velocity
            m_velocity.y = preservedVerticalVelocity;
            
            // Clamp horizontal speed only to preserve vertical velocity
            glm::vec2 horizontalVel(m_velocity.x, m_velocity.z);
            if (glm::length(horizontalVel) > m_flySpeed) {
                horizontalVel = glm::normalize(horizontalVel) * m_flySpeed;
                m_velocity.x = horizontalVel.x;
                m_velocity.z = horizontalVel.y;
            }
        } else {
            // Normal ground movement - only horizontal
            glm::vec3 targetHorizontalVel(0.0f);
            
            if (movement.z != 0.0f) {
                targetHorizontalVel.x += horizontalFront.x * movement.z * MAX_SPEED;
                targetHorizontalVel.z += horizontalFront.z * movement.z * MAX_SPEED;
            }
            if (movement.x != 0.0f) {
                targetHorizontalVel.x += right.x * movement.x * MAX_SPEED;
                targetHorizontalVel.z += right.z * movement.x * MAX_SPEED;
            }
            
            // Smooth interpolation for horizontal movement
            if (m_isMoving) {
                // Accelerate towards target velocity when moving
                glm::vec2 currentHorizontal(m_velocity.x, m_velocity.z);
                glm::vec2 targetHorizontal(targetHorizontalVel.x, targetHorizontalVel.z);
                glm::vec2 horizontalDiff = targetHorizontal - currentHorizontal;
                glm::vec2 horizontalAccel = horizontalDiff * ACCELERATION * deltaTime;
                
                m_velocity.x += horizontalAccel.x;
                m_velocity.z += horizontalAccel.y;
            } else {
                // Decelerate to stop when not moving
                glm::vec2 currentHorizontal(m_velocity.x, m_velocity.z);
                float currentSpeed = glm::length(currentHorizontal);
                
                if (currentSpeed > 0.1f) {
                    glm::vec2 decelDirection = -glm::normalize(currentHorizontal);
                    glm::vec2 deceleration = decelDirection * DECELERATION * deltaTime;
                    
                    // Don't overshoot zero
                    if (glm::length(deceleration) > currentSpeed) {
                        m_velocity.x = 0.0f;
                        m_velocity.z = 0.0f;
                    } else {
                        m_velocity.x += deceleration.x;
                        m_velocity.z += deceleration.y;
                    }
                } else {
                    // Stop completely when very slow
                    m_velocity.x = 0.0f;
                    m_velocity.z = 0.0f;
                }
            }
            
            // Clamp horizontal speed
            glm::vec2 horizVel(m_velocity.x, m_velocity.z);
            if (glm::length(horizVel) > MAX_SPEED) {
                horizVel = glm::normalize(horizVel) * MAX_SPEED;
                m_velocity.x = horizVel.x;
                m_velocity.z = horizVel.y;
            }
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
        
        // Handle block interaction
        handleBlockInteraction(window, chunkManager);
        
        // Handle block selection with number keys
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::STONE;
        } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::DIRT;
        } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::GRASS_BLOCK;
        } else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::OAK_PLANKS;
        }
        else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::OAK_SLAB;
        } else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::OAK_STAIRS;
        } else if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::OAK_LOG;
        } else if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::OAK_LEAVES;
        } else if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::CRAFTING_TABLE;
        } else if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
            m_selectedBlockType = BlockTypes::GLASS;
        }
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
        // Use the bottom center of player as position reference
        // This makes collision calculations more stable
        glm::vec3 size(PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_WIDTH);
        glm::vec3 center = m_position + glm::vec3(0.0f, PLAYER_HEIGHT * 0.5f, 0.0f);
        
        // Calculate AABB directly to avoid precision issues
        m_aabb.min = center - size * 0.5f;
        m_aabb.max = center + size * 0.5f;
    }

    void Player::applyGravity(float deltaTime) {
        if (!m_isFlying) {
            m_velocity.y -= GRAVITY * deltaTime;
        }
    }

    void Player::resolveCollisions(ChunkManager *chunkManager) {
        if (!chunkManager) return;

        m_onGround = false;

        // Get nearby blocks
        AABB expandedAABB = m_aabb;
        expandedAABB.min -= glm::vec3(1.0f);
        expandedAABB.max += glm::vec3(1.0f);

        std::vector<AABB> blockAABBs = CollisionSystem::getBlockAABBsInRegion(expandedAABB, chunkManager);

        // Small epsilon to prevent floating point issues
        const float EPSILON = 0.001f;

        for (const AABB &blockAABB: blockAABBs) {
            CollisionSystem::CollisionResult collision = CollisionSystem::checkAABBCollision(m_aabb, blockAABB);

            if (collision.hasCollision) {
                // Calculate penetration depth for each axis
                glm::vec3 penetration = collision.penetration * collision.normal;
                
                // Only resolve collisions if not flying (or if vertical collision when not flying)
                if (!m_isFlying || collision.normal.y != 0.0f) {
                    // Smooth position correction - push out by penetration amount plus small epsilon
                    m_position += collision.normal * (glm::length(penetration) + EPSILON);
                    
                    // Handle ground detection
                    if (collision.normal.y > 0.5f && !m_isFlying) {
                        m_onGround = true;
                        m_velocity.y = std::min(m_velocity.y, 0.0f); // Only stop downward velocity
                    } else if (collision.normal.y < -0.5f && !m_isFlying) {
                        m_velocity.y = std::max(m_velocity.y, 0.0f); // Only stop upward velocity
                    }
                    
                    // For horizontal collisions, only stop velocity in the collision direction
                    if (std::abs(collision.normal.x) > 0.5f) {
                        // Project velocity to remove component in collision direction
                        float dot = glm::dot(m_velocity, collision.normal);
                        if (dot < 0) { // Only if moving towards the collision
                            m_velocity -= collision.normal * dot;
                        }
                    }
                    
                    if (std::abs(collision.normal.z) > 0.5f) {
                        // Project velocity to remove component in collision direction
                        float dot = glm::dot(m_velocity, collision.normal);
                        if (dot < 0) { // Only if moving towards the collision
                            m_velocity -= collision.normal * dot;
                        }
                    }
                    
                    // Update AABB after position change
                    updateAABB();
                }
            }
        }
        
        // Don't apply constant friction - let the acceleration system handle deceleration
    }
    
    void Player::resolveCollisionsAxis(ChunkManager *chunkManager, int axis) {
        if (!chunkManager) return;
        
        // Get nearby blocks - use tighter bounds for better performance
        AABB searchAABB = m_aabb;
        searchAABB.min -= glm::vec3(0.1f);
        searchAABB.max += glm::vec3(0.1f);
        
        std::vector<AABB> blockAABBs = CollisionSystem::getBlockAABBsInRegion(searchAABB, chunkManager);
        
        const float EPSILON = 0.001f;
        
        for (const AABB &blockAABB: blockAABBs) {
            if (m_aabb.intersects(blockAABB)) {
                // Calculate overlap on the specific axis
                float overlap = 0.0f;
                float direction = 0.0f;
                
                switch(axis) {
                    case 0: // X axis
                        if (m_velocity.x > 0) {
                            // Moving right
                            overlap = m_aabb.max.x - blockAABB.min.x;
                            if (overlap > 0 && overlap < PLAYER_WIDTH) {
                                m_position.x = blockAABB.min.x - PLAYER_WIDTH * 0.5f - EPSILON;
                                m_velocity.x = 0.0f;
                            }
                        } else if (m_velocity.x < 0) {
                            // Moving left
                            overlap = blockAABB.max.x - m_aabb.min.x;
                            if (overlap > 0 && overlap < PLAYER_WIDTH) {
                                m_position.x = blockAABB.max.x + PLAYER_WIDTH * 0.5f + EPSILON;
                                m_velocity.x = 0.0f;
                            }
                        }
                        break;
                        
                    case 1: // Y axis
                        if (m_velocity.y > 0) {
                            // Moving up
                            overlap = m_aabb.max.y - blockAABB.min.y;
                            if (overlap > 0 && overlap < PLAYER_HEIGHT) {
                                m_position.y = blockAABB.min.y - PLAYER_HEIGHT - EPSILON;
                                m_velocity.y = 0.0f;
                            }
                        } else if (m_velocity.y < 0) {
                            // Moving down
                            overlap = blockAABB.max.y - m_aabb.min.y;
                            if (overlap > 0 && overlap < PLAYER_HEIGHT) {
                                m_position.y = blockAABB.max.y + EPSILON;
                                m_velocity.y = 0.0f;
                                m_onGround = true;
                            }
                        }
                        break;
                        
                    case 2: // Z axis
                        if (m_velocity.z > 0) {
                            // Moving forward
                            overlap = m_aabb.max.z - blockAABB.min.z;
                            if (overlap > 0 && overlap < PLAYER_WIDTH) {
                                m_position.z = blockAABB.min.z - PLAYER_WIDTH * 0.5f - EPSILON;
                                m_velocity.z = 0.0f;
                            }
                        } else if (m_velocity.z < 0) {
                            // Moving backward
                            overlap = blockAABB.max.z - m_aabb.min.z;
                            if (overlap > 0 && overlap < PLAYER_WIDTH) {
                                m_position.z = blockAABB.max.z + PLAYER_WIDTH * 0.5f + EPSILON;
                                m_velocity.z = 0.0f;
                            }
                        }
                        break;
                }
                
                // Update AABB after position adjustment
                updateAABB();
            }
        }
    }

    void Player::handleBlockInteraction(GLFWwindow* window, ChunkManager* chunkManager) {
        if (!chunkManager) return;
        
        // Check mouse button states
        bool leftMouseCurrentlyPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightMouseCurrentlyPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        
        // Handle left click (destroy block)
        if (leftMouseCurrentlyPressed && !m_leftMousePressed) {
            LOG_DEBUG("Left mouse clicked - attempting to destroy block");
            
            // Calculate camera direction
            glm::vec3 cameraFront;
            cameraFront.x = cos(m_rotation.y) * cos(m_rotation.x);
            cameraFront.y = sin(m_rotation.x);
            cameraFront.z = sin(m_rotation.y) * cos(m_rotation.x);
            cameraFront = glm::normalize(cameraFront);
            
            // Get eye position
            glm::vec3 eyePosition = m_position + glm::vec3(0.0f, m_eyeHeight, 0.0f);
            
            LOG_DEBUG("Raycast from eye pos (%.2f, %.2f, %.2f) direction (%.2f, %.2f, %.2f)", 
                     eyePosition.x, eyePosition.y, eyePosition.z,
                     cameraFront.x, cameraFront.y, cameraFront.z);
            
            // Perform raycast
            auto hit = Raycast::cast(eyePosition, cameraFront, BLOCK_REACH, chunkManager);
            
            if (hit.has_value()) {
                LOG_DEBUG("Raycast hit block %d at (%d, %d, %d), distance %.2f", 
                         hit->blockType, hit->blockPos.x, hit->blockPos.y, hit->blockPos.z, hit->distance);
                
                // Destroy the block
                chunkManager->setBlock(glm::vec3(hit->blockPos), BlockTypes::AIR);
                LOG_INFO("Block destroyed at (%d, %d, %d)", hit->blockPos.x, hit->blockPos.y, hit->blockPos.z);
            } else {
                LOG_DEBUG("Raycast did not hit any blocks");
            }
        }
        
        // Handle right click (place block)
        if (rightMouseCurrentlyPressed && !m_rightMousePressed) {
            LOG_DEBUG("Right mouse clicked - attempting to place block type %d", m_selectedBlockType);
            
            // Calculate camera direction
            glm::vec3 cameraFront;
            cameraFront.x = cos(m_rotation.y) * cos(m_rotation.x);
            cameraFront.y = sin(m_rotation.x);
            cameraFront.z = sin(m_rotation.y) * cos(m_rotation.x);
            cameraFront = glm::normalize(cameraFront);
            
            // Get eye position
            glm::vec3 eyePosition = m_position + glm::vec3(0.0f, m_eyeHeight, 0.0f);
            
            // Perform raycast
            auto hit = Raycast::cast(eyePosition, cameraFront, BLOCK_REACH, chunkManager);
            
            if (hit.has_value()) {
                // Place block adjacent to the hit block using the surface normal
                glm::ivec3 placePos = hit->blockPos + hit->normal;
                
                LOG_DEBUG("Raycast hit block at (%d, %d, %d), normal (%d, %d, %d), placing at (%d, %d, %d)", 
                         hit->blockPos.x, hit->blockPos.y, hit->blockPos.z,
                         hit->normal.x, hit->normal.y, hit->normal.z,
                         placePos.x, placePos.y, placePos.z);
                
                // Check if placement position would intersect with player
                AABB blockAABB;
                blockAABB.min = glm::vec3(placePos);
                blockAABB.max = blockAABB.min + glm::vec3(1.0f);
                
                if (!m_aabb.intersects(blockAABB)) {
                    chunkManager->setBlock(glm::vec3(placePos), m_selectedBlockType);
                    LOG_INFO("Block type %d placed at (%d, %d, %d)", m_selectedBlockType, placePos.x, placePos.y, placePos.z);
                } else {
                    LOG_DEBUG("Cannot place block at (%d, %d, %d) - would intersect with player", 
                             placePos.x, placePos.y, placePos.z);
                }
            } else {
                LOG_DEBUG("Raycast did not hit any blocks for placement");
            }
        }
        
        // Update button states
        m_leftMousePressed = leftMouseCurrentlyPressed;
        m_rightMousePressed = rightMouseCurrentlyPressed;
    }

    void Player::handleScrollInput(double xoffset, double yoffset) {
        if (m_isFlying) {
            if (yoffset > 0) {
                // Scroll up - increase fly speed
                m_flySpeed *= FLY_SPEED_MULTIPLIER;
                if (m_flySpeed > MAX_FLY_SPEED) {
                    m_flySpeed = MAX_FLY_SPEED;
                }
                LOG_DEBUG("Fly speed increased to %.1f", m_flySpeed);
            } else if (yoffset < 0) {
                // Scroll down - decrease fly speed
                m_flySpeed /= FLY_SPEED_MULTIPLIER;
                if (m_flySpeed < MIN_FLY_SPEED) {
                    m_flySpeed = MIN_FLY_SPEED;
                }
                LOG_DEBUG("Fly speed decreased to %.1f", m_flySpeed);
            }
        }
    }
} // namespace Zerith
