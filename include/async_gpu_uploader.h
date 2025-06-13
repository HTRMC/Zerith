#pragma once

#include "blockbench_instance_generator.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

namespace Zerith {

// GPU-compatible face instance data structure (matches shader layout)
struct FaceInstanceData {
    alignas(16) glm::vec4 position; // vec3 + padding
    alignas(16) glm::vec4 rotation; // quaternion
    alignas(16) glm::vec4 scale;    // face scale (width, height, 1.0, faceDirection)
    alignas(16) glm::vec4 uv;       // UV coordinates [minU, minV, maxU, maxV]
    uint32_t textureLayer;          // Texture array layer index
    uint32_t padding[3];            // Padding to maintain 16-byte alignment
};

// Request for GPU buffer update
struct BufferUpdateRequest {
    std::vector<BlockbenchInstanceGenerator::FaceInstance> faceInstances;
    std::function<void()> completionCallback;
    
    BufferUpdateRequest(std::vector<BlockbenchInstanceGenerator::FaceInstance> faces, 
                       std::function<void()> callback = nullptr)
        : faceInstances(std::move(faces)), completionCallback(std::move(callback)) {}
};

// Async GPU buffer uploader for non-blocking GPU operations
class AsyncGPUUploader {
public:
    AsyncGPUUploader(VkDevice device, VkPhysicalDevice physicalDevice, 
                     VkCommandPool commandPool, VkQueue graphicsQueue);
    ~AsyncGPUUploader();
    
    // Queue a buffer update request (non-blocking)
    void queueBufferUpdate(std::vector<BlockbenchInstanceGenerator::FaceInstance> faceInstances,
                          std::function<void()> completionCallback = nullptr);
    
    // Get the current GPU buffer info (thread-safe)
    struct BufferInfo {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        size_t instanceCount = 0;
        bool isValid = false;
    };
    BufferInfo getCurrentBufferInfo() const;
    
    // Check if upload is in progress
    bool isUploadInProgress() const { return m_uploadInProgress.load(); }
    
    // Wait for all pending uploads to complete
    void waitForCompletion();
    
    // Shutdown the uploader
    void shutdown();

private:
    // Background upload thread function
    void uploadThreadFunction();
    
    // Create a new face instance buffer
    void createFaceInstanceBuffer(const std::vector<BlockbenchInstanceGenerator::FaceInstance>& instances);
    
    // Cleanup old buffer
    void cleanupBuffer(VkBuffer buffer, VkDeviceMemory memory);
    
    // Find memory type for buffer allocation
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
    // Vulkan objects
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_graphicsQueue;
    
    // Current buffer info
    mutable std::mutex m_bufferMutex;
    BufferInfo m_currentBuffer;
    
    // Upload queue and threading
    std::thread m_uploadThread;
    std::queue<BufferUpdateRequest> m_uploadQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::atomic<bool> m_shutdown{false};
    std::atomic<bool> m_uploadInProgress{false};
    
    // Old buffers pending cleanup
    std::vector<std::pair<VkBuffer, VkDeviceMemory>> m_buffersToCleanup;
    std::mutex m_cleanupMutex;
};

} // namespace Zerith