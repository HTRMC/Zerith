#pragma once

#include <vector>
#include <memory>
#include <mutex>

template<typename T>
class ObjectPool {
public:
    class PooledObject {
    public:
        explicit PooledObject(ObjectPool<T>* pool, T* obj) : m_pool(pool), m_object(obj) {}
        
        ~PooledObject() {
            if (m_pool && m_object) {
                m_pool->returnObject(m_object);
            }
        }
        
        // Move constructor
        PooledObject(PooledObject&& other) noexcept : m_pool(other.m_pool), m_object(other.m_object) {
            other.m_pool = nullptr;
            other.m_object = nullptr;
        }
        
        // Move assignment
        PooledObject& operator=(PooledObject&& other) noexcept {
            if (this != &other) {
                if (m_pool && m_object) {
                    m_pool->returnObject(m_object);
                }
                m_pool = other.m_pool;
                m_object = other.m_object;
                other.m_pool = nullptr;
                other.m_object = nullptr;
            }
            return *this;
        }
        
        // Delete copy operations
        PooledObject(const PooledObject&) = delete;
        PooledObject& operator=(const PooledObject&) = delete;
        
        T* get() { return m_object; }
        const T* get() const { return m_object; }
        T& operator*() { return *m_object; }
        const T& operator*() const { return *m_object; }
        T* operator->() { return m_object; }
        const T* operator->() const { return m_object; }
        
        void reset() {
            if (m_pool && m_object) {
                m_pool->returnObject(m_object);
                m_pool = nullptr;
                m_object = nullptr;
            }
        }
        
    private:
        ObjectPool<T>* m_pool;
        T* m_object;
    };
    
    explicit ObjectPool(size_t initialSize = 32) {
        m_available.reserve(initialSize);
        for (size_t i = 0; i < initialSize; ++i) {
            m_available.emplace_back(std::make_unique<T>());
        }
    }
    
    ~ObjectPool() = default;
    
    PooledObject acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_available.empty()) {
            // Pool is empty, create a new object
            auto newObj = std::make_unique<T>();
            T* rawPtr = newObj.release();
            return PooledObject(this, rawPtr);
        }
        
        auto obj = std::move(m_available.back());
        m_available.pop_back();
        T* rawPtr = obj.release();
        return PooledObject(this, rawPtr);
    }
    
    void returnObject(T* obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        m_available.emplace_back(obj);
    }
    
    size_t availableCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_available.size();
    }
    
private:
    std::vector<std::unique_ptr<T>> m_available;
    mutable std::mutex m_mutex;
};