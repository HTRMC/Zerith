#include "ThreadPool.hpp"
#include "Logger.hpp"

namespace Zerith {

ThreadPool::ThreadPool(size_t threadCount) {
    // Use hardware concurrency if threadCount is 0
    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
    }
    
    // Ensure at least one thread
    threadCount = std::max(size_t(1), threadCount);
    
    LOG_INFO("Creating thread pool with %zu threads", threadCount);
    
    // Start worker threads
    threads.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i) {
        threads.emplace_back([this, i] {
            LOG_DEBUG("Worker thread %zu started", i);
            
            // Worker thread loop
            while (true) {
                std::function<void()> task;
                
                // Get a task from the queue
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    
                    // Wait until there's a task or the pool is stopping
                    condition.wait(lock, [this] {
                        return stopping || !tasks.empty();
                    });
                    
                    // Exit if the pool is stopping and there are no more tasks
                    if (stopping && tasks.empty()) {
                        LOG_DEBUG("Worker thread %zu exiting", i);
                        return;
                    }
                    
                    // Get the next task
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                
                // Set active flag and increment remaining tasks
                activeThreads++;
                remainingTasks++;
                
                try {
                    // Execute the task
                    task();
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in thread pool task: %s", e.what());
                } catch (...) {
                    LOG_ERROR("Unknown exception in thread pool task");
                }
                
                // Reset active flag and decrement remaining tasks
                activeThreads--;
                remainingTasks--;
                
                // If there are no more active threads or remaining tasks, notify waiting threads
                if (remainingTasks == 0) {
                    std::unique_lock<std::mutex> lock(completionMutex);
                    completionCondition.notify_all();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    LOG_DEBUG("Shutting down thread pool");
    
    // Signal all threads to stop
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stopping = true;
    }
    
    // Wake up all threads so they can exit
    condition.notify_all();
    
    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    LOG_INFO("Thread pool shut down");
}

size_t ThreadPool::queueSize() const {
    std::unique_lock<std::mutex> lock(queueMutex);
    return tasks.size();
}

void ThreadPool::waitForCompletion() {
    // If there are no remaining tasks, return immediately
    if (remainingTasks == 0) {
        return;
    }
    
    // Wait for all tasks to complete
    std::unique_lock<std::mutex> lock(completionMutex);
    completionCondition.wait(lock, [this] {
        return remainingTasks == 0;
    });
}

} // namespace Zerith