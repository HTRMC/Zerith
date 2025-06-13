#include "async_gpu_uploader.h"
#include "logger.h"
#include <cstring>

namespace Zerith {

AsyncGPUUploader::AsyncGPUUploader(VkDevice device, VkPhysicalDevice physicalDevice, 
                                   VkCommandPool commandPool, VkQueue graphicsQueue)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_commandPool(commandPool)
    , m_graphicsQueue(graphicsQueue)
{
    // Start the upload thread
    m_uploadThread = std::thread(&AsyncGPUUploader::uploadThreadFunction, this);
    LOG_INFO("AsyncGPUUploader initialized with background thread");
}

AsyncGPUUploader::~AsyncGPUUploader() {
    shutdown();
}

void AsyncGPUUploader::queueBufferUpdate(std::vector<BlockbenchInstanceGenerator::FaceInstance> faceInstances,
                                        std::function<void()> completionCallback) {
    if (m_shutdown.load()) {
        LOG_WARN("Attempted to queue buffer update after shutdown");
        return;
    }
    
    // Add to upload queue
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_uploadQueue.emplace(std::move(faceInstances), std::move(completionCallback));
    }
    
    // Notify upload thread
    m_queueCondition.notify_one();
    
    LOG_DEBUG("Queued GPU buffer update with %zu face instances", faceInstances.size());
}

AsyncGPUUploader::BufferInfo AsyncGPUUploader::getCurrentBufferInfo() const {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    return m_currentBuffer;
}

void AsyncGPUUploader::waitForCompletion() {
    // Wait until upload queue is empty and no upload in progress
    while (true) {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_uploadQueue.empty() && !m_uploadInProgress.load()) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void AsyncGPUUploader::shutdown() {
    if (m_shutdown.exchange(true)) {
        return; // Already shutdown
    }
    
    LOG_INFO("Shutting down AsyncGPUUploader...");
    
    // Signal shutdown and wake up upload thread
    m_queueCondition.notify_all();
    
    // Wait for upload thread to finish
    if (m_uploadThread.joinable()) {
        m_uploadThread.join();
    }
    
    // Cleanup current buffer
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (m_currentBuffer.buffer != VK_NULL_HANDLE) {
            cleanupBuffer(m_currentBuffer.buffer, m_currentBuffer.memory);
            m_currentBuffer = {};
        }
    }
    
    // Cleanup any pending buffers
    {
        std::lock_guard<std::mutex> lock(m_cleanupMutex);
        for (auto& [buffer, memory] : m_buffersToCleanup) {
            cleanupBuffer(buffer, memory);
        }
        m_buffersToCleanup.clear();
    }
    
    LOG_INFO("AsyncGPUUploader shutdown complete");
}

void AsyncGPUUploader::uploadThreadFunction() {
    LOG_DEBUG("AsyncGPUUploader thread started");
    
    while (!m_shutdown.load()) {
        BufferUpdateRequest request({}, nullptr);
        
        // Wait for work
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCondition.wait(lock, [this] { 
                return !m_uploadQueue.empty() || m_shutdown.load(); 
            });
            
            if (m_shutdown.load()) {
                break;
            }
            
            if (m_uploadQueue.empty()) {
                continue;
            }
            
            request = std::move(m_uploadQueue.front());
            m_uploadQueue.pop();
        }
        
        // Mark upload in progress
        m_uploadInProgress.store(true);
        
        try {
            // Create new buffer
            createFaceInstanceBuffer(request.faceInstances);
            
            // Call completion callback if provided
            if (request.completionCallback) {
                request.completionCallback();
            }
            
            LOG_DEBUG("GPU buffer upload completed with %zu instances", request.faceInstances.size());
        }
        catch (const std::exception& e) {
            LOG_ERROR("GPU buffer upload failed: %s", e.what());
        }
        
        // Mark upload complete
        m_uploadInProgress.store(false);
    }
    
    LOG_DEBUG("AsyncGPUUploader thread finished");
}

void AsyncGPUUploader::createFaceInstanceBuffer(const std::vector<BlockbenchInstanceGenerator::FaceInstance>& instances) {
    if (instances.empty()) {
        LOG_DEBUG("No face instances to upload, creating empty buffer");
        
        // Update buffer info with empty buffer
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        // Queue old buffer for cleanup
        if (m_currentBuffer.buffer != VK_NULL_HANDLE) {
            std::lock_guard<std::mutex> cleanupLock(m_cleanupMutex);
            m_buffersToCleanup.emplace_back(m_currentBuffer.buffer, m_currentBuffer.memory);
        }
        
        m_currentBuffer = {};
        m_currentBuffer.isValid = true;
        return;
    }
    
    // Convert FaceInstance data to GPU-compatible format
    std::vector<FaceInstanceData> gpuInstances;
    gpuInstances.reserve(instances.size());
    
    for (const auto& face : instances) {
        FaceInstanceData gpuInstance{};
        gpuInstance.position = glm::vec4(face.position, 1.0f);
        gpuInstance.rotation = face.rotation;
        gpuInstance.scale = glm::vec4(face.scale, static_cast<float>(face.faceDirection));
        gpuInstance.uv = face.uv;
        gpuInstance.textureLayer = face.textureLayer;
        // padding is automatically zero-initialized
        
        gpuInstances.push_back(gpuInstance);
    }
    
    LOG_DEBUG("Converted %zu face instances to GPU format", gpuInstances.size());
    if (!gpuInstances.empty()) {
        const auto& first = gpuInstances[0];
        LOG_DEBUG("Sample face: pos(%.2f,%.2f,%.2f) scale(%.2f,%.2f,%.2f) texLayer:%u", 
                 first.position.x, first.position.y, first.position.z,
                 first.scale.x, first.scale.y, first.scale.z, first.textureLayer);
    }
    
    VkDeviceSize bufferSize = sizeof(FaceInstanceData) * gpuInstances.size();
    
    // Create buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer newBuffer;
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &newBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create face instance buffer");
    }
    
    // Get memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, newBuffer, &memRequirements);
    
    // Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, 
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VkDeviceMemory newMemory;
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &newMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, newBuffer, nullptr);
        throw std::runtime_error("Failed to allocate face instance buffer memory");
    }
    
    // Bind buffer to memory
    vkBindBufferMemory(m_device, newBuffer, newMemory, 0);
    
    // Map memory and copy data
    void* data;
    vkMapMemory(m_device, newMemory, 0, bufferSize, 0, &data);
    memcpy(data, gpuInstances.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device, newMemory);
    
    // Update buffer info atomically
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        
        // Queue old buffer for cleanup
        if (m_currentBuffer.buffer != VK_NULL_HANDLE) {
            std::lock_guard<std::mutex> cleanupLock(m_cleanupMutex);
            m_buffersToCleanup.emplace_back(m_currentBuffer.buffer, m_currentBuffer.memory);
        }
        
        // Set new buffer info
        m_currentBuffer.buffer = newBuffer;
        m_currentBuffer.memory = newMemory;
        m_currentBuffer.instanceCount = instances.size();
        m_currentBuffer.isValid = true;
    }
    
    // Cleanup old buffers periodically (to avoid holding too many)
    {
        std::lock_guard<std::mutex> cleanupLock(m_cleanupMutex);
        if (m_buffersToCleanup.size() > 3) { // Keep max 3 old buffers
            auto [oldBuffer, oldMemory] = m_buffersToCleanup.front();
            m_buffersToCleanup.erase(m_buffersToCleanup.begin());
            
            // Schedule cleanup for next frame (we're in background thread)
            vkDestroyBuffer(m_device, oldBuffer, nullptr);
            vkFreeMemory(m_device, oldMemory, nullptr);
        }
    }
}

void AsyncGPUUploader::cleanupBuffer(VkBuffer buffer, VkDeviceMemory memory) {
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, buffer, nullptr);
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, memory, nullptr);
    }
}

uint32_t AsyncGPUUploader::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type for face instance buffer");
}

} // namespace Zerith