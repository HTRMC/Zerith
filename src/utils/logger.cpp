#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink_->set_level(spdlog::level::trace);
    console_sink_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    logger_ = std::make_shared<spdlog::logger>("Zerith", console_sink_);
    logger_->set_level(spdlog::level::info);
    logger_->flush_on(spdlog::level::warn);
    
    spdlog::register_logger(logger_);
}

Logger::~Logger() {
    shutdown();
}

void Logger::setLogLevel(spdlog::level::level_enum level) {
    logger_->set_level(level);
}

spdlog::level::level_enum Logger::getLogLevel() const {
    return logger_->level();
}

bool Logger::isLevelEnabled(spdlog::level::level_enum level) const {
    return logger_->should_log(level);
}

void Logger::addLogFile(const std::string& filename, size_t max_size, size_t max_files) {
    std::filesystem::path filePath(filename);
    std::filesystem::create_directories(filePath.parent_path());

    file_sink_ = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, max_size, max_files);
    file_sink_->set_level(spdlog::level::trace);
    file_sink_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    spdlog::sinks_init_list sinks = {console_sink_, file_sink_};
    logger_ = std::make_shared<spdlog::logger>("Zerith", sinks);
    logger_->set_level(spdlog::level::info);
    logger_->flush_on(spdlog::level::warn);
    
    spdlog::drop("Zerith");
    spdlog::register_logger(logger_);
}

void Logger::setConsoleOutput(bool enabled) {
    if (enabled) {
        if (file_sink_) {
            spdlog::sinks_init_list sinks = {console_sink_, file_sink_};
            logger_ = std::make_shared<spdlog::logger>("Zerith", sinks);
        } else {
            logger_ = std::make_shared<spdlog::logger>("Zerith", console_sink_);
        }
    } else {
        if (file_sink_) {
            logger_ = std::make_shared<spdlog::logger>("Zerith", file_sink_);
        }
    }
    logger_->set_level(getLogLevel());
    logger_->flush_on(spdlog::level::warn);
    
    spdlog::drop("Zerith");
    spdlog::register_logger(logger_);
}

void Logger::setPattern(const std::string& pattern) {
    logger_->set_pattern(pattern);
}

void Logger::flush() {
    logger_->flush();
}

void Logger::shutdown() {
    if (logger_) {
        logger_->flush();
        spdlog::drop("Zerith");
        logger_.reset();
    }
    console_sink_.reset();
    file_sink_.reset();
}