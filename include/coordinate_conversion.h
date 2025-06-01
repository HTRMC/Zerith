#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace CoordinateConversion {

// Convert Blender position (Z-up) to Vulkan position (Y-up)
inline constexpr glm::vec3 blenderToVulkanPosition(const glm::vec3& blenderPos) {
    // Blender (X, Y, Z) -> Vulkan (X, Z, -Y)
    return glm::vec3(
        blenderPos.x,      // X stays the same
        blenderPos.z,      // Y in Vulkan = Z in Blender
        -blenderPos.y      // Z in Vulkan = -Y in Blender
    );
}

// Convert Blender Euler rotation (Z-up) to Vulkan quaternion (Y-up)
inline glm::quat blenderToVulkanRotation(const glm::vec3& blenderEulerDegrees) {
    // First convert Blender Euler angles (in degrees) to radians
    glm::vec3 blenderEulerRadians = glm::radians(blenderEulerDegrees);
    
    // Create quaternion from Blender Euler angles
    glm::quat blenderQuat = glm::quat(blenderEulerRadians);
    
    // Create the base rotation quaternion (-90 degrees around X-axis)
    glm::quat baseRotation = glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f));
    
    // Apply the base rotation, then the original rotation
    return baseRotation * blenderQuat;
}

// Convert a complete Blender transform (position and rotation) to Vulkan
inline void blenderToVulkanTransform(
    const glm::vec3& blenderPos,
    const glm::vec3& blenderRotDegrees,
    glm::vec3& vulkanPos,
    glm::quat& vulkanRot
) {
    vulkanPos = blenderToVulkanPosition(blenderPos);
    vulkanRot = blenderToVulkanRotation(blenderRotDegrees);
}

} // namespace CoordinateConversion