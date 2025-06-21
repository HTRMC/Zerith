#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <chrono>

namespace Zerith {
    class Player;
    class ChunkManager;
}

class ImGuiIntegration {
public:
    struct PerformanceMetrics {
        float fps = 0.0f;
        float frameTime = 0.0f;
        float avgFrameTime = 0.0f;
        
        float chunkGenTime = 0.0f;
        float meshGenTime = 0.0f;
        int chunksLoaded = 0;
        int meshesGenerated = 0;
        
        std::chrono::high_resolution_clock::time_point lastUpdateTime;
        std::vector<float> frameTimeHistory;
        size_t frameTimeHistorySize = 60;
    };

    bool initialize(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice, 
                   VkDevice device, uint32_t queueFamily, VkQueue queue, VkRenderPass renderPass, 
                   uint32_t minImageCount, uint32_t imageCount);
    
    void cleanup();
    void newFrame();
    void render(VkCommandBuffer commandBuffer);
    
    void updatePerformanceMetrics(float deltaTime);
    void updateChunkGenTime(float time) { m_metrics.chunkGenTime = time; }
    void updateMeshGenTime(float time) { m_metrics.meshGenTime = time; }
    void incrementChunksLoaded() { m_metrics.chunksLoaded++; }
    void incrementMeshesGenerated() { m_metrics.meshesGenerated++; }
    
    void renderPerformanceWindow();
    void renderCameraWindow(const Zerith::Player& player);
    void renderChunkWindow(const Zerith::ChunkManager& chunkManager);
    
    void setShowPerformance(bool show) { m_showPerformance = show; }
    void setShowCamera(bool show) { m_showCamera = show; }
    void setShowChunks(bool show) { m_showChunks = show; }
    
    bool getShowPerformance() const { return m_showPerformance; }
    bool getShowCamera() const { return m_showCamera; }
    bool getShowChunks() const { return m_showChunks; }

private:
    bool m_initialized = false;
    bool m_showPerformance = true;
    bool m_showCamera = true;
    bool m_showChunks = true;
    
    PerformanceMetrics m_metrics;
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
};