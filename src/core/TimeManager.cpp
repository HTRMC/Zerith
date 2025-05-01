#include "TimeManager.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace Zerith {

TimeManager::TimeManager(int targetTickRate)
    : targetTickRate(targetTickRate),
      targetTickDuration(std::chrono::duration_cast<Duration>(std::chrono::seconds(1)) / targetTickRate),
      engineStartTime(Clock::now()),
      lastFrameTime(engineStartTime),
      accumulator(Duration::zero()),
      deltaTime(0.0f),
      lastFixedDeltaTime(0.0f),
      totalTime(0.0f),
      totalTicks(0),
      totalFrames(0) {
    
    // Reserve space for statistics
    frameTimes.resize(100, 0.0f);
    tickTimes.resize(100, 0.0f);
    
    LOG_INFO("TimeManager initialized with target tick rate: %d ticks/second", targetTickRate);
}

void TimeManager::update() {
    // Calculate time since last frame
    TimePoint currentTime = Clock::now();
    Duration frameTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;
    
    // Convert to seconds for game use
    deltaTime = durationToSeconds(frameTime);
    
    // Clamp deltaTime to avoid spiral of death when debugging
    // or when the application is suspended
    const float maxDeltaTime = 0.25f; // Max 250ms (4fps)
    deltaTime = std::min(deltaTime, maxDeltaTime);
    
    // Update total time
    totalTime += deltaTime;
    
    // Increment frame counter
    totalFrames++;
    
    // Update the accumulator for fixed-rate updates
    accumulator += frameTime;
    
    // Execute game logic ticks if needed
    executeGameTicks();
    
    // Update statistics
    updateStats();
}

void TimeManager::executeGameTicks() {
    // Convert target tick duration to seconds for callback
    float fixedDeltaTime = durationToSeconds(targetTickDuration);
    lastFixedDeltaTime = fixedDeltaTime;
    
    // Execute as many fixed ticks as needed to catch up
    // This ensures simulation runs at the correct speed
    // regardless of frame rate
    while (accumulator >= targetTickDuration) {
        // Execute game logic with fixed timestep
        if (tickCallback) {
            // Measure tick execution time for stats
            auto tickStart = Clock::now();
            
            // Execute the tick
            tickCallback(fixedDeltaTime);
            
            // Store execution time
            auto tickDuration = durationToSeconds(Clock::now() - tickStart);
            tickTimes.pop_front();
            tickTimes.push_back(tickDuration);
        }
        
        // Decrement the accumulator
        accumulator -= targetTickDuration;
        
        // Increment tick counter
        totalTicks++;
    }
}

void TimeManager::updateStats() {
    // Update frame time history
    frameTimes.pop_front();
    frameTimes.push_back(deltaTime);
}

void TimeManager::setTickCallback(TickCallback callback) {
    tickCallback = callback;
}

float TimeManager::getDeltaTime() const {
    return deltaTime;
}

float TimeManager::getLastFrameTime() const {
    return deltaTime;
}

float TimeManager::getAverageDeltaTime() const {
    return std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
}

float TimeManager::getTotalElapsedTime() const {
    return totalTime;
}

double TimeManager::getCurrentTPS() const {
    if (tickTimes.back() > 0.0f) {
        return 1.0 / tickTimes.back();
    }
    return 0.0;
}

double TimeManager::getAverageTPS() const {
    // Calculate average tick execution time
    float avgTickTime = std::accumulate(tickTimes.begin(), tickTimes.end(), 0.0f) / tickTimes.size();
    
    // Avoid division by zero
    if (avgTickTime > 0.0f) {
        return 1.0 / avgTickTime;
    }
    return 0.0;
}

int TimeManager::getTargetTickRate() const {
    return targetTickRate;
}

void TimeManager::setTargetTickRate(int tickRate) {
    // Validate input
    if (tickRate <= 0) {
        LOG_ERROR("Invalid tick rate: %d. Must be positive.", tickRate);
        return;
    }
    
    targetTickRate = tickRate;
    targetTickDuration = std::chrono::duration_cast<Duration>(std::chrono::seconds(1)) / targetTickRate;
    
    LOG_INFO("Target tick rate set to: %d ticks/second", targetTickRate);
}

long long TimeManager::getTotalTicks() const {
    return totalTicks;
}

long long TimeManager::getTotalFrames() const {
    return totalFrames;
}

std::string TimeManager::getDebugInfo() const {
    std::stringstream ss;
    ss << "Time Stats: "
       << "FT: " << std::fixed << std::setprecision(2) << (deltaTime * 1000.0f) << "ms, "
       << "Avg FT: " << std::fixed << std::setprecision(2) << (getAverageDeltaTime() * 1000.0f) << "ms, "
       << "TPS: " << std::fixed << std::setprecision(1) << getCurrentTPS() << "/" << targetTickRate << ", "
       << "Frames: " << totalFrames << ", "
       << "Ticks: " << totalTicks;
    return ss.str();
}

void TimeManager::reset() {
    // Reset all time tracking variables
    engineStartTime = Clock::now();
    lastFrameTime = engineStartTime;
    accumulator = Duration::zero();
    deltaTime = 0.0f;
    totalTime = 0.0f;
    totalTicks = 0;
    totalFrames = 0;
    
    // Reset statistics
    std::fill(frameTimes.begin(), frameTimes.end(), 0.0f);
    std::fill(tickTimes.begin(), tickTimes.end(), 0.0f);
    
    LOG_INFO("TimeManager reset");
}

float TimeManager::durationToSeconds(Duration duration) const {
    return std::chrono::duration<float>(duration).count();
}

} // namespace Zerith