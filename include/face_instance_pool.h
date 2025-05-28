#pragma once

#include "blockbench_instance_generator.h"
#include "object_pool.h"
#include <vector>
#include <memory>

class FaceInstancePool {
public:
    using FaceInstance = BlockbenchInstanceGenerator::FaceInstance;
    
    // A batch of face instances that automatically returns to pool when destroyed
    class FaceInstanceBatch {
    public:
        explicit FaceInstanceBatch(FaceInstancePool* pool) : m_pool(pool) {
            m_instances.reserve(64); // Reserve space for typical chunk face count
        }
        
        ~FaceInstanceBatch() {
            if (m_pool) {
                m_pool->returnBatch(std::move(m_instances));
            }
        }
        
        // Move constructor
        FaceInstanceBatch(FaceInstanceBatch&& other) noexcept 
            : m_pool(other.m_pool), m_instances(std::move(other.m_instances)) {
            other.m_pool = nullptr;
        }
        
        // Move assignment
        FaceInstanceBatch& operator=(FaceInstanceBatch&& other) noexcept {
            if (this != &other) {
                if (m_pool) {
                    m_pool->returnBatch(std::move(m_instances));
                }
                m_pool = other.m_pool;
                m_instances = std::move(other.m_instances);
                other.m_pool = nullptr;
            }
            return *this;
        }
        
        // Delete copy operations
        FaceInstanceBatch(const FaceInstanceBatch&) = delete;
        FaceInstanceBatch& operator=(const FaceInstanceBatch&) = delete;
        
        // Add a face instance to the batch
        void addFace(const glm::vec3& position, const glm::vec4& rotation, 
                     const glm::vec3& scale, int faceDirection, const glm::vec4& uv, 
                     uint32_t textureLayer, const std::string& textureName) {
            m_instances.emplace_back(position, rotation, scale, faceDirection, uv, textureLayer, textureName);
        }
        
        // Get read-only access to instances
        const std::vector<FaceInstance>& getInstances() const {
            return m_instances;
        }
        
        // Get mutable access to instances
        std::vector<FaceInstance>& getInstances() {
            return m_instances;
        }
        
        size_t size() const {
            return m_instances.size();
        }
        
        void clear() {
            m_instances.clear();
        }
        
        void reserve(size_t capacity) {
            m_instances.reserve(capacity);
        }
        
    private:
        friend class FaceInstancePool;
        FaceInstancePool* m_pool;
        std::vector<FaceInstance> m_instances;
    };
    
    explicit FaceInstancePool(size_t initialBatchCount = 8) {
        // Pre-allocate some batches
        for (size_t i = 0; i < initialBatchCount; ++i) {
            m_availableVectors.emplace_back();
            m_availableVectors.back().reserve(64);
        }
    }
    
    // Acquire a batch for filling with face instances
    FaceInstanceBatch acquireBatch() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        FaceInstanceBatch batch(this);
        
        if (!m_availableVectors.empty()) {
            batch.m_instances = std::move(m_availableVectors.back());
            m_availableVectors.pop_back();
            batch.m_instances.clear(); // Clear contents but keep capacity
        }
        
        return batch;
    }
    
    // Return a batch to the pool (called automatically by destructor)
    void returnBatch(std::vector<FaceInstance>&& instances) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        instances.clear(); // Clear contents but keep capacity
        m_availableVectors.emplace_back(std::move(instances));
        
        // Limit pool size to prevent unlimited growth
        if (m_availableVectors.size() > 32) {
            m_availableVectors.resize(16);
        }
    }
    
    size_t availableBatchCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_availableVectors.size();
    }
    
private:
    std::vector<std::vector<FaceInstance>> m_availableVectors;
    mutable std::mutex m_mutex;
};