#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "blocks/BlockModel.h"
#include "blocks/BlockType.h"
#include "world/ World.h"
#include "gui/GUIRenderer.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#include <Windows.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

World world;

// Camera movement enum
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

bool isPaused = false;
float gameTime = 0.0f;
float lastPauseTime = 0.0f;

// Camera class
class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    glm::vec3 velocity;

    float yaw;
    float pitch;

    // Constants for movement
    const float TICK_RATE = 20.0f; // Minecraft runs at 20 ticks per second
    const float BASE_ACCELERATION = 0.049f; // Matches Minecraft's flying acceleration
    const float AIR_FRICTION = 0.91f; // Matches Minecraft's air resistance
    const float MAX_SPEED = 10.79f; // Matches Minecraft's max flying speed

    float mouseSensitivity;

    enum class GameMode {
        CREATIVE,
        SPECTATOR
    };

    enum class MovementMode {
        WALKING,
        FLYING
    };

    GameMode currentGameMode = GameMode::CREATIVE;
    MovementMode currentMode = MovementMode::WALKING;

    bool isGrounded = false;
    float lastJumpTime = 0.0f;
    const float DOUBLE_JUMP_TIME = 0.3f; // Time window for double jump in seconds
    const float GRAVITY = -20.0f;
    float verticalVelocity = 0.0f;
    bool canFly = true; // Set to false for survival mode
    const float EYE_HEIGHT = 1.62f;
    const float PLAYER_HEIGHT = 1.8f;
    const float PLAYER_WIDTH = 0.6f;
    const float PLAYER_HALF_WIDTH = PLAYER_WIDTH / 2.0f;
    const float PLAYER_HALF_HEIGHT = PLAYER_HEIGHT / 2.0f;
    const float JUMP_VELOCITY = 8.0f;

    void onPause() {
        // Store the current state when pausing
        pausedVelocity = velocity;
        pausedVerticalVelocity = verticalVelocity;
        pausedPosition = position;
        pausedIsGrounded = isGrounded;
        // Reset accumulator to prevent time buildup
        accumulator = 0.0f;
    }

    void onUnpause() {
        // Restore the exact state from when we paused
        velocity = pausedVelocity;
        verticalVelocity = pausedVerticalVelocity;
        position = pausedPosition;
        isGrounded = pausedIsGrounded;
        // Reset accumulator
        accumulator = 0.0f;
    }

    void applyGravity(float deltaTime) {
        if (currentMode == MovementMode::WALKING && !isGrounded) {
            verticalVelocity += GRAVITY * deltaTime;
            position.y += verticalVelocity * deltaTime;

            // Check if we hit the ground (y = 0)
            if (position.y <= 1.0f) {
                // 1.0f because player height is 1.8
                position.y = 1.0f;
                verticalVelocity = 0.0f;
                isGrounded = true;
            }
        }
    }

    void jump(float currentTime) {
        if (currentMode == MovementMode::WALKING) {
            if (isGrounded) {
                verticalVelocity = JUMP_VELOCITY;
                isGrounded = false;

                // Check for double jump
                if (currentTime - lastJumpTime < DOUBLE_JUMP_TIME && canFly) {
                    currentMode = MovementMode::FLYING;
                    velocity = glm::vec3(0.0f);
                    verticalVelocity = 0.0f;
                }
                lastJumpTime = currentTime;
            }
        } else if (currentMode == MovementMode::FLYING) {
            // Double tap space in flying mode returns to walking
            if (currentTime - lastJumpTime < DOUBLE_JUMP_TIME) {
                currentMode = MovementMode::WALKING;
                velocity = glm::vec3(0.0f);
                verticalVelocity = 0.0f;
            }
            lastJumpTime = currentTime;
        }
    }

    void setGameMode(GameMode mode) {
        currentGameMode = mode;

        switch (currentGameMode) {
            case GameMode::SPECTATOR:
                currentMode = MovementMode::FLYING; // Force flying in spectator
                velocity = glm::vec3(0.0f);
                verticalVelocity = 0.0f;
                break;

            case GameMode::CREATIVE:
                // Keep current movement mode but reset velocities
                velocity = glm::vec3(0.0f);
                verticalVelocity = 0.0f;
                break;
        }
    }

    bool canBreakBlocks() const {
        return currentGameMode == GameMode::CREATIVE;
    }

    bool canPlaceBlocks() const {
        return currentGameMode == GameMode::CREATIVE;
    }


    AABB getBoundingBox() const {
        if (currentGameMode == GameMode::SPECTATOR) {
            return AABB(glm::vec3(0.0f), glm::vec3(0.0f)); // Zero-sized box
        }

        // Calculate the player's feet position (offset from eye position)
        glm::vec3 feetPos = position - glm::vec3(0.0f, EYE_HEIGHT, 0.0f);

        // Create the bounding box centered on the player's center of mass
        glm::vec3 center = feetPos + glm::vec3(0.0f, PLAYER_HALF_HEIGHT, 0.0f);
        glm::vec3 halfExtents(PLAYER_HALF_WIDTH, PLAYER_HALF_HEIGHT, PLAYER_HALF_WIDTH);

        return AABB(center - halfExtents, center + halfExtents);
    }

    bool checkCollision(const glm::vec3 &newPosition) {
        // Skip collision check in spectator mode
        if (currentGameMode == GameMode::SPECTATOR) {
            return false;
        }

        // Convert eye position to feet position for collision check
        glm::vec3 feetPos = newPosition - glm::vec3(0.0f, EYE_HEIGHT, 0.0f);

        // Create AABB for proposed position
        glm::vec3 center = feetPos + glm::vec3(0.0f, PLAYER_HALF_HEIGHT, 0.0f);
        glm::vec3 halfExtents(PLAYER_HALF_WIDTH, PLAYER_HALF_HEIGHT, PLAYER_HALF_WIDTH);
        AABB playerBox(center - halfExtents, center + halfExtents);

        return world.checkCollision(playerBox);
    }

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f))
        : position(position)
          , front(glm::vec3(0.0f, 0.0f, -1.0f))
          , worldUp(glm::vec3(0.0f, 1.0f, 0.0f))
          , velocity(glm::vec3(0.0f))
          , yaw(-90.0f)
          , pitch(0.0f)
          , mouseSensitivity(0.1f) {
        // Adjust initial position to account for eye height
        position.y += EYE_HEIGHT;
        updateCameraVectors();
    }

    void updateCameraVectors() {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(direction);
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    const float FIXED_TIMESTEP = 1.0f / 60.0f; // 60 Hz physics update
    float accumulator = 0.0f;

    void updateWithFixedTimestep(const glm::vec3 &moveDir, float deltaTime) {
        if (isPaused) return;

        // Accumulate the frame time
        accumulator += deltaTime;

        // Update physics in fixed timesteps while we have accumulated enough time
        while (accumulator >= FIXED_TIMESTEP) {
            updatePhysics(moveDir, FIXED_TIMESTEP);
            accumulator -= FIXED_TIMESTEP;
        }

        // Optional: Interpolate state for smooth rendering
        // float alpha = accumulator / FIXED_TIMESTEP;
        // Could interpolate position/rotation here if needed
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
        }

        updateCameraVectors();
    }

private:
    glm::vec3 pausedVelocity{0.0f};
    float pausedVerticalVelocity = 0.0f;
    glm::vec3 pausedPosition{0.0f};
    bool pausedIsGrounded = false;

    void updatePhysics(const glm::vec3 &moveDir, float dt) {
        // Convert real time to minecraft ticks (now using fixed timestep)
        float ticks = dt * TICK_RATE;

        // Apply acceleration if there's input
        if (glm::length(moveDir) > 0.0f) {
            glm::vec3 normalizedDir = glm::normalize(moveDir);
            velocity += normalizedDir * (BASE_ACCELERATION * ticks);
        }

        // Apply air friction (per tick)
        float frictionFactor = pow(AIR_FRICTION, ticks);
        velocity *= frictionFactor;

        // Calculate new position but don't apply it yet
        glm::vec3 newPosition = position + velocity;

        if (currentGameMode == GameMode::SPECTATOR) {
            // In spectator mode, just update position directly
            position = newPosition;
            return;
        }

        // Apply movement mode specific behavior
        if (currentMode == MovementMode::WALKING) {
            velocity.y = 0; // Don't accumulate Y velocity in walking mode

            // Apply gravity with fixed timestep
            if (!isGrounded) {
                verticalVelocity += GRAVITY * dt;
            }
            newPosition.y += verticalVelocity * dt;
        }

        // Handle collisions by checking each axis separately
        glm::vec3 finalPosition = position;

        // Try X movement
        glm::vec3 xMovement = position;
        xMovement.x = newPosition.x;
        if (!checkCollision(xMovement)) {
            finalPosition.x = xMovement.x;
        } else {
            velocity.x = 0;
        }

        // Try Y movement
        glm::vec3 yMovement = finalPosition;
        yMovement.y = newPosition.y;
        if (!checkCollision(yMovement)) {
            finalPosition.y = yMovement.y;
            isGrounded = false;
        } else {
            if (verticalVelocity < 0) {
                isGrounded = true;
                verticalVelocity = 0;
            } else {
                verticalVelocity = 0;
            }
        }

        // Try Z movement
        glm::vec3 zMovement = finalPosition;
        zMovement.z = newPosition.z;
        if (!checkCollision(zMovement)) {
            finalPosition.z = zMovement.z;
        } else {
            velocity.z = 0;
        }

        // Update final position
        position = finalPosition;

        // Check if we're grounded
        if (currentMode == MovementMode::WALKING && !isGrounded) {
            glm::vec3 groundCheck = position - glm::vec3(0.0f, 0.1f, 0.0f);
            if (checkCollision(groundCheck)) {
                isGrounded = true;
                verticalVelocity = 0;
            }
        }
    }
};

// Shader class to handle compilation and usage
class Shader {
public:
    unsigned int ID;

    void setVec3(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int) value);
    }

    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setIsTransparent(bool value) {
        glUniform1i(glGetUniformLocation(ID, "isTransparent"), (int) value);
    }

    Shader(const char *vertexPath, const char *fragmentPath) {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();

        const char *vShaderCode = vertexCode.c_str();
        const char *fShaderCode = fragmentCode.c_str();

        unsigned int vertex, fragment;
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() {
        glUseProgram(ID);
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
            }
        }
    }
};

Shader *guiShader;
GUIRenderer *guiRenderer;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    if (guiRenderer) {
        guiRenderer->updateScreenSize(width, height);
    }
}

// Add these global variables at the top of your file
float fps = 0.0f;
int frameCount = 0;
float lastTime = 0.0f;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::ivec3 highlightedBlock(-1);
float lastSpacePressTime = 0.0f; // Time of the last space key press
const float DOUBLE_TAP_THRESHOLD = 0.3f; // Maximum time interval for double-tap in seconds

std::string chatInput;
bool isChatOpen = false;
BlockType selectedBlockType = BlockType::STONE; // Default block type
bool acceptingInput = false;

// Add this function to handle text input
void character_callback(GLFWwindow *window, unsigned int codepoint) {
    if (isChatOpen && acceptingInput) {
        chatInput += static_cast<char>(codepoint);
    }
}

// Add this function to process chat commands
void processCommand(const std::string &command) {
    if (command.empty() || command[0] != '/') return;

    std::istringstream iss(command.substr(1)); // Remove the leading '/'
    std::string action;
    iss >> action;

    if (action == "give") {
        std::string blockName;
        iss >> blockName;

        static const std::unordered_map<std::string, BlockType> blockTypes = {
            {"stone", BlockType::STONE},
            {"dirt", BlockType::DIRT},
            {"grass_block", BlockType::GRASS_BLOCK},
            {"oak_planks", BlockType::OAK_PLANKS},
            {"oak_slab", BlockType::OAK_SLAB},
            {"oak_stairs", BlockType::OAK_STAIRS},
            {"oak_log", BlockType::OAK_LOG},
            {"glass", BlockType::GLASS}
        };

        auto it = blockTypes.find(blockName);
        if (it != blockTypes.end()) {
            selectedBlockType = it->second;
            std::cout << "Selected block type: " << blockName << std::endl;
        }
    } else if (action == "gamemode") {
        std::string mode;
        iss >> mode;

        if (mode == "creative" || mode == "1") {
            camera.setGameMode(Camera::GameMode::CREATIVE);
            std::cout << "Game mode set to Creative" << std::endl;
        } else if (mode == "spectator" || mode == "3") {
            camera.setGameMode(Camera::GameMode::SPECTATOR);
            std::cout << "Game mode set to Spectator" << std::endl;
        }
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (isChatOpen) {
            // Close chat if it's open
            chatInput.clear();
            isChatOpen = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            // Handle pause menu
            isPaused = !isPaused;
            guiRenderer->setPauseMenuOpen(isPaused);

            if (isPaused) {
                camera.onPause();
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                glfwSetCursorPos(window, SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0);
                lastPauseTime = static_cast<float>(glfwGetTime());
            } else {
                float currentTime = static_cast<float>(glfwGetTime());
                float pauseDuration = currentTime - lastPauseTime;
                lastFrame += pauseDuration;
                lastPauseTime = 0;
                camera.onUnpause();
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
    } else if ((key == GLFW_KEY_T || key == GLFW_KEY_SLASH) && action == GLFW_PRESS && !isChatOpen && !isPaused) {
        isChatOpen = true;
        chatInput = (key == GLFW_KEY_SLASH) ? "/" : ""; // Initialize with '/' only if slash key was pressed
        acceptingInput = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else if (isChatOpen && !acceptingInput) {
        acceptingInput = true; // Enable input on any subsequent key press
    } else if (isChatOpen) {
        if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS) {
            if (!chatInput.empty()) {
                chatInput.pop_back();
            }
        } else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
            processCommand(chatInput);
            chatInput.clear();
            isChatOpen = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    if (isChatOpen) return;
    if (isPaused) {
        guiRenderer->updateButtonHover(xpos, ypos);
    } else if (!isChatOpen) {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;
        camera.processMouseMovement(xoffset, yoffset);
    }
}

// Ray casting function to detect block intersection
bool raycastBlock(const glm::vec3 &start, const glm::vec3 &direction, float maxDistance,
                  glm::ivec3 &outBlockPos, glm::vec3 &outHitPos) {
    glm::vec3 rayDir = glm::normalize(direction);
    glm::vec3 rayPos = start;
    const float STEP_SIZE = 0.05f;
    glm::vec3 rayStep = rayDir * STEP_SIZE;

    float distance = 0.0f;
    while (distance < maxDistance) {
        rayPos += rayStep;
        distance += STEP_SIZE;

        int gridX = static_cast<int>(floor(rayPos.x));
        int gridY = static_cast<int>(floor(rayPos.y));
        int gridZ = static_cast<int>(floor(rayPos.z));

        // Convert to chunk coordinates
        int chunkX = floor(float(gridX) / Chunk::CHUNK_SIZE);
        int chunkZ = floor(float(gridZ) / Chunk::CHUNK_SIZE);
        int localX = gridX - (chunkX * Chunk::CHUNK_SIZE);
        int localY = gridY;
        int localZ = gridZ - (chunkZ * Chunk::CHUNK_SIZE);

        // Check if the chunk exists
        auto it = world.chunks.find(glm::ivec2(chunkX, chunkZ));
        if (it != world.chunks.end() && localY >= 0 && localY < Chunk::CHUNK_SIZE) {
            // Check block existence directly from chunk data
            size_t index = it->second.getIndex(localX, localY, localZ);
            uint8_t blockData = it->second.blocks[index];
            bool exists = (blockData & 0x80) != 0;

            if (exists) {
                outBlockPos = glm::ivec3(gridX, gridY, gridZ);
                outHitPos = rayPos;
                return true;
            }
        }
    }

    outBlockPos = glm::ivec3(-1);
    outHitPos = glm::vec3(0.0f);
    return false;
}

// Add these callback functions after your existing framebuffer_size_callback
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (isPaused && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int clickedButton = guiRenderer->getClickedButton(xpos, ypos);

        if (clickedButton == 0) {
            // Resume button
            isPaused = false;
            guiRenderer->setPauseMenuOpen(false);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (clickedButton == 1) {
            // Quit button
            glfwSetWindowShouldClose(window, true);
        }
    } else if (!isPaused && !isChatOpen) {
        if (action == GLFW_PRESS) {
            glm::ivec3 blockPos;
            glm::vec3 hitPos;

            if (raycastBlock(camera.position, camera.front, 5.0f, blockPos, hitPos)) {
                Block *block = world.getBlock(blockPos.x, blockPos.y, blockPos.z);
                if (block) {
                    if (button == GLFW_MOUSE_BUTTON_LEFT && camera.canBreakBlocks()) {
                        // Break block
                        Block emptyBlock;
                        emptyBlock.exists = false;

                        // Convert to chunk coordinates
                        int chunkX = floor(float(blockPos.x) / Chunk::CHUNK_SIZE);
                        int chunkZ = floor(float(blockPos.z) / Chunk::CHUNK_SIZE);
                        int localX = blockPos.x - (chunkX * Chunk::CHUNK_SIZE);
                        int localY = blockPos.y;
                        int localZ = blockPos.z - (chunkZ * Chunk::CHUNK_SIZE);
                        glm::ivec2 chunkPos(chunkX, chunkZ);

                        // Set the block in the chunk
                        auto it = world.chunks.find(chunkPos);
                        if (it != world.chunks.end()) {
                            it->second.setBlock(localX, localY, localZ, emptyBlock);
                            it->second.needsRemesh = true;

                            // Check neighboring chunks
                            if (localX == 0) {
                                auto westChunk = world.chunks.find(glm::ivec2(chunkX - 1, chunkZ));
                                if (westChunk != world.chunks.end())
                                    westChunk->second.needsRemesh = true;
                            }
                            if (localX == Chunk::CHUNK_SIZE - 1) {
                                auto eastChunk = world.chunks.find(glm::ivec2(chunkX + 1, chunkZ));
                                if (eastChunk != world.chunks.end())
                                    eastChunk->second.needsRemesh = true;
                            }
                            if (localZ == 0) {
                                auto northChunk = world.chunks.find(glm::ivec2(chunkX, chunkZ - 1));
                                if (northChunk != world.chunks.end())
                                    northChunk->second.needsRemesh = true;
                            }
                            if (localZ == Chunk::CHUNK_SIZE - 1) {
                                auto southChunk = world.chunks.find(glm::ivec2(chunkX, chunkZ + 1));
                                if (southChunk != world.chunks.end())
                                    southChunk->second.needsRemesh = true;
                            }
                        }
                    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && camera.canPlaceBlocks()) {
                        // Calculate adjacent block position based on hit normal
                        glm::vec3 blockCenter = glm::vec3(blockPos) + glm::vec3(0.5f);
                        glm::vec3 normal = glm::normalize(hitPos - blockCenter);

                        int newX = blockPos.x + (normal.x > 0.5f ? 1 : (normal.x < -0.5f ? -1 : 0));
                        int newY = blockPos.y + (normal.y > 0.5f ? 1 : (normal.y < -0.5f ? -1 : 0));
                        int newZ = blockPos.z + (normal.z > 0.5f ? 1 : (normal.z < -0.5f ? -1 : 0));

                        Block *newBlock = world.getBlock(newX, newY, newZ);
                        if (newBlock && !newBlock->exists) {
                            Block placedBlock;

                            // Handle special block types
                            if (selectedBlockType == BlockType::OAK_LOG) {
                                // Determine log axis based on the face we're placing against
                                Axis axis;
                                if (abs(normal.x) > abs(normal.y) && abs(normal.x) > abs(normal.z)) {
                                    axis = Axis::X;
                                } else if (abs(normal.y) > abs(normal.x) && abs(normal.y) > abs(normal.z)) {
                                    axis = Axis::Y;
                                } else {
                                    axis = Axis::Z;
                                }
                                placedBlock = createOakLog(axis);
                            } else if (selectedBlockType == BlockType::OAK_STAIRS) {
                                float yaw = camera.yaw;
                                BlockFacing facing;
                                while (yaw < 0) yaw += 360.0f;
                                while (yaw >= 360.0f) yaw -= 360.0f;

                                if (yaw >= 315.0f || yaw < 45.0f) {
                                    facing = BlockFacing::NORTH;
                                } else if (yaw >= 45.0f && yaw < 135.0f) {
                                    facing = BlockFacing::WEST;
                                } else if (yaw >= 135.0f && yaw < 225.0f) {
                                    facing = BlockFacing::SOUTH;
                                } else {
                                    facing = BlockFacing::EAST;
                                }

                                StairHalf half = (camera.pitch > 0) ? StairHalf::TOP : StairHalf::BOTTOM;
                                placedBlock = createOakStairs(facing, half);
                            } else if (selectedBlockType == BlockType::OAK_SLAB) {
                                // Convert to chunk coordinates for new block
                                int chunkX = floor(float(newX) / Chunk::CHUNK_SIZE);
                                int chunkZ = floor(float(newZ) / Chunk::CHUNK_SIZE);
                                glm::ivec2 chunkPos(chunkX, chunkZ);
                                auto it = world.chunks.find(chunkPos);

                                // Check if we're clicking on an existing slab
                                Block *targetBlock = world.getBlock(blockPos.x, blockPos.y, blockPos.z);
                                if (targetBlock && targetBlock->exists && targetBlock->type == BlockType::OAK_SLAB) {
                                    SlabType existingType = std::get<SlabType>(
                                        targetBlock->properties.properties.at("type"));
                                    SlabType newType = determineSlabType(hitPos, glm::ivec3(blockPos));

                                    if ((existingType == SlabType::BOTTOM && newType == SlabType::TOP) ||
                                        (existingType == SlabType::TOP && newType == SlabType::BOTTOM)) {
                                        // Create a double slab
                                        placedBlock = createOakSlab(SlabType::DOUBLE);

                                        // Place in target position instead of new position
                                        int targetChunkX = floor(float(blockPos.x) / Chunk::CHUNK_SIZE);
                                        int targetChunkZ = floor(float(blockPos.z) / Chunk::CHUNK_SIZE);
                                        int localX = blockPos.x - (targetChunkX * Chunk::CHUNK_SIZE);
                                        int localY = blockPos.y;
                                        int localZ = blockPos.z - (targetChunkZ * Chunk::CHUNK_SIZE);

                                        auto targetChunk = world.chunks.find(glm::ivec2(targetChunkX, targetChunkZ));
                                        if (targetChunk != world.chunks.end()) {
                                            targetChunk->second.setBlock(localX, localY, localZ, placedBlock);
                                            targetChunk->second.needsRemesh = true;
                                        }
                                        return;
                                    }
                                }

                                SlabType type = determineSlabType(hitPos, glm::ivec3(newX, newY, newZ));
                                placedBlock = createOakSlab(type);
                            } else {
                                placedBlock = Block(selectedBlockType);
                            }

                            // Convert to chunk coordinates
                            int chunkX = floor(float(newX) / Chunk::CHUNK_SIZE);
                            int chunkZ = floor(float(newZ) / Chunk::CHUNK_SIZE);
                            int localX = newX - (chunkX * Chunk::CHUNK_SIZE);
                            int localY = newY;
                            int localZ = newZ - (chunkZ * Chunk::CHUNK_SIZE);
                            glm::ivec2 chunkPos(chunkX, chunkZ);

                            // Set the block in the chunk
                            auto it = world.chunks.find(chunkPos);
                            if (it != world.chunks.end()) {
                                placedBlock.exists = true;
                                it->second.setBlock(localX, localY, localZ, placedBlock);
                                it->second.needsRemesh = true;

                                // Check neighboring chunks
                                if (localX == 0) {
                                    auto westChunk = world.chunks.find(glm::ivec2(chunkX - 1, chunkZ));
                                    if (westChunk != world.chunks.end())
                                        westChunk->second.needsRemesh = true;
                                }
                                if (localX == Chunk::CHUNK_SIZE - 1) {
                                    auto eastChunk = world.chunks.find(glm::ivec2(chunkX + 1, chunkZ));
                                    if (eastChunk != world.chunks.end())
                                        eastChunk->second.needsRemesh = true;
                                }
                                if (localZ == 0) {
                                    auto northChunk = world.chunks.find(glm::ivec2(chunkX, chunkZ - 1));
                                    if (northChunk != world.chunks.end())
                                        northChunk->second.needsRemesh = true;
                                }
                                if (localZ == Chunk::CHUNK_SIZE - 1) {
                                    auto southChunk = world.chunks.find(glm::ivec2(chunkX, chunkZ + 1));
                                    if (southChunk != world.chunks.end())
                                        southChunk->second.needsRemesh = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// Modify your existing processInput function
void processInput(GLFWwindow *window) {
    float currentFrame = static_cast<float>(glfwGetTime());

    if (isChatOpen) return;

    // Only update game time and process movement if not paused
    if (!isPaused) {
        if (lastPauseTime > 0) {
            // Adjust lastFrame by the pause duration to prevent time jump
            float pauseDuration = currentFrame - lastPauseTime;
            lastFrame += pauseDuration;
            lastPauseTime = 0;
        }

        float actualDeltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        gameTime += actualDeltaTime;  // Only increment game time when not paused

        // Track movement direction
        glm::vec3 moveDir(0.0f);

        glm::vec3 horizontalFront = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
        glm::vec3 horizontalRight = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            moveDir += (camera.currentMode == Camera::MovementMode::FLYING) ? camera.front : horizontalFront;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            moveDir -= (camera.currentMode == Camera::MovementMode::FLYING) ? camera.front : horizontalFront;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            moveDir -= horizontalRight;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            moveDir += horizontalRight;
        }

        // Check for space double-tap and jumping
        static bool spaceWasPressed = false;
        bool spaceIsPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

        if (spaceIsPressed && !spaceWasPressed) {
            // Handle jumping for walking mode
            if (camera.currentMode == Camera::MovementMode::WALKING) {
                camera.jump(gameTime);  // Use gameTime instead of currentFrame
            }

            // Detect double-tap for flying toggle
            if (gameTime - lastSpacePressTime < DOUBLE_TAP_THRESHOLD) {
                if (camera.currentMode == Camera::MovementMode::FLYING) {
                    camera.currentMode = Camera::MovementMode::WALKING;
                } else {
                    camera.currentMode = Camera::MovementMode::FLYING;
                }
            }
            lastSpacePressTime = gameTime;  // Use gameTime instead of currentFrame
        }
        spaceWasPressed = spaceIsPressed;

        // Handle flying vertical movement
        if (camera.currentMode == Camera::MovementMode::FLYING) {
            if (spaceIsPressed) {
                moveDir += camera.worldUp;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                moveDir -= camera.worldUp;
            }
        }

        // Update camera movement with actual delta time
        camera.updateWithFixedTimestep(moveDir, actualDeltaTime);
    } else {
        // Store the time when we paused if we haven't already
        if (lastPauseTime == 0) {
            lastPauseTime = currentFrame;
        }
    }
}

unsigned int loadTexture(const char *path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        std::cout << "Failed to load texture at path: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

void setWindowIcon(GLFWwindow* window) {
    GLFWimage images[1];
    images[0].pixels = stbi_load("Zerith.png", &images[0].width, &images[0].height, 0, 4); //RGBA channels

    if (images[0].pixels) {
        glfwSetWindowIcon(window, 1, images);
        stbi_image_free(images[0].pixels);
    }
}

void setTitlebarColor(GLFWwindow* window, int r, int g, int b) {
    HWND hwnd = glfwGetWin32Window(window);

    // Enable dark mode for title bar
    BOOL value = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

    // Set the title bar color
    COLORREF color = RGB(r, g, b);
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &color, sizeof(color));
}

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Zerith", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    setWindowIcon(window);
    setTitlebarColor(window, 32, 32, 32);
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glfwSwapInterval(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CCW);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, character_callback);

    // Initialize GUI components
    guiShader = new Shader("shaders/gui_vertex_shader.glsl", "shaders/gui_fragment_shader.glsl");
    guiShader->use();
    guiShader->setInt("guiTexture", 15); // Texture unit 15 for GUI textures
    guiRenderer = new GUIRenderer(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Create and compile shaders
    Shader shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    std::vector<unsigned int> textureArray;
    textureArray.push_back(TextureManager::getTexture("block/dirt")); // 0
    textureArray.push_back(TextureManager::getTexture("block/stone")); // 1
    textureArray.push_back(TextureManager::getTexture("block/grass_block_side")); // 2
    textureArray.push_back(TextureManager::getTexture("block/grass_block_top")); // 3
    textureArray.push_back(TextureManager::getTexture("block/oak_planks")); // 4
    textureArray.push_back(TextureManager::getTexture("block/oak_log")); // 5
    textureArray.push_back(TextureManager::getTexture("block/oak_log_top")); // 6
    textureArray.push_back(TextureManager::getTexture("block/grass_block_side_overlay")); // 7
    textureArray.push_back(TextureManager::getTexture("block/glass")); // 8

    // Bind all textures to different texture units
    shader.use(); // Make sure shader is active when setting uniforms
    for (size_t i = 0; i < textureArray.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textureArray[i]);
        std::string uniformName = "blockTextures[" + std::to_string(i) + "]";
        GLint location = glGetUniformLocation(shader.ID, uniformName.c_str());
        if (location == -1) {
            std::cout << "Warning: Uniform " << uniformName << " not found!" << std::endl;
        }
        shader.setInt(uniformName, i);
    }

    // Create buffers/arrays
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) 0); // position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (3 * sizeof(float))); // color
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (6 * sizeof(float))); // texture
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *) (8 * sizeof(float))); // face index

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    // Move camera back to see the full grid
    camera.position = glm::vec3(8.0f, 20.0f, 8.0f);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.4157f, 0.9961f, 0.9961f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Create transformations
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(90.0f),
                                                static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT),
                                                0.1f, 1000.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // Ray casting for block highlighting
        glm::ivec3 blockPos;
        glm::vec3 hitPos;
        glm::vec3 rayDirection = glm::normalize(camera.front);
        bool lookingAtBlock = raycastBlock(camera.position, rayDirection, 5.0f, blockPos, hitPos);
        highlightedBlock = lookingAtBlock ? blockPos : glm::ivec3(-1);

        // First pass: Render opaque blocks
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        for (auto &[chunkPos, chunk]: world.chunks) {
            if (chunk.needsRemesh) {
                chunk.generateMesh();
            }

            glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                             glm::vec3(chunkPos.x * Chunk::CHUNK_SIZE, 0,
                                                       chunkPos.y * Chunk::CHUNK_SIZE));
            shader.setMat4("model", model);
            shader.setBool("useTexture", true);
            shader.setBool("isTransparent", false);

            // Check if highlighted block is in this chunk
            bool hasHighlight = false;
            if (highlightedBlock.x >= chunkPos.x * Chunk::CHUNK_SIZE &&
                highlightedBlock.x < (chunkPos.x + 1) * Chunk::CHUNK_SIZE &&
                highlightedBlock.z >= chunkPos.y * Chunk::CHUNK_SIZE &&
                highlightedBlock.z < (chunkPos.y + 1) * Chunk::CHUNK_SIZE) {
                hasHighlight = true;
            }

            shader.setBool("isHighlighted", hasHighlight);
            if (hasHighlight) {
                shader.setVec3("highlightedBlockPos", glm::vec3(
                                   highlightedBlock.x,
                                   highlightedBlock.y,
                                   highlightedBlock.z
                               ));
            }

            glBindVertexArray(chunk.opaqueVAO);
            glDrawArrays(GL_TRIANGLES, 0, chunk.opaqueVertexCount);
        }

        // Second pass: Render transparent blocks
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);

        // Sort chunks by distance from camera (back to front)
        std::vector<std::pair<glm::ivec2, Chunk *> > sortedChunks;
        glm::vec3 cameraPos = camera.position;
        for (auto &[chunkPos, chunk]: world.chunks) {
            glm::vec3 chunkCenter(
                chunkPos.x * Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE / 2.0f,
                Chunk::CHUNK_SIZE / 2.0f,
                chunkPos.y * Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE / 2.0f
            );
            float dist = glm::length(camera.position - chunkCenter);
            sortedChunks.push_back({chunkPos, &chunk});
        }

        // Sort using a lambda that compares distances
        std::sort(sortedChunks.begin(), sortedChunks.end(),
                  [cameraPos](const auto &a, const auto &b) {
                      glm::vec3 centerA(
                          a.first.x * Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE / 2.0f,
                          Chunk::CHUNK_SIZE / 2.0f,
                          a.first.y * Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE / 2.0f
                      );
                      glm::vec3 centerB(
                          b.first.x * Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE / 2.0f,
                          Chunk::CHUNK_SIZE / 2.0f,
                          b.first.y * Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE / 2.0f
                      );
                      float distA = glm::length(camera.position - centerA);
                      float distB = glm::length(camera.position - centerB);
                      return distA > distB; // Sort from back to front
                  });

        // Render transparent blocks from back to front
        for (const auto &[chunkPos, chunk]: sortedChunks) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                             glm::vec3(chunkPos.x * Chunk::CHUNK_SIZE, 0,
                                                       chunkPos.y * Chunk::CHUNK_SIZE));
            shader.setMat4("model", model);
            shader.setBool("useTexture", true);
            shader.setBool("isTransparent", true);

            // Check highlighting for transparent blocks too
            bool hasHighlight = false;
            if (highlightedBlock.x >= chunkPos.x * Chunk::CHUNK_SIZE &&
                highlightedBlock.x < (chunkPos.x + 1) * Chunk::CHUNK_SIZE &&
                highlightedBlock.z >= chunkPos.y * Chunk::CHUNK_SIZE &&
                highlightedBlock.z < (chunkPos.y + 1) * Chunk::CHUNK_SIZE) {
                hasHighlight = true;
            }

            shader.setBool("isHighlighted", hasHighlight);
            if (hasHighlight) {
                shader.setVec3("highlightedBlockPos", glm::vec3(
                                   highlightedBlock.x,
                                   highlightedBlock.y,
                                   highlightedBlock.z
                               ));
            }

            glBindVertexArray(chunk->transparentVAO);
            glDrawArrays(GL_TRIANGLES, 0, chunk->transparentVertexCount);
        }

        // Reset depth mask
        glDepthMask(GL_TRUE);

        if (isChatOpen) {
            // Render chat input with background
            glUseProgram(guiShader->ID);

            // Save OpenGL state
            GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
            GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

            // Setup for GUI rendering
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Draw chat background
            glBindVertexArray(guiRenderer->getButtonVAO());
            glUniform1i(glGetUniformLocation(guiShader->ID, "isButton"), 1);

            // Position the chat box at the bottom left
            glm::mat4 chatBgModel = glm::mat4(1.0f);
            chatBgModel = glm::translate(chatBgModel, glm::vec3(-0.7f, -0.8f, 0.0f));
            chatBgModel = glm::scale(chatBgModel, glm::vec3(0.6f, 0.1f, 1.0f));

            glUniformMatrix4fv(glGetUniformLocation(guiShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(chatBgModel));
            glUniform4f(glGetUniformLocation(guiShader->ID, "color"), 0.0f, 0.0f, 0.0f, 0.5f);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Render chat text
            std::string displayText = chatInput;
            if (static_cast<int>(glfwGetTime() * 2) % 2) { // Blink cursor every 0.5 seconds
                displayText += "_";
            }
            guiRenderer->renderText(guiShader->ID, displayText, -0.95f, -0.85f, 0.05f);

            // Restore OpenGL state
            if (depthTestWasEnabled) glEnable(GL_DEPTH_TEST);
            if (cullFaceEnabled) glEnable(GL_CULL_FACE);
        }

        // Render GUI elements (moved to here)
        guiRenderer->renderCrosshair(guiShader->ID);

        if (isPaused) {
            guiRenderer->renderPauseMenu(guiShader->ID);
        } else if (!isChatOpen) {
            guiRenderer->renderCrosshair(guiShader->ID);
        }

        // guiRenderer->renderText(guiShader->ID, R"(!"#$%&'()*+,-./))", -0.5f, 0.15f, 0.1f);
        // guiRenderer->renderText(guiShader->ID, R"(0123456789:;<=>?)", -0.5f, 0.0f, 0.1f);
        // guiRenderer->renderText(guiShader->ID, R"(@ABCDEFGHIJKLMNO)", -0.5f, -0.15f, 0.1f);
        // guiRenderer->renderText(guiShader->ID, R"(PQRSTUVWXYZ[|]^_)", -0.5f, -0.3f, 0.1f);
        // guiRenderer->renderText(guiShader->ID, R"(`abcdefghijklmno)", -0.5f, -0.45f, 0.1f);
        // guiRenderer->renderText(guiShader->ID, R"(pqrstuvwxyz{|}~)", -0.5f, -0.6f, 0.1f);

        frameCount++;
        float currentTime = static_cast<float>(glfwGetTime());
        if (currentTime - lastTime >= 1.0f) {
            fps = frameCount / (currentTime - lastTime) + 1;
            frameCount = 0;
            lastTime = currentTime;
        }

        std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
        guiRenderer->renderText(guiShader->ID, fpsText, -0.95f, 0.9f, 0.05f);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete guiShader;
    delete guiRenderer;

    glfwTerminate();
    return 0;
}
