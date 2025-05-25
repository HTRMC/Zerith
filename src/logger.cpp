#include "logger.h"
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <cstdarg>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Start the background logging thread
    loggerThread = std::thread(&Logger::processLogQueue, this);
}

Logger::~Logger() {
    shutdown();
}

void Logger::shutdown() {
    // Signal the thread to stop
    running = false;
    queueCondition.notify_all();
    
    // Wait for thread to finish
    if (loggerThread.joinable()) {
        loggerThread.join();
    }
    
    // Close log file if open
    std::lock_guard<std::mutex> lock(fileMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

LogLevel Logger::getLogLevel() const {
    return currentLevel;
}

bool Logger::isLevelEnabled(LogLevel level) const {
    return static_cast<int>(level) <= static_cast<int>(currentLevel.load());
}

void Logger::addLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(fileMutex);

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
        std::cerr << getColorCode(LogColor::BRIGHT_RED) 
                  << "ERROR: Failed to open log file: " << filename 
                  << getResetCode() << std::endl;
    }
}

void Logger::setConsoleOutput(bool enabled) {
    consoleOutput = enabled;
}

void Logger::setFileOutput(bool enabled) {
    fileOutput = enabled;
}

void Logger::setIncludeTimestamp(bool enabled) {
    includeTimestamp = enabled;
}

void Logger::setIncludeSourceInfo(bool enabled) {
    includeSourceInfo = enabled;
}

void Logger::logMessage(LogLevel level, const std::string& message, const char* file, int line) {
    // Return early if this level is disabled
    if (!isLevelEnabled(level)) {
        return;
    }

    // Create log entry
    LogEntry entry;
    entry.level = level;
    entry.message = message;
    entry.file = file ? file : "";
    entry.line = line;
    entry.timestamp = std::chrono::system_clock::now();

    // Add to queue
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        messageQueue.push(std::move(entry));
    }
    
    // Notify the logging thread
    queueCondition.notify_one();
}

void Logger::flush() {
    // Wait until queue is empty
    std::unique_lock<std::mutex> lock(queueMutex);
    queueCondition.wait(lock, [this] { return messageQueue.empty(); });
}

void Logger::processLogQueue() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        // Wait for messages or shutdown signal
        queueCondition.wait(lock, [this] { 
            return !messageQueue.empty() || !running; 
        });
        
        // Process all pending messages
        while (!messageQueue.empty()) {
            LogEntry entry = std::move(messageQueue.front());
            messageQueue.pop();
            
            // Unlock while processing to allow new messages to be queued
            lock.unlock();
            
            processLogEntry(entry);
            
            // Re-lock for next iteration
            lock.lock();
        }
    }
    
    // Process any remaining messages before shutting down
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!messageQueue.empty()) {
        LogEntry entry = std::move(messageQueue.front());
        messageQueue.pop();
        processLogEntry(entry);
    }
}

void Logger::processLogEntry(const LogEntry& entry) {
    // Format the message
    std::string formattedMessage = formatLogMessage(entry);

    // Log to console
    if (consoleOutput) {
        std::string colorCode = getColorCode(getLevelColor(entry.level));
        std::string resetCode = getResetCode();
        std::cout << colorCode << formattedMessage << resetCode << std::endl;
    }

    // Log to file
    if (fileOutput && logFile.is_open()) {
        std::lock_guard<std::mutex> lock(fileMutex);
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
        case LogLevel::WARN:  return "WARN";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::TRACE: return "TRACE";
        default:              return "UNKNOWN";
    }
}

std::string Logger::formatLogMessage(const LogEntry& entry) const {
    std::ostringstream formatted;

    // Add timestamp if enabled
    if (includeTimestamp) {
        auto time = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;

        formatted << std::put_time(std::localtime(&time), "%H:%M:%S")
                  << '.' << std::setfill('0') << std::setw(3) << ms.count() << " ";
    }

    // Add log level
    formatted << "[" << levelToString(entry.level) << "] ";

    // Add source info if enabled
    if (includeSourceInfo && !entry.file.empty()) {
        // Extract just the filename, not the full path
        std::string filename(entry.file);
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = filename.substr(lastSlash + 1);
        }

        formatted << "(" << filename << ":" << entry.line << ") ";
    }

    // Add the actual message
    formatted << entry.message;

    return formatted.str();
}

std::string Logger::getColorCode(LogColor color) const {
    return "\033[" + std::to_string(static_cast<int>(color)) + "m";
}

std::string Logger::getResetCode() const {
    return "\033[0m";
}

// LogMessage implementation
LogMessage::LogMessage(LogLevel level, const char* file, int line)
    : level(level), file(file), line(line) {
}

LogMessage::~LogMessage() {
    // When the message object is destroyed, send the built message to the logger
    Logger::getInstance().logMessage(level, stream.str(), file, line);
}