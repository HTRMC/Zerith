#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <string>

namespace Zerith {

/**
 * @brief Time management system for the game engine
 * 
 * Handles fixed tick rates, delta time calculation, and TPS monitoring.
 * Provides a stable game loop timing mechanism independent of frame rate.
 */
class TimeManager {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;
    using TickCallback = std::function<void(float)>;

    // Initialization
    TimeManager(int targetTickRate = 20);
    ~TimeManager() = default;
    
    // Delete copy and move constructors/assignments
    TimeManager(const TimeManager&) = delete;
    TimeManager& operator=(const TimeManager&) = delete;
    TimeManager(TimeManager&&) = delete;
    TimeManager& operator=(TimeManager&&) = delete;

    // Core time update function - called every frame
    void update();
    
    // Set callback function for game logic ticks
    void setTickCallback(TickCallback callback);
    
    // Time getters
    float getDeltaTime() const;
    float getLastFrameTime() const;
    float getAverageDeltaTime() const;
    float getTotalElapsedTime() const;
    
    // Frame/tick statistics
    double getCurrentTPS() const;
    double getAverageTPS() const;
    int getTargetTickRate() const;
    void setTargetTickRate(int tickRate);
    long long getTotalTicks() const;
    long long getTotalFrames() const;

    // Debug/profiling
    std::string getDebugInfo() const;
    
    // Reset all time tracking (for new game sessions)
    void reset();

private:
    // Target timing
    int targetTickRate;              // Target game logic updates per second
    Duration targetTickDuration;     // Duration of a single tick in nanoseconds
    
    // Time tracking
    TimePoint engineStartTime;       // When the engine started
    TimePoint lastFrameTime;         // Last frame timestamp
    Duration accumulator;            // Time accumulator for fixed timestep
    float deltaTime;                 // Time between frames in seconds
    float lastFixedDeltaTime;        // Last fixed time step in seconds
    float totalTime;                 // Total elapsed time since start
    
    // Statistics tracking
    long long totalTicks;            // Total number of ticks executed
    long long totalFrames;           // Total number of frames rendered
    std::deque<float> frameTimes;    // Recent frame times for averaging
    std::deque<float> tickTimes;     // Recent tick execution times
    
    // Tick callback
    TickCallback tickCallback;
    
    // Update tick accumulator and execute necessary ticks
    void executeGameTicks();
    
    // Update timing statistics
    void updateStats();
    
    // Convert Duration to seconds as float
    float durationToSeconds(Duration duration) const;
};

} // namespace Zerith