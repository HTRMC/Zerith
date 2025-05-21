#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <array>

// FaceInstance struct for cube faces
struct FaceInstance {
    glm::vec3 position;
    glm::quat rotation;
};

// Create an array of face instances with Blender coordinates converted to Vulkan
const std::array<FaceInstance, 6> faceInstances = {
    // Top face: Blender Pos(0.5, 0.5, 1), Rot(0, 0, 90) -> Vulkan Pos(0.5, 1, 0.5), Rot around X(-90) * Rot(0, 0, 90)
    FaceInstance{ 
        glm::vec3(0.5f, 1.0f, 0.5f), 
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)) * glm::quat(glm::vec3(0.0f, 0.0f, glm::radians(90.0f)))
    },
    
    // Bottom face: Blender Pos(0.5, 0.5, 0), Rot(180, 0, 90) -> Vulkan Pos(0.5, 0, 0.5), Rot around X(-90) * Rot(180, 0, 90)
    FaceInstance{
        glm::vec3(0.5f, 0.0f, 0.5f),
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)) * glm::quat(glm::vec3(glm::radians(180.0f), 0.0f, glm::radians(90.0f)))
    },
    
    // Front face: Blender Pos(0, 0.5, 0.5), Rot(-90, 180, 90) -> Vulkan Pos(0, 0.5, 0.5), Rot around X(-90) * Rot(-90, 180, 90)
    FaceInstance{
        glm::vec3(0.0f, 0.5f, 0.5f),
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)) * glm::quat(glm::vec3(glm::radians(-90.0f), glm::radians(180.0f), glm::radians(90.0f)))
    },
    
    // Back face: Blender Pos(1, 0.5, 0.5), Rot(-90, 180, -90) -> Vulkan Pos(1, 0.5, 0.5), Rot around X(-90) * Rot(-90, 180, -90)
    FaceInstance{
        glm::vec3(1.0f, 0.5f, 0.5f),
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)) * glm::quat(glm::vec3(glm::radians(-90.0f), glm::radians(180.0f), glm::radians(-90.0f)))
    },
    
    // Left face: Blender Pos(0.5, 0, 0.5), Rot(90, 0, 0) -> Vulkan Pos(0.5, 0.5, 0), Rot around X(-90) * Rot(90, 0, 0)
    FaceInstance{
        glm::vec3(0.5f, 0.5f, 0.0f),
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)) * glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f))
    },
    
    // Right face: Blender Pos(0.5, 1, 0.5), Rot(-90, 180, 0) -> Vulkan Pos(0.5, 0.5, 1), Rot around X(-90) * Rot(-90, 180, 0)
    FaceInstance{
        glm::vec3(0.5f, 0.5f, 1.0f),
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)) * glm::quat(glm::vec3(glm::radians(-90.0f), glm::radians(180.0f), 0.0f))
    }
};

/*
Coordinate System Conversion Details:

Blender (Z-up, right-handed):
- Z is up
- X is right
- Y is forward

Vulkan (Y-up, right-handed):
- Y is up (Blender's Z)
- X is right (Blender's X)
- Z is forward (negative of Blender's Y)

The conversion requires:
1. Swapping Y and Z axes
2. Negating the new Z (old Y)
3. Applying a -90° rotation around X-axis to all transforms

Position conversion:
- Vulkan X = Blender X
- Vulkan Y = Blender Z
- Vulkan Z = -Blender Y

Rotation conversion:
- Apply a base rotation of -90° around X-axis
- Then apply the original Blender rotation

This system properly converts between the coordinate systems while maintaining
the correct face orientations for the cube.
*/