#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>
#include <memory>
#include <iomanip>

// Log levels in increasing verbosity
enum class LogLevel {
    FATAL = 0,
    ERROR = 1,
    WARN  = 2,
    INFO  = 3,
    DEBUG = 4,
    TRACE = 5
};

// ANSI color codes for cross-platform console colors
enum class LogColor {
    BLACK    = 30,
    RED      = 31,
    GREEN    = 32,
    YELLOW   = 33,
    BLUE     = 34,
    MAGENTA  = 35,
    CYAN     = 36,
    WHITE    = 37,
    BRIGHT_BLACK   = 90,
    BRIGHT_RED     = 91,
    BRIGHT_GREEN   = 92,
    BRIGHT_YELLOW  = 93,
    BRIGHT_BLUE    = 94,
    BRIGHT_MAGENTA = 95,
    BRIGHT_CYAN    = 96,
    BRIGHT_WHITE   = 97
};

// Forward declaration
class LogMessage;

// Async logger with background thread
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

    // Log a message (adds to queue for async processing)
    void logMessage(LogLevel level, const std::string& message, const char* file, int line);

    // Flush all pending log messages
    void flush();

    // Stop the logger (call before program exit)
    void shutdown();

private:
    struct LogEntry {
        LogLevel level;
        std::string message;
        std::string file;
        int line;
        std::chrono::system_clock::time_point timestamp;
    };

    // Private constructor for singleton
    Logger();

    // Destructor
    ~Logger();

    // Background thread function
    void processLogQueue();

    // Process a single log entry
    void processLogEntry(const LogEntry& entry);

    // Current log level
    std::atomic<LogLevel> currentLevel{LogLevel::INFO};

    // Output settings
    std::atomic<bool> consoleOutput{true};
    std::atomic<bool> fileOutput{false};
    std::atomic<bool> includeTimestamp{true};
    std::atomic<bool> includeSourceInfo{true};

    // Log file
    std::ofstream logFile;
    std::mutex fileMutex;

    // Message queue
    std::queue<LogEntry> messageQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;

    // Background thread
    std::thread loggerThread;
    std::atomic<bool> running{true};

    // Get the appropriate color for a log level
    LogColor getLevelColor(LogLevel level) const;

    // Convert log level to string
    std::string levelToString(LogLevel level) const;

    // Format a log message
    std::string formatLogMessage(const LogEntry& entry) const;

    // Set console text color using ANSI escape codes
    std::string getColorCode(LogColor color) const;

    // Reset console text color
    std::string getResetCode() const;
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

// Convenience macros
#define LOG_FATAL(...) logPrintf(LogLevel::FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logPrintf(LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  logPrintf(LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  logPrintf(LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) logPrintf(LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_TRACE(...) logPrintf(LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)

// Stream-style logging macros
#define LOG_FATAL_STREAM LogMessage(LogLevel::FATAL, __FILE__, __LINE__)
#define LOG_ERROR_STREAM LogMessage(LogLevel::ERROR, __FILE__, __LINE__)
#define LOG_WARN_STREAM  LogMessage(LogLevel::WARN, __FILE__, __LINE__)
#define LOG_INFO_STREAM  LogMessage(LogLevel::INFO, __FILE__, __LINE__)
#define LOG_DEBUG_STREAM LogMessage(LogLevel::DEBUG, __FILE__, __LINE__)
#define LOG_TRACE_STREAM LogMessage(LogLevel::TRACE, __FILE__, __LINE__)