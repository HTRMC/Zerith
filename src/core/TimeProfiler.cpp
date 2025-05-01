#include "TimeProfiler.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace Zerith {

TimeProfiler& TimeProfiler::getInstance() {
    static TimeProfiler instance;
    return instance;
}

TimeProfiler::TimeProfiler() {
    LOG_INFO("TimeProfiler initialized");
}

void TimeProfiler::beginSection(const std::string& name) {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    // Get or create section stats
    auto& section = sections[name];
    
    // Make sure the section isn't already active
    if (section.isActive) {
        LOG_WARN("Attempting to begin already active profile section: %s", name.c_str());
        return;
    }
    
    // Mark section as active and store start time
    section.isActive = true;
    section.startTime = Clock::now();
}

void TimeProfiler::endSection(const std::string& name) {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    // Make sure the section exists and is active
    auto it = sections.find(name);
    if (it == sections.end() || !it->second.isActive) {
        LOG_WARN("Attempting to end inactive profile section: %s", name.c_str());
        return;
    }
    
    // Get section stats
    auto& section = it->second;
    
    // Calculate time taken
    TimePoint endTime = Clock::now();
    double timeMs = durationToMs(endTime - section.startTime);
    
    // Update min/max times
    if (section.sampleCount == 0 || timeMs < section.minTime) {
        section.minTime = timeMs;
    }
    if (section.sampleCount == 0 || timeMs > section.maxTime) {
        section.maxTime = timeMs;
    }
    
    // Add to running total
    section.totalTime += timeMs;
    section.sampleCount++;
    
    // Add to sample history (maintaining fixed size)
    if (section.samples.size() >= section.historySize) {
        section.samples.pop_front();
    }
    section.samples.push_back(timeMs);
    
    // Mark section as inactive
    section.isActive = false;
}

double TimeProfiler::getAverageTime(const std::string& name) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = sections.find(name);
    if (it == sections.end() || it->second.sampleCount == 0) {
        return 0.0;
    }
    
    const auto& section = it->second;
    
    // Return average over recent samples if available
    if (!section.samples.empty()) {
        return std::accumulate(section.samples.begin(), section.samples.end(), 0.0) / section.samples.size();
    }
    
    // Fallback to overall average
    return section.totalTime / section.sampleCount;
}

double TimeProfiler::getMinTime(const std::string& name) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = sections.find(name);
    if (it == sections.end() || it->second.sampleCount == 0) {
        return 0.0;
    }
    
    return it->second.minTime;
}

double TimeProfiler::getMaxTime(const std::string& name) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = sections.find(name);
    if (it == sections.end() || it->second.sampleCount == 0) {
        return 0.0;
    }
    
    return it->second.maxTime;
}

double TimeProfiler::getLastTime(const std::string& name) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = sections.find(name);
    if (it == sections.end() || it->second.samples.empty()) {
        return 0.0;
    }
    
    return it->second.samples.back();
}

long TimeProfiler::getSampleCount(const std::string& name) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = sections.find(name);
    if (it == sections.end()) {
        return 0;
    }
    
    return it->second.sampleCount;
}

void TimeProfiler::reset() {
    std::lock_guard<std::mutex> lock(profileMutex);
    sections.clear();
    LOG_INFO("TimeProfiler reset");
}

std::string TimeProfiler::getReport(bool detailed) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    std::stringstream ss;
    ss << "Performance Profile Report:" << std::endl;
    
    if (sections.empty()) {
        ss << "  No profiling data available." << std::endl;
        return ss.str();
    }
    
    // Create a sortable list of sections
    std::vector<std::pair<std::string, SectionStats>> sortedSections;
    for (const auto& [name, stats] : sections) {
        sortedSections.emplace_back(name, stats);
    }
    
    // Sort by average time (descending)
    std::sort(sortedSections.begin(), sortedSections.end(), 
              [this](const auto& a, const auto& b) {
                  return getAverageTime(a.first) > getAverageTime(b.first);
              });
    
    // Format each section
    for (const auto& [name, stats] : sortedSections) {
        if (stats.sampleCount == 0) {
            continue; // Skip sections with no samples
        }
        
        double avgTime = getAverageTime(name);
        
        ss << "  " << std::left << std::setw(25) << name << " | ";
        ss << "Avg: " << std::fixed << std::setprecision(3) << std::setw(7) << avgTime << "ms | ";
        ss << "Min: " << std::fixed << std::setprecision(3) << std::setw(7) << stats.minTime << "ms | ";
        ss << "Max: " << std::fixed << std::setprecision(3) << std::setw(7) << stats.maxTime << "ms | ";
        ss << "Samples: " << stats.sampleCount;
        ss << std::endl;
        
        // Add histogram for detailed view
        if (detailed && !stats.samples.empty()) {
            // Find range for histogram
            double min = *std::min_element(stats.samples.begin(), stats.samples.end());
            double max = *std::max_element(stats.samples.begin(), stats.samples.end());
            
            // Create histogram with 10 buckets
            const int bucketCount = 10;
            std::vector<int> histogram(bucketCount, 0);
            
            double bucketSize = (max - min) / bucketCount;
            if (bucketSize <= 0.0) {
                bucketSize = 1.0; // Handle case where all samples are equal
            }
            
            // Count samples in each bucket
            for (double sample : stats.samples) {
                int bucket = static_cast<int>((sample - min) / bucketSize);
                if (bucket >= bucketCount) {
                    bucket = bucketCount - 1;
                }
                histogram[bucket]++;
            }
            
            // Draw histogram
            const int maxBarWidth = 50;
            int maxCount = *std::max_element(histogram.begin(), histogram.end());
            
            ss << "    Distribution:" << std::endl;
            for (int i = 0; i < bucketCount; i++) {
                double bucketMin = min + i * bucketSize;
                double bucketMax = bucketMin + bucketSize;
                
                ss << "    " << std::fixed << std::setprecision(2) << std::setw(6) << bucketMin
                   << " - " << std::setw(6) << bucketMax << " ms | ";
                
                // Calculate bar width proportional to count
                int barWidth = 0;
                if (maxCount > 0) {
                    barWidth = static_cast<int>(static_cast<double>(histogram[i]) / maxCount * maxBarWidth);
                }
                
                // Draw the bar
                ss << std::string(barWidth, '#') << std::endl;
            }
        }
    }
    
    return ss.str();
}

double TimeProfiler::durationToMs(Duration duration) const {
    return std::chrono::duration<double, std::milli>(duration).count();
}

// ScopedProfiler implementation
ScopedProfiler::ScopedProfiler(const std::string& sectionName)
    : sectionName(sectionName) {
    TimeProfiler::getInstance().beginSection(sectionName);
}

ScopedProfiler::~ScopedProfiler() {
    TimeProfiler::getInstance().endSection(sectionName);
}

} // namespace Zerith