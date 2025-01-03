// CubeGeometry.hpp
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
};

// Define the transforms for each face of the cube
const std::vector<glm::mat4> CUBE_FACE_TRANSFORMS = {
    // Front face (Z+)
    glm::translate(
        glm::rotate(
            glm::mat4(1.0f),                    // Start with identity matrix
            glm::radians(90.0f),          // Rotate 180 degrees (convert to radians)
            glm::vec3(0.0f, 1.0f, 0.0f)     // Rotate around Y axis
        ),
        glm::vec3(-1.0f, 0.0f, 0.0f)             // Then translate
    ),
    // Back face (Z-)
    glm::translate(
        glm::rotate(
            glm::mat4(1.0f),                        // Start with identity matrix
            glm::radians(-90.0f),               // Rotate 90 degrees
            glm::vec3(0.0f, 1.0f, 0.0f)         // Rotate around Y axis
        ),
        glm::vec3(0.0f, 0.0f, -1.0f)             // Then translate
    ),
    // Right face (X+)
    glm::translate(
        glm::rotate(
            glm::mat4(1.0f),                        // Start with identity matrix
            glm::radians(-90.0f),               // Rotate 90 degrees
            glm::vec3(1.0f, 0.0f, 0.0f)         // Rotate around Y axis
        ),
        glm::vec3(0.0f, -1.0f, 0.0f)             // Then translate
    ),
    // Left face (X-)
    glm::translate(
        glm::rotate(
            glm::mat4(1.0f),                        // Start with identity matrix
            glm::radians(90.0f),               // Rotate 90 degrees
            glm::vec3(1.0f, 0.0f, 0.0f)         // Rotate around Y axis
        ),
        glm::vec3(0.0f, 0.0f, -1.0f)             // Then translate
    ),
    // Top face (Y+)
    glm::rotate(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 1.0f)),  // First translate
        glm::radians(180.0f),                                           // Then rotate 180 degrees
        glm::vec3(1.0f, 0.0f, 0.0f)                                    // Around X axis
    ),
    // Bottom face (Y-)
    glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))
};

class CubeGeometry {
public:
    static std::vector<Vertex> getPlaneVertices() {
        return {
                {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // Bottom-left
                {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // Bottom-right
                {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // Top-right
                {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}   // Top-left
        };
    }

    static std::vector<uint32_t> getPlaneIndices() {
        return {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
        };
    }
};