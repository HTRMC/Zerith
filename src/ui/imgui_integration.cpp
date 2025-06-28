#include "imgui_integration.h"
#include "logger.h"
#include "player.h"
#include "chunk_manager.h"
#include "voxel_ao.h"

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
    m_metrics.frameTimeHistory.resize(100, 0.0f); // Initialize with 100 elements
    m_metrics.chunkGenTimeHistory.reserve(m_metrics.generationTimeHistorySize);
    m_metrics.meshGenTimeHistory.reserve(m_metrics.generationTimeHistorySize);
    
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

void ImGuiIntegration::renderDebugWindow(const Zerith::Player* player, const Zerith::ChunkManager* chunkManager) {
    if (!m_initialized) return;
    
    ImGui::Begin("Debug Info");
    
    // Performance section
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
        
        // Add frame time to history for graph
        static int frame_count = 0;
        frame_count++;
        if (frame_count % 10 == 0) { // Update every 10 frames to reduce noise
            float currentFrameTime = 1000.0f / ImGui::GetIO().Framerate;
            m_metrics.frameTimeHistory[m_metrics.frameTimeIndex] = currentFrameTime;
            m_metrics.frameTimeIndex = (m_metrics.frameTimeIndex + 1) % 100;
        }
        
        if (m_metrics.frameTimeHistory.size() > 0) {
            ImGui::PlotLines("Frame Time (ms)", m_metrics.frameTimeHistory.data(), 
                            m_metrics.frameTimeHistory.size(), 0, nullptr, 0.0f, 50.0f, 
                            ImVec2(0, 80));
        }
        
        ImGui::Separator();
        ImGui::Checkbox("Pause Updates", &m_pauseMetricsUpdate);
        
        ImGui::Text("Chunk Generation Time: %.3f ms", m_metrics.chunkGenTime);
        ImGui::Text("Mesh Generation Time: %.3f ms", m_metrics.meshGenTime);
        
        if (!m_metrics.chunkGenTimeHistory.empty() && !m_metrics.meshGenTimeHistory.empty()) {
            ImGui::Text("Generation Times History:");
            
            // Custom function to format values with units
            auto PlotHistogramWithUnits = [](const char* label, const float* values, int values_count, const char* overlay_text = nullptr) {
                ImGui::PushID(label);
                ImGui::PlotHistogram("", values, values_count, 0, overlay_text, 0.0f, FLT_MAX, ImVec2(0, 80));
                
                // Check if hovering over the plot
                if (ImGui::IsItemHovered()) {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    ImVec2 plot_pos = ImGui::GetItemRectMin();
                    ImVec2 plot_size = ImGui::GetItemRectSize();
                    
                    // Calculate which bar we're hovering over
                    float rel_x = (mouse_pos.x - plot_pos.x) / plot_size.x;
                    int bar_idx = (int)(rel_x * values_count);
                    
                    if (bar_idx >= 0 && bar_idx < values_count) {
                        ImGui::SetTooltip("%.2f ms", values[bar_idx]);
                    }
                }
                ImGui::PopID();
            };
            
            // Plot chunk generation times
            ImGui::Text("Chunk Generation Time (ms):");
            PlotHistogramWithUnits("##ChunkGenHist", m_metrics.chunkGenTimeHistory.data(), 
                                 m_metrics.chunkGenTimeHistory.size(), nullptr);
            
            // Plot mesh generation times
            ImGui::Text("Mesh Generation Time (ms):");
            PlotHistogramWithUnits("##MeshGenHist", m_metrics.meshGenTimeHistory.data(), 
                                 m_metrics.meshGenTimeHistory.size(), nullptr);
        }
    }
    
    // Camera section
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (player) {
            const auto& pos = player->getPosition();
            const auto& rot = player->getRotation();
            
            // Convert Y rotation to compass direction
            auto getCompassDirection = [](float yRotationRad) -> const char* {
                // Convert radians to degrees
                float degrees = glm::degrees(yRotationRad);
                // Normalize rotation to 0-360 degrees
                float normalizedY = fmod(degrees + 360.0f, 360.0f);
                
                if (normalizedY >= 315.0f || normalizedY < 45.0f) return "North";
                else if (normalizedY >= 45.0f && normalizedY < 135.0f) return "East";
                else if (normalizedY >= 135.0f && normalizedY < 225.0f) return "South";
                else return "West";
            };
            
            ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
            ImGui::Text("Rotation: %.1f, %.1f, %.1f rad", rot.x, rot.y, rot.z);
            ImGui::Text("Facing: %s (%.1f°)", getCompassDirection(rot.y), glm::degrees(rot.y));
            ImGui::Text("Flying: %s", player->getIsFlying() ? "Yes" : "No");
            ImGui::Text("On Ground: %s", player->isOnGround() ? "Yes" : "No");
        } else {
            ImGui::Text("Player data not available");
        }
    }
    
    // Chunk section
    if (ImGui::CollapsingHeader("Chunks", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (chunkManager) {
            ImGui::Text("Render Distance: %d", chunkManager->getRenderDistance());
            ImGui::Text("Loaded Chunks: %zu", chunkManager->getLoadedChunkCount());
            ImGui::Text("Face Instances: %zu", chunkManager->getAllFaceInstances().size());
        } else {
            ImGui::Text("ChunkManager data not available");
        }
    }
    
    // Ambient Occlusion Debug section
    if (ImGui::CollapsingHeader("Ambient Occlusion Debug")) {
        static bool debugMode = false;
        static float aoValues[4] = {1.0f, 0.8f, 0.6f, 0.4f}; // TL, BL, TR, BR
        static float aoMultiplier = 1.0f;
        
        ImGui::Checkbox("Enable AO Debug Mode", &debugMode);
        Zerith::VoxelAO::setDebugMode(debugMode);
        
        if (debugMode) {
            ImGui::Text("Manual AO Values (TL, BL, TR, BR):");
            ImGui::SliderFloat("Top-Left", &aoValues[0], 0.0f, 1.0f);
            ImGui::SliderFloat("Bottom-Left", &aoValues[1], 0.0f, 1.0f);
            ImGui::SliderFloat("Top-Right", &aoValues[2], 0.0f, 1.0f);
            ImGui::SliderFloat("Bottom-Right", &aoValues[3], 0.0f, 1.0f);
            
            Zerith::VoxelAO::setDebugAOValues(aoValues[0], aoValues[1], aoValues[2], aoValues[3]);
            
            if (ImGui::Button("Reset to Test Pattern")) {
                aoValues[0] = 1.0f; // TL - White
                aoValues[1] = 0.7f; // BL - Light gray
                aoValues[2] = 0.4f; // TR - Dark gray  
                aoValues[3] = 0.1f; // BR - Almost black
            }
            
            ImGui::Text("This creates a gradient pattern:");
            ImGui::Text("  TL: %.2f (lightest)", aoValues[0]);
            ImGui::Text("  BL: %.2f", aoValues[1]);
            ImGui::Text("  TR: %.2f", aoValues[2]);
            ImGui::Text("  BR: %.2f (darkest)", aoValues[3]);
        }
        
        ImGui::Separator();
        ImGui::SliderFloat("AO Strength Multiplier", &aoMultiplier, 0.0f, 2.0f);
        Zerith::VoxelAO::setAOStrengthMultiplier(aoMultiplier);
        
        ImGui::Text("Use debug mode to:");
        ImGui::BulletText("Test vertex mapping with fixed patterns");
        ImGui::BulletText("Identify which corner is which");
        ImGui::BulletText("Adjust AO strength globally");
    }
    
    ImGui::End();
}