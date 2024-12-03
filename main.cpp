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

// Camera movement enum
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Camera class
class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;

    void updateCameraVectors() {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(direction);
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }

public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f))
        : position(position)
        , front(glm::vec3(0.0f, 0.0f, -1.0f))
        , worldUp(glm::vec3(0.0f, 1.0f, 0.0f))
        , yaw(-90.0f)
        , pitch(0.0f)
        , movementSpeed(2.5f)
        , mouseSensitivity(0.1f)
    {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    void processKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        if (direction == FORWARD)
            position += front * velocity;
        if (direction == BACKWARD)
            position -= front * velocity;
        if (direction == LEFT)
            position -= right * velocity;
        if (direction == RIGHT)
            position += right * velocity;
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        updateCameraVectors();
    }
};

// Shader class to handle compilation and usage
class Shader {
public:
    unsigned int ID;

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

// Modify your existing processInput function
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
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

    // Create and compile shaders
    Shader shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    // Vertex data for cube
    float vertices[] = {
        // positions          // colors
        // Front face (z = 1)
        0.0f, 0.0f, 1.0f,    1.0f, 0.0f, 1.0f,  // 0,0,1
        1.0f, 0.0f, 1.0f,    1.0f, 0.0f, 1.0f,  // 1,0,1
        1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f,  // 1,1,1
        1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f,  // 1,1,1
        0.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f,  // 0,1,1
        0.0f, 0.0f, 1.0f,    1.0f, 0.0f, 1.0f,  // 0,0,1

        // Back face (z = 0)
        0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,  // 0,0,0
        1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,  // 1,0,0
        1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f,  // 1,1,0
        1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f,  // 1,1,0
        0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f,  // 0,1,0
        0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,  // 0,0,0

        // Left face (x = 0)
        0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f,  // 0,1,1
        0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,  // 0,1,0
        0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 1.0f,  // 0,0,0
        0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 1.0f,  // 0,0,0
        0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,  // 0,0,1
        0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f,  // 0,1,1

        // Right face (x = 1)
        1.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f,  // 1,1,1
        1.0f, 1.0f, 0.0f,    0.0f, 0.0f, 1.0f,  // 1,1,0
        1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,  // 1,0,0
        1.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,  // 1,0,0
        1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,  // 1,0,1
        1.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f,  // 1,1,1

        // Bottom face (y = 0)
        0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,  // 0,0,0
        1.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,  // 1,0,0
        1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,  // 1,0,1
        1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,  // 1,0,1
        0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,  // 0,0,1
        0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,  // 0,0,0

        // Top face (y = 1)
        0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,  // 0,1,0
        1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,  // 1,1,0
        1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 0.0f,  // 1,1,1
        1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 0.0f,  // 1,1,1
        0.0f, 1.0f, 1.0f,    1.0f, 0.0f, 0.0f,  // 0,1,1
        0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f   // 0,1,0
    };

    // Create buffers/arrays
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Move camera back to see the full grid
    camera.position = glm::vec3(8.0f, 8.0f, 20.0f);

    // Store transformations for each cube
    std::vector<glm::mat4> modelMatrices;
    for(int x = 0; x < 16; x++) {
        for(int z = 0; z < 16; z++) {
            glm::mat4 model = glm::mat4(1.0f);
            // Position each cube with small gaps between them
            model = glm::translate(model, glm::vec3(
                x * 1.1f,  // Add 0.1 spacing between cubes
                0.0f,      // All cubes at same height
                z * 1.1f   // Add 0.1 spacing between cubes
            ));
            modelMatrices.push_back(model);
        }
    }

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Create transformations
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        view = camera.getViewMatrix();
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // Draw all cubes
        glBindVertexArray(VAO);
        for(const auto& model : modelMatrices) {
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}