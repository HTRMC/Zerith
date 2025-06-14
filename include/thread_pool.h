#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <chrono>
#include <optional>
#include <random>

namespace Zerith {

enum class TaskPriority : uint8_t {
    Critical = 0,  // Immediate execution needed (e.g., chunks at player position)
    High = 1,      // High priority (e.g., chunks in view frustum)
    Normal = 2,    // Normal priority (e.g., chunks at medium distance)
    Low = 3,       // Low priority (e.g., distant chunks, cleanup tasks)
    Idle = 4       // Only run when nothing else to do
};

class Task {
public:
    using TaskFunc = std::function<void()>;
    using TaskId = uint64_t;
    
    Task() = default;
    Task(TaskFunc func, TaskPriority priority = TaskPriority::Normal, std::string name = "")
        : m_function(std::move(func))
        , m_priority(priority)
        , m_name(std::move(name))
        , m_id(s_nextId.fetch_add(1, std::memory_order_relaxed))
        , m_cancelled(std::make_shared<std::atomic<bool>>(false))
        , m_timestamp(std::chrono::steady_clock::now()) {}
    
    void execute() {
        if (!m_cancelled->load(std::memory_order_acquire) && m_function) {
            m_function();
        }
    }
    
    void cancel() {
        m_cancelled->store(true, std::memory_order_release);
    }
    
    bool isCancelled() const {
        return m_cancelled->load(std::memory_order_acquire);
    }
    
    TaskPriority getPriority() const { return m_priority; }
    TaskId getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    auto getTimestamp() const { return m_timestamp; }
    
    bool operator<(const Task& other) const {
        if (m_priority != other.m_priority) {
            return m_priority > other.m_priority;  // Higher priority value = lower priority
        }
        return m_timestamp > other.m_timestamp;  // Older tasks first for same priority
    }

    std::shared_ptr<std::atomic<bool>> m_cancelled;
    
private:
    TaskFunc m_function;
    TaskPriority m_priority;
    std::string m_name;
    TaskId m_id;
    std::chrono::steady_clock::time_point m_timestamp;
    static std::atomic<TaskId> s_nextId;
};

class WorkStealingQueue {
public:
    WorkStealingQueue() = default;
    
    void push(Task task) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push_back(std::move(task));
    }
    
    bool tryPop(Task& task) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        task = std::move(m_queue.back());
        m_queue.pop_back();
        return true;
    }
    
    bool trySteal(Task& task) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        task = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }
    
    bool empty() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    
    size_t size() const {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    
private:
    mutable std::mutex m_mutex;
    std::deque<Task> m_queue;
};

class ThreadPool {
public:
    struct Stats {
        std::atomic<uint64_t> tasksCompleted{0};
        std::atomic<uint64_t> tasksStolen{0};
        std::atomic<uint64_t> tasksCancelled{0};
        std::atomic<uint64_t> totalWaitTime{0};  // in microseconds
        std::atomic<uint64_t> totalExecutionTime{0};  // in microseconds
        std::atomic<uint32_t> activeThreads{0};
        
        // Copy constructor for Stats
        Stats() = default;
        Stats(const Stats& other) 
            : tasksCompleted(other.tasksCompleted.load())
            , tasksStolen(other.tasksStolen.load())
            , tasksCancelled(other.tasksCancelled.load())
            , totalWaitTime(other.totalWaitTime.load())
            , totalExecutionTime(other.totalExecutionTime.load())
            , activeThreads(other.activeThreads.load()) {}
    };
    
    explicit ThreadPool(size_t numThreads = 0);
    ~ThreadPool();
    
    // Submit a task and get a future for its result
    template<typename F>
    auto submitTask(F&& f, TaskPriority priority = TaskPriority::Normal, 
                    const std::string& name = "") -> std::future<decltype(f())> {
        using ReturnType = decltype(f());
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::forward<F>(f)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        Task wrappedTask([task]() { (*task)(); }, priority, name);
        submitTask(std::move(wrappedTask));
        
        return result;
    }
    
    // Submit a task without a future
    void submitTask(Task task);
    
    // Cancel a task by ID
    bool cancelTask(Task::TaskId taskId);
    
    // Cancel all tasks with a given priority or lower
    void cancelTasksByPriority(TaskPriority maxPriority);
    
    // Get current statistics
    Stats getStats() const { return m_stats; }
    
    // Get number of pending tasks
    size_t getPendingTaskCount() const;
    
    // Dynamically adjust thread count based on load
    void setThreadCount(size_t count);
    
    // Get current thread count
    size_t getThreadCount() const { return m_threads.size(); }
    
    // Enable/disable work stealing
    void setWorkStealingEnabled(bool enabled) { 
        m_workStealingEnabled.store(enabled, std::memory_order_release); 
    }
    
private:
    void workerThread(size_t threadIndex);
    bool tryGetTask(Task& task, size_t threadIndex);
    bool tryStealTask(Task& task, size_t threadIndex);
    void updateStats(const Task& task, uint64_t waitTime, uint64_t executionTime);
    
    std::vector<std::thread> m_threads;
    std::vector<std::unique_ptr<WorkStealingQueue>> m_localQueues;
    
    // Global priority queue for tasks
    std::priority_queue<Task> m_globalQueue;
    mutable std::mutex m_globalQueueMutex;
    std::condition_variable m_condition;
    
    // Task tracking
    std::unordered_map<Task::TaskId, std::weak_ptr<std::atomic<bool>>> m_taskCancellationFlags;
    mutable std::mutex m_taskMapMutex;
    
    // Control flags
    std::atomic<bool> m_shutdown{false};
    std::atomic<bool> m_workStealingEnabled{true};
    
    // Statistics
    mutable Stats m_stats;
    
    // Thread-local random for work stealing
    static thread_local std::mt19937 s_rng;
};

// Global thread pool instance
extern std::unique_ptr<ThreadPool> g_threadPool;

// Initialize the global thread pool
void initializeThreadPool(size_t numThreads = 0);

// Shutdown the global thread pool
void shutdownThreadPool();

} // namespace Zerith