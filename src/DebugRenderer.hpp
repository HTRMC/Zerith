// DebugRenderer.hpp
#pragma once
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "AABB.hpp"

struct DebugVertex {
    glm::vec3 pos;
    glm::vec4 color;
};

struct DebugLine {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec4 color;
    float duration;
    float startTime;
};

struct DebugBox {
    AABB aabb;
    glm::vec4 color;
    float duration;
    float startTime;
};

class DebugRenderer {
public:
    DebugRenderer(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkCommandPool commandPool, VkRenderPass renderPass,
                  uint32_t windowWidth, uint32_t windowHeight);
    ~DebugRenderer();

    // Drawing functions
    void drawLine(const glm::vec3& start, const glm::vec3& end,
                 const glm::vec4& color = glm::vec4(1.0f), float duration = 0.0f);
    void drawBox(const AABB& box, const glm::vec4& color = glm::vec4(1.0f),
                float duration = 0.0f);
    // void drawSphere(const glm::vec3& center, float radius,
    //                const glm::vec4& color = glm::vec4(1.0f), float duration = 0.0f);
    // void drawCone(const glm::vec3& apex, const glm::vec3& direction, float height,
    //              float radius, const glm::vec4& color = glm::vec4(1.0f),
    //              float duration = 0.0f);

    // Update and render
    void update(float deltaTime);
    void render(VkCommandBuffer commandBuffer, const glm::mat4& viewProj);

    void clearBoxes() {
        boxes.clear();
        needsUpdate = true;
    }

private:
    void createVertexBuffer();
    void createPipeline();
    void updateBuffers();
    std::vector<DebugVertex> generateSphereVertices(const glm::vec3& center,
                                                   float radius,
                                                   const glm::vec4& color);
    std::vector<DebugVertex> generateConeVertices(const glm::vec3& apex,
                                                 const glm::vec3& direction,
                                                 float height, float radius,
                                                 const glm::vec4& color);
    std::vector<DebugVertex> generateBoxVertices(const AABB& box,
                                                const glm::vec4& color);

    VkShaderModule createShaderModule(const std::vector<char>& code);

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkRenderPass renderPass;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<DebugLine> lines;
    std::vector<DebugBox> boxes;
    std::vector<DebugVertex> vertices;

    uint32_t width;
    uint32_t height;

    const size_t MAX_BOXES = 16384;

    bool needsUpdate;
};
