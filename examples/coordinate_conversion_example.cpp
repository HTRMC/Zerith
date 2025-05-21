#include "../include/coordinate_conversion.h"
#include "../include/face_instance.h"
#include <iostream>
#include <iomanip>

// Utility to print a vector
void printVec3(const std::string& label, const glm::vec3& vec) {
    std::cout << label << "(" 
              << std::fixed << std::setprecision(2) << vec.x << ", " 
              << std::fixed << std::setprecision(2) << vec.y << ", " 
              << std::fixed << std::setprecision(2) << vec.z << ")" << std::endl;
}

// Utility to print a quaternion in angle-axis format
void printQuat(const std::string& label, const glm::quat& q) {
    // Convert quaternion to axis-angle representation
    float angle = 2.0f * acos(q.w);
    float s = sqrt(1.0f - q.w * q.w);
    
    glm::vec3 axis;
    if (s < 0.001f) {
        // Avoid division by zero
        axis = glm::vec3(q.x, q.y, q.z);
    } else {
        axis = glm::vec3(q.x / s, q.y / s, q.z / s);
    }
    
    std::cout << label << "Angle: " 
              << std::fixed << std::setprecision(2) << glm::degrees(angle) 
              << "° around axis (" 
              << std::fixed << std::setprecision(2) << axis.x << ", " 
              << std::fixed << std::setprecision(2) << axis.y << ", " 
              << std::fixed << std::setprecision(2) << axis.z << ")" << std::endl;
}

int main() {
    // The original Blender cube face transforms
    struct BlenderFace {
        std::string name;
        glm::vec3 position;
        glm::vec3 rotation;
    };
    
    BlenderFace blenderFaces[6] = {
        {"Top    ", glm::vec3(0.5f, 0.5f, 1.0f), glm::vec3(0.0f, 0.0f, 90.0f)},
        {"Bottom ", glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(180.0f, 0.0f, 90.0f)},
        {"Front  ", glm::vec3(0.0f, 0.5f, 0.5f), glm::vec3(-90.0f, 180.0f, 90.0f)},
        {"Back   ", glm::vec3(1.0f, 0.5f, 0.5f), glm::vec3(-90.0f, 180.0f, -90.0f)},
        {"Left   ", glm::vec3(0.5f, 0.0f, 0.5f), glm::vec3(90.0f, 0.0f, 0.0f)},
        {"Right  ", glm::vec3(0.5f, 1.0f, 0.5f), glm::vec3(-90.0f, 180.0f, 0.0f)}
    };
    
    std::cout << "===== Coordinate System Conversion Example =====" << std::endl;
    std::cout << "Blender: Z-up, right-handed coordinate system" << std::endl;
    std::cout << "Vulkan:  Y-up, right-handed coordinate system" << std::endl;
    std::cout << std::endl;
    
    std::cout << "===== Converting Blender Coordinates to Vulkan =====" << std::endl;
    
    for (int i = 0; i < 6; i++) {
        auto& face = blenderFaces[i];
        std::cout << "--- " << face.name << " Face ---" << std::endl;
        
        // Original Blender coordinates
        printVec3("Blender Position: ", face.position);
        std::cout << "Blender Rotation: (" 
                  << std::fixed << std::setprecision(2) << face.rotation.x << ", " 
                  << std::fixed << std::setprecision(2) << face.rotation.y << ", " 
                  << std::fixed << std::setprecision(2) << face.rotation.z << ")" << std::endl;
        
        // Converted Vulkan coordinates
        glm::vec3 vulkanPos;
        glm::quat vulkanRot;
        CoordinateConversion::blenderToVulkanTransform(
            face.position, face.rotation, vulkanPos, vulkanRot);
        
        printVec3("Vulkan Position: ", vulkanPos);
        printQuat("Vulkan Rotation: ", vulkanRot);
        
        // Compare with precomputed values
        std::cout << "Precomputed: " << std::endl;
        printVec3("  Position: ", faceInstances[i].position);
        printQuat("  Rotation: ", faceInstances[i].rotation);
        
        std::cout << std::endl;
    }
    
    return 0;
}