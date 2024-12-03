#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
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

// Camera movement enum
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

struct Block {
    bool exists;
    glm::vec3 color;
};

std::vector<std::vector<std::vector<Block>>> blocks(16, std::vector<std::vector<Block>>(16, std::vector<Block>(16, {true, glm::vec3(1.0f, 1.0f, 1.0f)})));

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

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
    const float TICK_RATE = 20.0f;  // Minecraft runs at 20 ticks per second
    const float BASE_ACCELERATION = 0.049f;  // Matches Minecraft's flying acceleration
    const float AIR_FRICTION = 0.91f / 2;       // Matches Minecraft's air resistance
    const float MAX_SPEED = 10.79f;         // Matches Minecraft's max flying speed

    float mouseSensitivity;

    enum class MovementMode {
        WALKING,
        FLYING
    };

    MovementMode currentMode = MovementMode::WALKING;
    bool isGrounded = false;
    float lastJumpTime = 0.0f;
    const float DOUBLE_JUMP_TIME = 0.3f; // Time window for double jump in seconds
    const float GRAVITY = -20.0f;
    float verticalVelocity = 0.0f;
    bool canFly = true; // Set to false for survival mode
    const float EYE_HEIGHT = 1.6f;  // Eyes are slightly below total height
    const float JUMP_VELOCITY = 8.0f;

    void applyGravity(float deltaTime) {
        if (currentMode == MovementMode::WALKING && !isGrounded) {
            verticalVelocity += GRAVITY * deltaTime;
            position.y += verticalVelocity * deltaTime;

            // Check if we hit the ground (y = 0)
            if (position.y <= 1.0f) { // 1.0f because player height is 1.8
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

    const float PLAYER_WIDTH = 0.6f;  // Minecraft player is ~0.6 blocks wide
    const float PLAYER_HEIGHT = 1.8f; // Minecraft player is ~1.8 blocks tall

    AABB getBoundingBox() const {
        glm::vec3 halfExtents(PLAYER_WIDTH / 2.0f, PLAYER_HEIGHT / 2.0f, PLAYER_WIDTH / 2.0f);
        return AABB(position - halfExtents, position + halfExtents);
    }

    bool checkCollision(const glm::vec3& newPosition) {
        glm::vec3 playerPos = newPosition - glm::vec3(0.0f, EYE_HEIGHT, 0.0f);
        glm::vec3 halfExtents(PLAYER_WIDTH / 2.0f, PLAYER_HEIGHT / 2.0f, PLAYER_WIDTH / 2.0f);
        AABB playerBox(playerPos - halfExtents, playerPos + halfExtents);

        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 16; y++) {
                for (int z = 0; z < 16; z++) {
                    if (blocks[x][y][z].exists) {
                        AABB blockBox(
                            glm::vec3(x, y, z),
                            glm::vec3(x + 1.0f, y + 1.0f, z + 1.0f)
                        );

                        if (playerBox.intersects(blockBox)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f))
        : position(position)
        , front(glm::vec3(0.0f, 0.0f, -1.0f))
        , worldUp(glm::vec3(0.0f, 1.0f, 0.0f))
        , velocity(glm::vec3(0.0f))
        , yaw(-90.0f)
        , pitch(0.0f)
        , mouseSensitivity(0.1f)
    {
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

        void update(const glm::vec3& moveDir, float deltaTime) {
        // Convert real time to minecraft ticks
        float ticks = deltaTime * TICK_RATE;

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

        // Apply movement mode specific behavior
        if (currentMode == MovementMode::WALKING) {
            velocity.y = 0; // Don't accumulate Y velocity in walking mode

            // Apply gravity
            if (!isGrounded) {
                verticalVelocity += GRAVITY * deltaTime;
            }
            newPosition.y += verticalVelocity * deltaTime;
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
                // We hit something below us
                isGrounded = true;
                verticalVelocity = 0;
            } else {
                // We hit something above us
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

        // Check if we're grounded by doing a small check below us
        if (currentMode == MovementMode::WALKING && !isGrounded) {
            glm::vec3 groundCheck = position - glm::vec3(0.0f, 0.1f, 0.0f);
            if (checkCollision(groundCheck)) {
                isGrounded = true;
                verticalVelocity = 0;
            }
        }
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
};

// Shader class to handle compilation and usage
class Shader {
public:
    unsigned int ID;

    void setVec3(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    Shader(const char* vertexPath, const char* fragmentPath) {
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

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Add these global variables at the top of your file
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::ivec3 highlightedBlock(-1);
float lastSpacePressTime = 0.0f; // Time of the last space key press
const float DOUBLE_TAP_THRESHOLD = 0.3f; // Maximum time interval for double-tap in seconds

// Ray casting function to detect block intersection
bool raycastBlock(const glm::vec3& start, const glm::vec3& direction, float maxDistance,
                 glm::ivec3& outBlockPos, glm::vec3& outHitPos) {
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

        // Check if we're within the grid bounds
        if (gridX >= 0 && gridX < 16 &&
            gridY >= 0 && gridY < 16 &&
            gridZ >= 0 && gridZ < 16) {
            if (blocks[gridX][gridY][gridZ].exists) {
                // Define block boundaries
                glm::vec3 blockMin(gridX, gridY, gridZ);
                glm::vec3 blockMax = blockMin + glm::vec3(1.0f);

                // Check if the ray position is within the block's bounds
                if (rayPos.x >= blockMin.x && rayPos.x <= blockMax.x &&
                    rayPos.y >= blockMin.y && rayPos.y <= blockMax.y &&
                    rayPos.z >= blockMin.z && rayPos.z <= blockMax.z) {
                    outBlockPos = glm::ivec3(gridX, gridY, gridZ);
                    outHitPos = rayPos;
                    return true;
                    }
            }
            }
    }

    outBlockPos = glm::ivec3(-1);
    outHitPos = glm::vec3(0.0f);
    return false;
}

// Add these callback functions after your existing framebuffer_size_callback
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        glm::ivec3 blockPos;
        glm::vec3 hitPos;

        if (raycastBlock(camera.position, camera.front, 5.0f, blockPos, hitPos)) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                // Break block
                blocks[blockPos.x][blockPos.y][blockPos.z].exists = false;
            }
            else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                // Calculate adjacent block position based on hit normal
                glm::vec3 blockCenter = glm::vec3(blockPos) + glm::vec3(0.5f);
                glm::vec3 normal = glm::normalize(hitPos - blockCenter);

                int newX = blockPos.x + (normal.x > 0.5f ? 1 : (normal.x < -0.5f ? -1 : 0));
                int newY = blockPos.y + (normal.y > 0.5f ? 1 : (normal.y < -0.5f ? -1 : 0));
                int newZ = blockPos.z + (normal.z > 0.5f ? 1 : (normal.z < -0.5f ? -1 : 0));

                if (newX >= 0 && newX < 16 &&
                    newY >= 0 && newY < 16 &&
                    newZ >= 0 && newZ < 16) {
                    blocks[newX][newY][newZ].exists = true;
                    blocks[newX][newY][newZ].color = glm::vec3(0.8f, 0.4f, 0.2f);
                }
            }
        }
    }
}

// Modify your existing processInput function
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

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
            camera.jump(currentFrame);
        }

        // Detect double-tap for flying toggle
        if (currentFrame - lastSpacePressTime < DOUBLE_TAP_THRESHOLD) {
            if (camera.currentMode == Camera::MovementMode::FLYING) {
                camera.currentMode = Camera::MovementMode::WALKING;
            } else {
                camera.currentMode = Camera::MovementMode::FLYING;
            }
        }
        lastSpacePressTime = currentFrame;
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

    // Update camera movement
    camera.update(moveDir, deltaTime);
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

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
    }
    else {
        std::cout << "Failed to load texture at path: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

std::string loadFileContent(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Cube", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Create and compile shaders
    Shader shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    // Load a block model
    std::string jsonContent = loadFileContent("assets/minecraft/models/block/stairs.json"); // You'll need to implement this
    BlockModel model = BlockModel::loadFromJson(jsonContent);

    // Generate vertex data
    std::vector<float> vertices = model.generateVertexData();

    // Create buffers/arrays
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);                   // position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float))); // color
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float))); // texture
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(8 * sizeof(float))); // face index

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    // Move camera back to see the full grid
    camera.position = glm::vec3(8.0f, 20.0f, 8.0f);

    unsigned int texture = loadTexture("cobblestone.png");
    shader.use();
    shader.setInt("blockTexture", 0);
    shader.setBool("useTexture", true);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.4157f, 0.9961f, 0.9961f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Create transformations
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        view = camera.getViewMatrix();
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 1000.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // Add this before your cube drawing loop
        glm::ivec3 blockPos;
        glm::vec3 hitPos;

        glm::vec3 rayDirection = glm::normalize(camera.front);
        bool lookingAtBlock = raycastBlock(camera.position, rayDirection, 5.0f, blockPos, hitPos);

        highlightedBlock = lookingAtBlock ? blockPos : glm::ivec3(-1);

        // Then in your cube drawing loop, update it to:
        for(int x = 0; x < 16; x++) {
            for(int y = 0; y < 16; y++) {
                for(int z = 0; z < 16; z++) {
                    if (blocks[x][y][z].exists) {
                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, glm::vec3(x, y, z));
                        shader.setMat4("model", model);
                        shader.setVec3("blockColor", blocks[x][y][z].color);

                        bool isHighlighted = lookingAtBlock &&
                            (highlightedBlock.x == x &&
                             highlightedBlock.y == y &&
                             highlightedBlock.z == z);
                        shader.setBool("isHighlighted", isHighlighted);

                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture);

    glfwTerminate();
    return 0;
}