#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <atomic>

namespace Zerith {

class ThreadPool {
public:
    // Constructor: initializes the thread pool with a specific number of threads
    // If threadCount is 0, uses hardware concurrency count
    explicit ThreadPool(size_t threadCount = 0);
    
    // Destructor: joins all threads
    ~ThreadPool();
    
    // Disable copying and moving
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Enqueue a task and get a future for its result
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using ReturnType = typename std::invoke_result<F, Args...>::type;

        // Create a packaged task that will execute the function with its arguments
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        // Get the future result before pushing the task
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            
            // Check if the pool is being stopped
            if (stopping) {
                throw std::runtime_error("Cannot enqueue task on a stopping thread pool");
            }
            
            // Add the task to the queue
            tasks.emplace([task]() { (*task)(); });
        }
        
        // Notify one waiting thread that there's a new task
        condition.notify_one();
        
        return result;
    }

    // Get the number of active threads
    size_t size() const { return threads.size(); }
    
    // Get the number of tasks in the queue
    size_t queueSize() const;
    
    // Wait for all tasks to complete
    void waitForCompletion();
    
private:
    // Worker threads
    std::vector<std::thread> threads;
    
    // Task queue
    std::queue<std::function<void()>> tasks;
    
    // Synchronization primitives
    mutable std::mutex queueMutex;
    std::condition_variable condition;
    
    // Atomic flags for thread pool state
    std::atomic<bool> stopping{false};
    std::atomic<size_t> activeThreads{0};
    std::atomic<size_t> remainingTasks{0};
    
    // Completion synchronization
    std::mutex completionMutex;
    std::condition_variable completionCondition;
};

} // namespace Zerith