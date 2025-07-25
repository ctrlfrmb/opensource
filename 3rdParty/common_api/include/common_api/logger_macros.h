/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2022 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei
* Version: v2.0.0
* Date: 2022-09-12
*----------------------------------------------------------------------------*/

/**
* @file logger_macros.h
* @brief High-performance logging macros with move semantics optimization and integrated timer functionality
*
* This header provides optimized logging macros that leverage fmt library's
* compile-time format string checking and modern C++ move semantics for
* maximum performance with zero unnecessary copies. Additionally includes
* high-precision timing functionality for performance measurement.
*
* Features:
* - Compile-time format string validation
* - Move semantics for optimal string handling
* - Zero-overhead when logging is disabled
* - Thread-safe asynchronous logging
* - Automatic type deduction and perfect forwarding
* - Multiple overloads for different parameter types
* - Integrated high-precision timing with automatic logging
* - RAII-based scoped timing measurements
*
* Usage example:
*   LOG_INIT("app.log", LOG_LEVEL_DEBUG, 10, 5);
*   LOG_START(true);
*
*   LOG_INFO("Starting application version {}", appVersion);
*   LOG_DEBUG("Connection established with {}", clientAddress);
*   LOG_DEBUG_HEX("Received data: ", receivedBytes, dataLength);
*
*   // Automatic timing with logging
*   void myFunction() {
*       TIMER_LOG_TIMEOUT("Data processing", 1000);  // WARN if > 1000μs
*       // ... code to measure ...
*   }  // Automatically logs timing on scope exit
*/

#ifndef COMMON_LOGGER_MACROS_H
#define COMMON_LOGGER_MACROS_H

#include <fmt/core.h>
#include <string>
#include <chrono>
#include <cstdint>
#include "common_global.h"

// Forward declarations to avoid including logger.h in header
namespace Common {
class Logger;
enum class LogLevel;
class ScopedTimerInternal;  // Forward declaration for timer implementation
}

// Log level constants - explicit and clear
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

namespace Common {

// Core logger functions - using std::string for better performance
COMMON_API_EXPORT void logInit(const std::string& logFile, int level, int maxSize, int maxFiles);
COMMON_API_EXPORT void logStart(bool toConsole);
COMMON_API_EXPORT void logStop();
COMMON_API_EXPORT void logSoftwareInfo(const std::string& softwareName,
                                      const std::string& version,
                                      const std::string& author,
                                      const std::string& platform);

// Hex logging functions with multiple overloads for optimal performance
COMMON_API_EXPORT void logHex(int level, std::string&& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logHex(int level, const std::string& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logHex(int level, const char* prefix, const void* data, size_t length);

COMMON_API_EXPORT void logDebugHex(std::string&& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logDebugHex(const std::string& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logDebugHex(const char* prefix, const void* data, size_t length);

// Internal functions for level checking and raw logging with move semantics
COMMON_API_EXPORT bool shouldLogLevel(int level);
COMMON_API_EXPORT void logRawString(int level, std::string&& message);

// Timer factory functions - used internally by macros
COMMON_API_EXPORT ScopedTimerInternal* createScopedTimer(const char* logMessage, uint64_t timeoutMicroseconds);
COMMON_API_EXPORT void destroyScopedTimer(ScopedTimerInternal* timer);

// High-performance template functions for formatted logging
template<typename... Args>
inline void logDebug(fmt::format_string<Args...> fmt, Args&&... args) {
    constexpr int level = LOG_LEVEL_DEBUG;
    if (shouldLogLevel(level)) {
        // Direct move of formatted string - zero copies
        logRawString(level, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

template<typename... Args>
inline void logInfo(fmt::format_string<Args...> fmt, Args&&... args) {
    constexpr int level = LOG_LEVEL_INFO;
    if (shouldLogLevel(level)) {
        logRawString(level, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

template<typename... Args>
inline void logWarn(fmt::format_string<Args...> fmt, Args&&... args) {
    constexpr int level = LOG_LEVEL_WARN;
    if (shouldLogLevel(level)) {
        logRawString(level, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

template<typename... Args>
inline void logError(fmt::format_string<Args...> fmt, Args&&... args) {
    constexpr int level = LOG_LEVEL_ERROR;
    if (shouldLogLevel(level)) {
        logRawString(level, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

// Overloads for string logging with move semantics
inline void logDebug(std::string&& message) {
    constexpr int level = LOG_LEVEL_DEBUG;
    if (shouldLogLevel(level)) {
        logRawString(level, std::move(message));
    }
}

inline void logInfo(std::string&& message) {
    constexpr int level = LOG_LEVEL_INFO;
    if (shouldLogLevel(level)) {
        logRawString(level, std::move(message));
    }
}

inline void logWarn(std::string&& message) {
    constexpr int level = LOG_LEVEL_WARN;
    if (shouldLogLevel(level)) {
        logRawString(level, std::move(message));
    }
}

inline void logError(std::string&& message) {
    constexpr int level = LOG_LEVEL_ERROR;
    if (shouldLogLevel(level)) {
        logRawString(level, std::move(message));
    }
}

// Overloads for const string reference (avoid unnecessary copies)
inline void logDebug(const std::string& message) {
    constexpr int level = LOG_LEVEL_DEBUG;
    if (shouldLogLevel(level)) {
        logRawString(level, std::string(message)); // Copy only when necessary
    }
}

inline void logInfo(const std::string& message) {
    constexpr int level = LOG_LEVEL_INFO;
    if (shouldLogLevel(level)) {
        logRawString(level, std::string(message));
    }
}

inline void logWarn(const std::string& message) {
    constexpr int level = LOG_LEVEL_WARN;
    if (shouldLogLevel(level)) {
        logRawString(level, std::string(message));
    }
}

inline void logError(const std::string& message) {
    constexpr int level = LOG_LEVEL_ERROR;
    if (shouldLogLevel(level)) {
        logRawString(level, std::string(message));
    }
}

// C-string overloads for compatibility
inline void logDebug(const char* message) {
    constexpr int level = LOG_LEVEL_DEBUG;
    if (shouldLogLevel(level)) {
        logRawString(level, std::string(message));
    }
}

inline void logInfo(const char* message) {
    constexpr int level = LOG_LEVEL_INFO;
    if (shouldLogLevel(level)) {
        logRawString(level, std::string(message));
    }
}

inline void logWarn(const char* message) {
    constexpr int level = LOG_LEVEL_WARN;
    if (shouldLogLevel(level)) {
        logRawString(level, std::string(message));
    }
}

inline void logError(const char* message) {
    constexpr int level = LOG_LEVEL_ERROR;
    if (shouldLogLevel(level)) {
        logRawString(level, std::string(message));
    }
}

//=============================================================================
// Timer RAII Wrapper - handles timer lifetime automatically
//=============================================================================

/**
 * @brief RAII wrapper for scoped timer management
 *
 * This class manages the lifetime of ScopedTimerInternal objects using RAII.
 * It automatically creates a timer on construction and destroys it on destruction,
 * ensuring proper cleanup even in case of exceptions.
 */
class TimerRAII {
public:
    /**
     * @brief Constructor - creates and starts a scoped timer
     * @param msg Log message for timing output
     * @param timeout Timeout threshold in microseconds (0 = always DEBUG level)
     */
    TimerRAII(const char* msg, uint64_t timeout)
        : timer_(createScopedTimer(msg, timeout)) {}

    /**
     * @brief Destructor - automatically destroys the timer and logs results
     */
    ~TimerRAII() {
        if (timer_) {
            destroyScopedTimer(timer_);
        }
    }

    // Disable copy and move to ensure proper RAII semantics
    TimerRAII(const TimerRAII&) = delete;
    TimerRAII& operator=(const TimerRAII&) = delete;
    TimerRAII(TimerRAII&&) = delete;
    TimerRAII& operator=(TimerRAII&&) = delete;

private:
    ScopedTimerInternal* timer_;
};

} // namespace Common

// Convenient macros - these are now just thin wrappers
#define LOG_INIT(logFile, level, maxSize, maxFiles) \
    Common::logInit(logFile, level, maxSize, maxFiles)

#define LOG_START(toConsole) \
    Common::logStart(toConsole)

#define LOG_STOP() \
    Common::logStop()

#define LOG_SOFTWARE_INFO(softwareName, version, author, platform) \
    Common::logSoftwareInfo(softwareName, version, author, platform)

#define LOG_HEX(level, prefix, data, length) \
    Common::logHex(level, prefix, data, length)

#define LOG_DEBUG_HEX(prefix, data, length) \
    Common::logDebugHex(prefix, data, length)

// High-performance logging macros
#define LOG_DEBUG(...) Common::logDebug(__VA_ARGS__)
#define LOG_INFO(...) Common::logInfo(__VA_ARGS__)
#define LOG_WARN(...) Common::logWarn(__VA_ARGS__)
#define LOG_ERROR(...) Common::logError(__VA_ARGS__)

//=============================================================================
// Timer Logging Macros
//=============================================================================

#ifndef TIMER_LOG_DISABLED
    #define TIMER_LOG() \
        Common::TimerRAII TIMER_UNIQUE_NAME(timer_raii_)(__FUNCTION__, static_cast<uint64_t>(0))

    #define TIMER_LOG_MSG(msg) \
        Common::TimerRAII TIMER_UNIQUE_NAME(timer_raii_)((msg), static_cast<uint64_t>(0))

    #define TIMER_LOG_TIMEOUT(msg, timeout) \
        Common::TimerRAII TIMER_UNIQUE_NAME(timer_raii_)((msg), static_cast<uint64_t>(timeout))

    #define TIMER_UNIQUE_NAME(prefix) TIMER_CONCAT(prefix, __LINE__)
    #define TIMER_CONCAT(a, b) TIMER_CONCAT_IMPL(a, b)
    #define TIMER_CONCAT_IMPL(a, b) a##b
#else
    // When timer logging is disabled, macros become no-ops
    #define TIMER_LOG() do { } while(0)
    #define TIMER_LOG_MSG(msg) do { } while(0)
    #define TIMER_LOG_TIMEOUT(msg, timeout) do { } while(0)
#endif

#endif // COMMON_LOGGER_MACROS_H
