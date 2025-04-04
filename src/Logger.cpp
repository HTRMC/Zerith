// Logger.cpp
#include "Logger.hpp"
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <cstdarg>

namespace Zerith {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Get console handle for color output
    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
}

Logger::~Logger() {
    // Close log file if open
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    currentLevel = level;
}

LogLevel Logger::getLogLevel() const {
    return currentLevel;
}

bool Logger::isLevelEnabled(LogLevel level) const {
    return static_cast<int>(level) <= static_cast<int>(currentLevel);
}

void Logger::addLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);

    // Close existing log file if open
    if (logFile.is_open()) {
        logFile.close();
    }

    // Create directory if it doesn't exist
    std::filesystem::path filePath(filename);
    std::filesystem::create_directories(filePath.parent_path());

    // Open log file
    logFile.open(filename, std::ios::out | std::ios::app);

    if (logFile.is_open()) {
        fileOutput = true;
        // Write header to log file
        std::time_t now = std::time(nullptr);
        logFile << "\n--- Log started at "
                << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S")
                << " ---\n\n";
        logFile.flush();
    } else {
        // If we couldn't open the file, log to console
        std::lock_guard<std::mutex> consoleLock(logMutex);
        setConsoleColor(LogColor::BRIGHT_RED);
        std::cerr << "ERROR: Failed to open log file: " << filename << std::endl;
        resetConsoleColor();
    }
}

void Logger::setConsoleOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex);
    consoleOutput = enabled;
}

void Logger::setFileOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex);
    fileOutput = enabled;
}

void Logger::setIncludeTimestamp(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex);
    includeTimestamp = enabled;
}

void Logger::setIncludeSourceInfo(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex);
    includeSourceInfo = enabled;
}

void Logger::logMessage(LogLevel level, const std::string& message, const char* file, int line) {
    // Return early if this level is disabled
    if (!isLevelEnabled(level)) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);

    // Format the message
    std::string formattedMessage = formatLogMessage(level, message, file, line);

    // Log to console
    if (consoleOutput) {
        setConsoleColor(getLevelColor(level));
        std::cout << formattedMessage << std::endl;
        resetConsoleColor();
    }

    // Log to file
    if (fileOutput && logFile.is_open()) {
        logFile << formattedMessage << std::endl;
        logFile.flush();
    }
}

LogColor Logger::getLevelColor(LogLevel level) const {
    switch (level) {
        case LogLevel::FATAL: return LogColor::BRIGHT_RED;
        case LogLevel::ERROR: return LogColor::RED;
        case LogLevel::WARN:  return LogColor::YELLOW;
        case LogLevel::INFO:  return LogColor::WHITE;
        case LogLevel::DEBUG: return LogColor::CYAN;
        case LogLevel::TRACE: return LogColor::BRIGHT_BLACK;
        default:              return LogColor::WHITE;
    }
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::FATAL: return "FATAL";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::TRACE: return "TRACE";
        default:              return "UNKNOWN";
    }
}

std::string Logger::formatLogMessage(LogLevel level, const std::string& message, const char* file, int line) {
    std::ostringstream formatted;

    // Add timestamp if enabled
    if (includeTimestamp) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        formatted << std::put_time(std::localtime(&time), "%H:%M:%S")
                  << '.' << std::setfill('0') << std::setw(3) << ms.count() << " ";
    }

    // Add log level
    formatted << "[" << levelToString(level) << "] ";

    // Add source info if enabled
    if (includeSourceInfo && file != nullptr) {
        // Extract just the filename, not the full path
        std::string filename(file);
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }

        formatted << "(" << filename << ":" << line << ") ";
    }

    // Add the actual message
    formatted << message;

    return formatted.str();
}

void Logger::setConsoleColor(LogColor color) {
    if (consoleHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    WORD attributes = static_cast<WORD>(color);
    SetConsoleTextAttribute(consoleHandle, attributes);
}

void Logger::resetConsoleColor() {
    if (consoleHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    // Reset to default (white text on black background)
    SetConsoleTextAttribute(consoleHandle, static_cast<WORD>(LogColor::WHITE));
}

// LogMessage implementation
LogMessage::LogMessage(LogLevel level, const char* file, int line)
    : level(level), file(file), line(line) {
}

LogMessage::~LogMessage() {
    // When the message object is destroyed, send the built message to the logger
    Logger::getInstance().logMessage(level, stream.str(), file, line);
}

} // namespace Zerith