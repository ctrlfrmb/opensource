/**
 * @file    logger.hpp
 * @ingroup opensource
 * @brief   Logger class for handling logging with different levels and output methods.
 *          Supports conditional compilation for Android platform logging.
 * @author  leiwei
 * @date    2023.11.08
 * Copyright (c) ctrlfrmb 2023-2033
 */

#pragma once

#ifndef OPEN_SOURCE_LOGGER_HPP
#define OPEN_SOURCE_LOGGER_HPP

#include <iostream>
#include <string>
#include <functional>
#include <memory>

// Macro definition to enable Android logging
// #define ENABLE_ANDROID_LOGGING

#ifdef ENABLE_ANDROID_LOGGING
#include <android/log.h>
#else
#include "file_logger.hpp"
#endif

namespace opensource {
namespace ctrlfrmb {

// Enumeration for log levels
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

// Type definition for prefix callback function
using PrefixCallback = std::function<std::string()>;

// Logger class
class Logger {
public:
    // Constructor initializes the log level and the prefix callback
    Logger() : currentLevel(LogLevel::INFO), prefixCallback(nullptr)
#ifdef ENABLE_ANDROID_LOGGING
    , androidLogLevel(ANDROID_LOG_DEFAULT)
#else
    , fileLogger(nullptr)
#endif
    {}

    // Sets the current log level
    void setLevel(LogLevel level) {
        currentLevel = level;
    }

    // Sets the prefix callback for custom log message prefixes
    void setPrefixCallback(PrefixCallback callback) {
        prefixCallback = callback;
    }

#ifndef ENABLE_ANDROID_LOGGING
    // Enable file logging
    bool enableFileWrite(const FileLoggerConfig& config) {
        try {
            fileLogger = std::make_unique<FileLogger>(config);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to enable file logging: " << e.what() << std::endl;
            return false;
        }
    }

    // Disable file logging
    void disableFileWrite() {
        fileLogger.reset();
    }
#endif

    // Log methods for different levels
    void trace(const std::string& message) {
        if (LogLevel::TRACE >= currentLevel) {
#ifdef ENABLE_ANDROID_LOGGING
            androidLogLevel = ANDROID_LOG_DEBUG;
#endif
            log("TRACE", message);
        }
    }

    void debug(const std::string& message) {
        if (LogLevel::DEBUG >= currentLevel) {
#ifdef ENABLE_ANDROID_LOGGING
            androidLogLevel = ANDROID_LOG_DEBUG;
#endif
            log("DEBUG", message);
        }
    }

    void info(const std::string& message) {
        if (LogLevel::INFO >= currentLevel) {
#ifdef ENABLE_ANDROID_LOGGING
            androidLogLevel = ANDROID_LOG_INFO;
#endif
            log("INFO", message);
        }
    }

    void warn(const std::string& message) {
        if (LogLevel::WARN >= currentLevel) {
#ifdef ENABLE_ANDROID_LOGGING
            androidLogLevel = ANDROID_LOG_WARN;
#endif
            log("WARN", message);
        }
    }

    void error(const std::string& message) {
        if (LogLevel::ERROR >= currentLevel) {
#ifdef ENABLE_ANDROID_LOGGING
            androidLogLevel = ANDROID_LOG_ERROR;
#endif
            log("ERROR", message);
        }
    }

    void fatal(const std::string& message) {
        if (LogLevel::FATAL >= currentLevel) {
#ifdef ENABLE_ANDROID_LOGGING
            androidLogLevel = ANDROID_LOG_FATAL;
#endif
            log("FATAL", message);
        }
    }

private:
    LogLevel currentLevel;  // Current log level
    PrefixCallback prefixCallback;  // Callback for generating log message prefixes

#ifdef ENABLE_ANDROID_LOGGING
    int androidLogLevel;  // Android specific log level
#else
    std::unique_ptr<FileLogger> fileLogger;  // File logger instance
#endif

    // Generic log output method
    void log(const std::string& level, const std::string& message) {
        std::string prefix = prefixCallback ? prefixCallback() : "";
        std::string fullMessage = prefix + " [" + level + "]: " + message;

#ifdef ENABLE_ANDROID_LOGGING
        __android_log_print(androidLogLevel, "LoggerTag", "%s", fullMessage.c_str());
#else
        // Write to file if file logging is enabled
        if (fileLogger) {
            fileLogger->write(fullMessage);
        }
        else {
            std::cout << fullMessage << std::endl;
        }
#endif
    }
};
}  // namespace ctrlfrmb
}  // namespace opensource

#endif // !OPEN_SOURCE_LOGGER_HPP
