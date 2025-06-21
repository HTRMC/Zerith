#include "imgui_integration.h"
#include "logger.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <numeric>

bool ImGuiIntegration::initialize(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice,
                                VkDevice device, uint32_t queueFamily, VkQueue queue, VkRenderPass renderPass,
                                uint32_t minImageCount, uint32_t imageCount) {
    
    if (m_initialized) {
        LOG_WARN("ImGui already initialized");
        return true;
    }
    
    m_device = device;
    m_renderPass = renderPass;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    if (!ImGui_ImplGlfw_InitForVulkan(window, true)) {
        LOG_ERROR("Failed to initialize ImGui GLFW implementation");
        return false;
    }
    
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        LOG_ERROR("Failed to create ImGui descriptor pool");
        return false;
    }
    
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = physicalDevice;
    initInfo.Device = device;
    initInfo.QueueFamily = queueFamily;
    initInfo.Queue = queue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_descriptorPool;
    initInfo.RenderPass = renderPass;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = minImageCount;
    initInfo.ImageCount = imageCount;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = nullptr;
    
    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        LOG_ERROR("Failed to initialize ImGui Vulkan implementation");
        return false;
    }
    
    m_metrics.lastUpdateTime = std::chrono::high_resolution_clock::now();
    m_metrics.frameTimeHistory.reserve(m_metrics.frameTimeHistorySize);
    
    m_initialized = true;
    LOG_INFO("ImGui initialized successfully");
    return true;
}

void ImGuiIntegration::cleanup() {
    if (!m_initialized) return;
    
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
    LOG_INFO("ImGui cleaned up");
}

void ImGuiIntegration::newFrame() {
    if (!m_initialized) return;
    
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiIntegration::render(VkCommandBuffer commandBuffer) {
    if (!m_initialized) return;
    
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void ImGuiIntegration::updatePerformanceMetrics(float deltaTime) {
    if (!m_initialized) return;
    
    m_metrics.frameTime = deltaTime * 1000.0f;
    m_metrics.fps = 1.0f / deltaTime;
    
    m_metrics.frameTimeHistory.push_back(m_metrics.frameTime);
    if (m_metrics.frameTimeHistory.size() > m_metrics.frameTimeHistorySize) {
        m_metrics.frameTimeHistory.erase(m_metrics.frameTimeHistory.begin());
    }
    
    if (!m_metrics.frameTimeHistory.empty()) {
        m_metrics.avgFrameTime = std::accumulate(m_metrics.frameTimeHistory.begin(), 
                                               m_metrics.frameTimeHistory.end(), 0.0f) / 
                               m_metrics.frameTimeHistory.size();
    }
}

void ImGuiIntegration::renderPerformanceWindow() {
    if (!m_initialized || !m_showPerformance) return;
    
    ImGui::Begin("Performance Metrics", &m_showPerformance);
    
    ImGui::Text("FPS: %.1f", m_metrics.fps);
    ImGui::Text("Frame Time: %.3f ms", m_metrics.frameTime);
    ImGui::Text("Avg Frame Time: %.3f ms", m_metrics.avgFrameTime);
    
    if (!m_metrics.frameTimeHistory.empty()) {
        ImGui::PlotLines("Frame Time (ms)", m_metrics.frameTimeHistory.data(), 
                        m_metrics.frameTimeHistory.size(), 0, nullptr, 0.0f, 50.0f, 
                        ImVec2(0, 80));
    }
    
    ImGui::Separator();
    ImGui::Text("Chunk Generation Time: %.3f ms", m_metrics.chunkGenTime);
    ImGui::Text("Mesh Generation Time: %.3f ms", m_metrics.meshGenTime);
    ImGui::Text("Chunks Loaded: %d", m_metrics.chunksLoaded);
    ImGui::Text("Meshes Generated: %d", m_metrics.meshesGenerated);
    
    ImGui::End();
}

void ImGuiIntegration::renderCameraWindow(const Zerith::Player& player) {
    if (!m_initialized || !m_showCamera) return;
    
    ImGui::Begin("Camera Info", &m_showCamera);
    
    ImGui::Text("Camera Information");
    ImGui::Text("Player data available");
    ImGui::Text("(Implementation requires Player header)");
    
    ImGui::End();
}

void ImGuiIntegration::renderChunkWindow(const Zerith::ChunkManager& chunkManager) {
    if (!m_initialized || !m_showChunks) return;
    
    ImGui::Begin("Chunk Info", &m_showChunks);
    
    ImGui::Text("Chunk Information");
    ImGui::Text("ChunkManager data available");
    ImGui::Text("(Implementation requires ChunkManager header)");
    
    ImGui::End();
}