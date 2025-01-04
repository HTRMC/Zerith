#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "CubeGeometry.hpp"

class SubChunk {
public:
    static const int SIZE = 16;
    static const int SUBCHUNK = SIZE * SIZE * SIZE;

    SubChunk() {
        generateTransforms();
    }

    const std::vector<glm::mat4>& getTransforms() const {
        return transforms;
    }

private:
    std::vector<glm::mat4> transforms;

    void generateTransforms() {
        transforms.clear();
        transforms.reserve(SUBCHUNK * 6); // 6 faces per cube

        // Generate cube positions in a 16x16x16 grid
        for (int x = 0; x < SIZE; x++) {
            for (int y = 0; y < SIZE; y++) {
                for (int z = 0; z < SIZE; z++) {
                    glm::mat4 baseTransform = glm::translate(
                        glm::mat4(1.0f),
                        glm::vec3(static_cast<float>(x),
                                 static_cast<float>(y),
                                 static_cast<float>(z))
                    );

                    // Add transforms for each face of the cube
                    // These are based on the CUBE_FACE_TRANSFORMS from CubeGeometry
                    for (const auto& faceTransform : CUBE_FACE_TRANSFORMS) {
                        transforms.push_back(baseTransform * faceTransform);
                    }
                }
            }
        }
    }
};
