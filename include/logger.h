#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>
#include <cstdio>

class Logger {
public:
    static Logger& getInstance();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void setLogLevel(spdlog::level::level_enum level);
    spdlog::level::level_enum getLogLevel() const;
    bool isLevelEnabled(spdlog::level::level_enum level) const;
    
    void addLogFile(const std::string& filename, size_t max_size = 1048576 * 5, size_t max_files = 3);
    void setConsoleOutput(bool enabled);
    void setPattern(const std::string& pattern);
    
    void flush();
    void shutdown();
    
    std::shared_ptr<spdlog::logger> getLogger() { return logger_; }
    
    // Printf-style logging helpers
    template<typename... Args>
    void logPrintf(spdlog::level::level_enum level, const char* format, Args... args) {
        if (logger_->should_log(level)) {
            char buffer[4096];
            std::snprintf(buffer, sizeof(buffer), format, args...);
            logger_->log(level, buffer);
        }
    }

private:
    Logger();
    ~Logger();
    
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink_;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> file_sink_;
};

#define LOG_FATAL(...) Logger::getInstance().logPrintf(spdlog::level::critical, __VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().logPrintf(spdlog::level::err, __VA_ARGS__)
#define LOG_WARN(...)  Logger::getInstance().logPrintf(spdlog::level::warn, __VA_ARGS__)
#define LOG_INFO(...)  Logger::getInstance().logPrintf(spdlog::level::info, __VA_ARGS__)
#define LOG_DEBUG(...) Logger::getInstance().logPrintf(spdlog::level::debug, __VA_ARGS__)
#define LOG_TRACE(...) Logger::getInstance().logPrintf(spdlog::level::trace, __VA_ARGS__)

#define LOG_FATAL_STREAM Logger::getInstance().getLogger()->critical
#define LOG_ERROR_STREAM Logger::getInstance().getLogger()->error
#define LOG_WARN_STREAM  Logger::getInstance().getLogger()->warn
#define LOG_INFO_STREAM  Logger::getInstance().getLogger()->info
#define LOG_DEBUG_STREAM Logger::getInstance().getLogger()->debug
#define LOG_TRACE_STREAM Logger::getInstance().getLogger()->trace