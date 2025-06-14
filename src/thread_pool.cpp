#include "thread_pool.h"
#include "logger.h"
#include <algorithm>
#include <cassert>

namespace Zerith {

std::atomic<Task::TaskId> Task::s_nextId{1};
thread_local std::mt19937 ThreadPool::s_rng{std::random_device{}()};
std::unique_ptr<ThreadPool> g_threadPool;

ThreadPool::ThreadPool(size_t numThreads) {
    if (numThreads == 0) {
        numThreads = std::max(2u, std::thread::hardware_concurrency());
    }
    
    LOG_INFO("Initializing ThreadPool with %zu threads", numThreads);
    
    m_localQueues.reserve(numThreads);
    m_threads.reserve(numThreads);
    
    for (size_t i = 0; i < numThreads; ++i) {
        m_localQueues.emplace_back(std::make_unique<WorkStealingQueue>());
    }
    
    for (size_t i = 0; i < numThreads; ++i) {
        m_threads.emplace_back(&ThreadPool::workerThread, this, i);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(m_globalQueueMutex);
        m_shutdown.store(true, std::memory_order_release);
    }
    m_condition.notify_all();
    
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    LOG_INFO("ThreadPool shutdown complete. Stats: completed=%llu, stolen=%llu, cancelled=%llu",
             m_stats.tasksCompleted.load(), m_stats.tasksStolen.load(), m_stats.tasksCancelled.load());
}

void ThreadPool::submitTask(Task task) {
    if (m_shutdown.load(std::memory_order_acquire)) {
        return;
    }
    
    // Track task for potential cancellation
    {
        std::unique_lock<std::mutex> lock(m_taskMapMutex);
        m_taskCancellationFlags[task.getId()] = task.m_cancelled;
    }
    
    // For critical priority, try to find the least loaded local queue
    if (task.getPriority() == TaskPriority::Critical) {
        size_t minSize = std::numeric_limits<size_t>::max();
        size_t targetQueue = 0;
        
        for (size_t i = 0; i < m_localQueues.size(); ++i) {
            size_t size = m_localQueues[i]->size();
            if (size < minSize) {
                minSize = size;
                targetQueue = i;
            }
        }
        
        m_localQueues[targetQueue]->push(std::move(task));
        m_condition.notify_one();
        return;
    }
    
    // For other priorities, use the global queue
    {
        std::unique_lock<std::mutex> lock(m_globalQueueMutex);
        m_globalQueue.push(std::move(task));
    }
    m_condition.notify_one();
}

bool ThreadPool::cancelTask(Task::TaskId taskId) {
    std::unique_lock<std::mutex> lock(m_taskMapMutex);
    auto it = m_taskCancellationFlags.find(taskId);
    if (it != m_taskCancellationFlags.end()) {
        if (auto flag = it->second.lock()) {
            flag->store(true, std::memory_order_release);
            m_stats.tasksCancelled.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
    return false;
}

void ThreadPool::cancelTasksByPriority(TaskPriority maxPriority) {
    std::unique_lock<std::mutex> lock(m_globalQueueMutex);
    std::priority_queue<Task> newQueue;
    
    while (!m_globalQueue.empty()) {
        Task task = std::move(const_cast<Task&>(m_globalQueue.top()));
        m_globalQueue.pop();
        
        if (task.getPriority() <= maxPriority) {
            task.cancel();
            m_stats.tasksCancelled.fetch_add(1, std::memory_order_relaxed);
        } else {
            newQueue.push(std::move(task));
        }
    }
    
    m_globalQueue = std::move(newQueue);
}

size_t ThreadPool::getPendingTaskCount() const {
    size_t count = 0;
    
    // Count global queue
    {
        std::unique_lock<std::mutex> lock(m_globalQueueMutex);
        count += m_globalQueue.size();
    }
    
    // Count local queues
    for (const auto& queue : m_localQueues) {
        count += queue->size();
    }
    
    return count;
}

void ThreadPool::setThreadCount(size_t count) {
    if (count == m_threads.size()) {
        return;
    }
    
    LOG_INFO("Adjusting thread count from %zu to %zu", m_threads.size(), count);
    
    if (count < m_threads.size()) {
        // Need to reduce threads
        size_t toRemove = m_threads.size() - count;
        for (size_t i = 0; i < toRemove; ++i) {
            // Signal specific threads to exit (implementation would need per-thread shutdown flags)
            // For now, we'll just log a warning
            LOG_WARN("Thread reduction not fully implemented yet");
        }
    } else {
        // Add new threads
        size_t oldSize = m_threads.size();
        for (size_t i = oldSize; i < count; ++i) {
            m_localQueues.emplace_back(std::make_unique<WorkStealingQueue>());
            m_threads.emplace_back(&ThreadPool::workerThread, this, i);
        }
    }
}

void ThreadPool::workerThread(size_t threadIndex) {
    LOG_DEBUG("Worker thread %zu started", threadIndex);
    
    while (!m_shutdown.load(std::memory_order_acquire)) {
        Task task;
        auto waitStart = std::chrono::steady_clock::now();
        
        if (tryGetTask(task, threadIndex)) {
            auto waitEnd = std::chrono::steady_clock::now();
            auto waitTime = std::chrono::duration_cast<std::chrono::microseconds>(waitEnd - waitStart).count();
            
            m_stats.activeThreads.fetch_add(1, std::memory_order_relaxed);
            
            auto execStart = std::chrono::steady_clock::now();
            task.execute();
            auto execEnd = std::chrono::steady_clock::now();
            auto execTime = std::chrono::duration_cast<std::chrono::microseconds>(execEnd - execStart).count();
            
            m_stats.activeThreads.fetch_sub(1, std::memory_order_relaxed);
            
            updateStats(task, waitTime, execTime);
            
            // Clean up completed task from tracking map
            {
                std::unique_lock<std::mutex> lock(m_taskMapMutex);
                m_taskCancellationFlags.erase(task.getId());
            }
        } else {
            // Wait for new tasks
            std::unique_lock<std::mutex> lock(m_globalQueueMutex);
            m_condition.wait_for(lock, std::chrono::milliseconds(10), [this] {
                return !m_globalQueue.empty() || m_shutdown.load(std::memory_order_acquire);
            });
        }
    }
    
    LOG_DEBUG("Worker thread %zu exiting", threadIndex);
}

bool ThreadPool::tryGetTask(Task& task, size_t threadIndex) {
    // First, try local queue
    if (m_localQueues[threadIndex]->tryPop(task)) {
        return true;
    }
    
    // Then, try global queue
    {
        std::unique_lock<std::mutex> lock(m_globalQueueMutex);
        if (!m_globalQueue.empty()) {
            task = std::move(const_cast<Task&>(m_globalQueue.top()));
            m_globalQueue.pop();
            return true;
        }
    }
    
    // Finally, try work stealing if enabled
    if (m_workStealingEnabled.load(std::memory_order_acquire)) {
        return tryStealTask(task, threadIndex);
    }
    
    return false;
}

bool ThreadPool::tryStealTask(Task& task, size_t threadIndex) {
    const size_t numQueues = m_localQueues.size();
    if (numQueues <= 1) {
        return false;
    }
    
    // Random starting point for stealing
    size_t victimIndex = s_rng() % numQueues;
    
    for (size_t i = 0; i < numQueues - 1; ++i) {
        if (victimIndex == threadIndex) {
            victimIndex = (victimIndex + 1) % numQueues;
            continue;
        }
        
        if (m_localQueues[victimIndex]->trySteal(task)) {
            m_stats.tasksStolen.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        
        victimIndex = (victimIndex + 1) % numQueues;
    }
    
    return false;
}

void ThreadPool::updateStats(const Task& task, uint64_t waitTime, uint64_t executionTime) {
    m_stats.tasksCompleted.fetch_add(1, std::memory_order_relaxed);
    m_stats.totalWaitTime.fetch_add(waitTime, std::memory_order_relaxed);
    m_stats.totalExecutionTime.fetch_add(executionTime, std::memory_order_relaxed);
    
    if (!task.getName().empty()) {
        LOG_DEBUG("Task '%s' (priority=%d) completed: wait=%lluus, exec=%lluus",
                  task.getName().c_str(), static_cast<int>(task.getPriority()),
                  waitTime, executionTime);
    }
}

void initializeThreadPool(size_t numThreads) {
    if (g_threadPool) {
        LOG_WARN("Thread pool already initialized");
        return;
    }
    
    g_threadPool = std::make_unique<ThreadPool>(numThreads);
    LOG_INFO("Global thread pool initialized");
}

void shutdownThreadPool() {
    if (!g_threadPool) {
        LOG_WARN("Thread pool not initialized");
        return;
    }
    
    g_threadPool.reset();
    LOG_INFO("Global thread pool shut down");
}

} // namespace Zerith