#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <deque>
#include <mutex>

namespace Zerith {

/**
 * @brief System for profiling execution time of different parts of the game loop
 * 
 * Provides tools for measuring and tracking performance statistics.
 * Can be used to identify bottlenecks and performance issues.
 */
class TimeProfiler {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    // Singleton instance
    static TimeProfiler& getInstance();

    // Delete copy and move constructors/assignments
    TimeProfiler(const TimeProfiler&) = delete;
    TimeProfiler& operator=(const TimeProfiler&) = delete;
    TimeProfiler(TimeProfiler&&) = delete;
    TimeProfiler& operator=(TimeProfiler&&) = delete;

    // Start measuring a section
    void beginSection(const std::string& name);
    
    // End measuring a section and record the time
    void endSection(const std::string& name);
    
    // Get stats for a specific section
    double getAverageTime(const std::string& name) const;
    double getMinTime(const std::string& name) const;
    double getMaxTime(const std::string& name) const;
    double getLastTime(const std::string& name) const;
    long getSampleCount(const std::string& name) const;
    
    // Reset all profiling data
    void reset();
    
    // Get formatted profile report
    std::string getReport(bool detailed = false) const;

private:
    // Private constructor for singleton
    TimeProfiler();
    
    // Section statistics data
    struct SectionStats {
        std::deque<double> samples;    // Recent sample times (ms)
        TimePoint startTime;           // Current section start time if active
        bool isActive = false;         // Whether section is currently being timed
        double minTime = 0.0;          // Minimum time recorded (ms)
        double maxTime = 0.0;          // Maximum time recorded (ms)
        double totalTime = 0.0;        // Total time across all samples (ms)
        long sampleCount = 0;          // Number of samples collected
        size_t historySize = 100;      // Number of samples to keep in history
    };
    
    // Profile data
    std::unordered_map<std::string, SectionStats> sections;
    
    // Thread safety
    mutable std::mutex profileMutex;
    
    // Convert Duration to milliseconds as double
    double durationToMs(Duration duration) const;
};

// Scope-based profiler that automatically times the current scope
class ScopedProfiler {
public:
    explicit ScopedProfiler(const std::string& sectionName);
    ~ScopedProfiler();

private:
    std::string sectionName;
};

// Macros for easy profiling
#ifdef ZERITH_PROFILE_ENABLED
    #define PROFILE_SCOPE(name) Zerith::ScopedProfiler scopedProfiler##__LINE__(name)
    #define PROFILE_FUNCTION() Zerith::ScopedProfiler scopedProfiler##__LINE__(__FUNCTION__)
    #define PROFILE_BEGIN(name) Zerith::TimeProfiler::getInstance().beginSection(name)
    #define PROFILE_END(name) Zerith::TimeProfiler::getInstance().endSection(name)
#else
    #define PROFILE_SCOPE(name)
    #define PROFILE_FUNCTION()
    #define PROFILE_BEGIN(name)
    #define PROFILE_END(name)
#endif

} // namespace Zerith