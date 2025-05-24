#pragma once

#include "blockbench_model.h"
#include "blockbench_instance_generator.h"
#include <vector>
#include <glm/glm.hpp>

// Wrapper class to generate instances at specific positions
class BlockbenchInstanceWrapper {
public:
    explicit BlockbenchInstanceWrapper(BlockbenchModel::Model model) 
        : m_model(std::move(model)) {}
    
    // Generate face instances at a specific world position
    std::vector<BlockbenchInstanceGenerator::FaceInstance> generateInstancesAtPosition(const glm::vec3& position) {
        // Generate base instances
        auto instances = BlockbenchInstanceGenerator::Generator::generateModelInstances(m_model);
        
        // Offset all faces by the given position
        for (auto& face : instances.faces) {
            face.position += position;
        }
        
        return instances.faces;
    }
    
private:
    BlockbenchModel::Model m_model;
};