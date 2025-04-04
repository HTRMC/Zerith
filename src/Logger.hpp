// Logger.hpp
#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <vector>
#include <memory>
#include <iomanip>
#include <functional>
#include "core/windows_wrapper.hpp"

namespace Zerith {

// Log levels in increasing verbosity
enum class LogLevel {
    FATAL = 0,
    ERROR = 1,
    WARN  = 2,
    INFO  = 3,
    DEBUG = 4,
    TRACE = 5
};

// Color codes for Windows console
enum class LogColor {
    BLACK    = 0,
    BLUE     = 1,
    GREEN    = 2,
    CYAN     = 3,
    RED      = 4,
    MAGENTA  = 5,
    YELLOW   = 6,
    WHITE    = 7,
    BRIGHT_BLACK   = 8,
    BRIGHT_BLUE    = 9,
    BRIGHT_GREEN   = 10,
    BRIGHT_CYAN    = 11,
    BRIGHT_RED     = 12,
    BRIGHT_MAGENTA = 13,
    BRIGHT_YELLOW  = 14,
    BRIGHT_WHITE   = 15
};

// Forward declaration of LogMessage
class LogMessage;

// Logger class that handles configuration and dispatch of log messages
class Logger {
public:
    // Get the singleton instance
    static Logger& getInstance();

    // Disallow copying and moving
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Set the minimum log level
    void setLogLevel(LogLevel level);

    // Get the current log level
    LogLevel getLogLevel() const;

    // Check if a given log level is enabled
    bool isLevelEnabled(LogLevel level) const;

    // Add a log output file
    void addLogFile(const std::string& filename);

    // Enable/disable console output
    void setConsoleOutput(bool enabled);

    // Enable/disable file output
    void setFileOutput(bool enabled);

    // Set whether to include timestamps
    void setIncludeTimestamp(bool enabled);

    // Set whether to include source info (file:line)
    void setIncludeSourceInfo(bool enabled);

    // Log a message internal implementation (called by the LogMessage destructor)
    void logMessage(LogLevel level, const std::string& message, const char* file, int line);

private:
    // Private constructor for singleton
    Logger();

    // Destructor
    ~Logger();

    // Current log level
    LogLevel currentLevel = LogLevel::INFO;

    // Output settings
    bool consoleOutput = true;
    bool fileOutput = false;
    bool includeTimestamp = true;
    bool includeSourceInfo = true;

    // Console handle for Windows color output
    HANDLE consoleHandle;

    // Log file
    std::ofstream logFile;

    // Mutex for thread safety
    std::mutex logMutex;

    // Get the appropriate color for a log level
    LogColor getLevelColor(LogLevel level) const;

    // Convert log level to string
    std::string levelToString(LogLevel level) const;

    // Format a log message
    std::string formatLogMessage(LogLevel level, const std::string& message, const char* file, int line);

    // Set console text color
    void setConsoleColor(LogColor color);

    // Reset console text color
    void resetConsoleColor();
};

// Helper class for building log messages
class LogMessage {
public:
    LogMessage(LogLevel level, const char* file, int line);
    ~LogMessage();

    // Stream operator for easy logging
    template<typename T>
    LogMessage& operator<<(const T& value) {
        stream << value;
        return *this;
    }

private:
    LogLevel level;
    const char* file;
    int line;
    std::ostringstream stream;
};

// Variadic template for printf-style logging
template<typename... Args>
void logPrintf(LogLevel level, const char* file, int line, const char* format, Args... args) {
    // Check if this level is enabled
    if (!Logger::getInstance().isLevelEnabled(level)) {
        return;
    }

    // Calculate buffer size needed
    int size = snprintf(nullptr, 0, format, args...);
    if (size <= 0) {
        return;
    }

    // Create a buffer of the right size
    std::vector<char> buffer(size + 1);
    snprintf(buffer.data(), buffer.size(), format, args...);

    // Log the message
    Logger::getInstance().logMessage(level, buffer.data(), file, line);
}

} // namespace Zerith

// Convenience macros
#define LOG_FATAL(...) Zerith::logPrintf(Zerith::LogLevel::FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) Zerith::logPrintf(Zerith::LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  Zerith::logPrintf(Zerith::LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  Zerith::logPrintf(Zerith::LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) Zerith::logPrintf(Zerith::LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_TRACE(...) Zerith::logPrintf(Zerith::LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)

// Stream-style logging macros
#define LOG_FATAL_STREAM Zerith::LogMessage(Zerith::LogLevel::FATAL, __FILE__, __LINE__)
#define LOG_ERROR_STREAM Zerith::LogMessage(Zerith::LogLevel::ERROR, __FILE__, __LINE__)
#define LOG_WARN_STREAM  Zerith::LogMessage(Zerith::LogLevel::WARN, __FILE__, __LINE__)
#define LOG_INFO_STREAM  Zerith::LogMessage(Zerith::LogLevel::INFO, __FILE__, __LINE__)
#define LOG_DEBUG_STREAM Zerith::LogMessage(Zerith::LogLevel::DEBUG, __FILE__, __LINE__)
#define LOG_TRACE_STREAM Zerith::LogMessage(Zerith::LogLevel::TRACE, __FILE__, __LINE__)