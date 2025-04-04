#include "core/VulkanApp.hpp"
#include "Logger.hpp" // Include the logger
#include <iostream>
#include <stdexcept>

// Initialize the logger
void initializeLogger() {
    auto& logger = Zerith::Logger::getInstance();
    logger.setLogLevel(Zerith::LogLevel::INFO);  // Set default level

    // In debug builds, you might want to show DEBUG logs too
#ifdef _DEBUG
    logger.setLogLevel(Zerith::LogLevel::DEBUG);
#endif

    // Create logs directory and log file with timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    // Use a stringstream to format the time
    std::stringstream timeStr;
    timeStr << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");

    std::string logFileName = "logs/zerith_" + timeStr.str() + ".log";

    logger.addLogFile(logFileName);

    LOG_INFO("Zerith Engine starting up...");
}

int main() {
    // Initialize the logger first thing
    initializeLogger();

    LOG_INFO("Creating Vulkan application");
    VulkanApp app;

    try {
        LOG_INFO("Running application");
        app.run();
        LOG_INFO("Application exited normally");
    } catch (const std::exception& e) {
        // Log the error with proper formatting
        LOG_FATAL("Critical error: %s", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        LOG_FATAL("Unknown critical error occurred");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}