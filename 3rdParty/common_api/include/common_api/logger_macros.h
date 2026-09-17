/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2022-2042 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
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
* - Multi-DLL isolated logging (each DLL writes to its own log file)
*
* ============================================================================
* Multi-DLL Usage (shared common_api.dll topology)
* ============================================================================
*
* When multiple DLLs (e.g. doip_api.dll, dbc_api.dll, lvds_api.dll) all link
* against the same common_api.dll, each DLL can maintain its own isolated log
* file. The key is calling LOG_SET_DEFAULT_LOGGER with a unique name per DLL.
*
* Pattern for each business DLL:
*
*   // ---- In DllMain or static init ----
*   static void InitLibrary() {
*       LOG_SET_DEFAULT_LOGGER("my_dll_name");  // binds this DLL to its Logger
*   }
*
*   BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
*       if (reason == DLL_PROCESS_ATTACH) InitLibrary();
*       return TRUE;
*   }
*
*   // ---- Exported OpenLog API ----
*   int MyDllOpenLog(const char* logFile, int level, int maxSize, int maxFiles) {
*       LOG_INIT(logFile, level, maxSize, maxFiles);
*       LOG_START(false);
*       LOG_SOFTWARE_INFO("MyDll", "1.0.0", "author", "platform");
*       LOG_INFO("Log system initialized: file={}", logFile);
*       return 0;
*   }
*
*   // ---- Exported CloseLog API ----
*   void MyDllCloseLog() {
*       LOG_STOP();
*   }
*
*   // ---- Normal usage anywhere in this DLL ----
*   void SomeFunction() {
*       LOG_INFO("Processing request id={}", id);
*       LOG_DEBUG_HEX("Raw data: ", buf, len);
*   }
*
* Host process (e.g. LabView, FKMaster) loads multiple DLLs:
*
*   DoIPOpenLog("C:/logs/doip.log", LOG_LEVEL_INFO, 5, 3);
*   DBCOpenLog("C:/logs/dbc.log", LOG_LEVEL_INFO, 5, 3);
*   LVDSOpenLog("C:/logs/lvds.log", LOG_LEVEL_INFO, 5, 3);
*   // Each DLL now writes exclusively to its own log file.
*   // No cross-write between DLLs. Order of initialization does not matter.
*
* Key rules:
*   1. LOG_SET_DEFAULT_LOGGER must be called BEFORE LOG_INIT/LOG_START.
*      Best place: DllMain(DLL_PROCESS_ATTACH) or a static initializer.
*   2. The logger name string must be unique per DLL (e.g. "doip", "dbc").
*   3. Each DLL's LOG_INIT points to a different file path.
*   4. LOG_INFO/LOG_WARN/etc. automatically route to the correct logger
*      based on which DLL the calling code resides in.
*   5. No code changes needed in existing DLLs that already follow this pattern.
*
* ============================================================================
*
* Single-module usage (simple case):
*   LOG_INIT("app.log", LOG_LEVEL_DEBUG, 10, 5);
*   LOG_START(true);
*
*   LOG_INFO("Starting application version {}", appVersion);
*   LOG_DEBUG("Connection established with {}", clientAddress);
*   LOG_DEBUG_HEX("Received data: ", receivedBytes, dataLength);
*
*   // Automatic timing with logging
*   void myFunction() {
*       TIMER_LOG_TIMEOUT("Data processing", 1000);  // WARN if > 1000us
*       // ... code to measure ...
*   }  // Automatically logs timing on scope exit
*/

#ifndef COMMON_LOGGER_MACROS_H
#define COMMON_LOGGER_MACROS_H

#include <fmt/format.h>

#include <string>
#include <cstdint>
#include <chrono>
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

enum class LogFileMode;
class Logger;

COMMON_API_EXPORT void setDefaultLoggerName(const std::string& loggerName);
COMMON_API_EXPORT Logger* resolveLogger(const std::string& loggerName);
COMMON_API_EXPORT Logger* defaultLogger();
COMMON_API_EXPORT void logInitFor(Logger* logger, const std::string& logFile, int level, int maxSize, int maxFiles, int fileMode = 0);
COMMON_API_EXPORT void logStartFor(Logger* logger, bool toConsole);
COMMON_API_EXPORT void logStopFor(Logger* logger);
COMMON_API_EXPORT void logSoftwareInfoFor(Logger* logger,
                                         const std::string& softwareName,
                                         const std::string& version,
                                         const std::string& author,
                                         const std::string& platform);
COMMON_API_EXPORT void logInit(const std::string& logFile, int level, int maxSize, int maxFiles, int fileMode = 0);
COMMON_API_EXPORT void logStart(bool toConsole);
COMMON_API_EXPORT void logStop();
COMMON_API_EXPORT void logSoftwareInfo(const std::string& softwareName,
                                      const std::string& version,
                                      const std::string& author,
                                      const std::string& platform);

namespace detail {
inline Logger*& moduleLoggerCache() {
    static Logger* logger = nullptr;
    return logger;
}

inline std::string& moduleLoggerName() {
    static std::string loggerName{"default"};
    return loggerName;
}

inline void setModuleLoggerName(const std::string& loggerName) {
    moduleLoggerName() = loggerName;
    moduleLoggerCache() = resolveLogger(loggerName);
}

inline Logger* moduleLogger() {
    Logger*& cached = moduleLoggerCache();
    if (cached == nullptr) {
        cached = resolveLogger(moduleLoggerName());
    }
    return cached;
}

inline Logger& currentLogger() {
    return *moduleLogger();
}
}

// Hex logging functions with multiple overloads for optimal performance
COMMON_API_EXPORT void logHexFor(Logger* logger, int level, std::string&& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logHexFor(Logger* logger, int level, const std::string& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logHexFor(Logger* logger, int level, const char* prefix, const void* data, size_t length);
COMMON_API_EXPORT void logHex(int level, std::string&& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logHex(int level, const std::string& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logHex(int level, const char* prefix, const void* data, size_t length);

COMMON_API_EXPORT void logDebugHex(std::string&& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logDebugHex(const std::string& prefix, const void* data, size_t length);
COMMON_API_EXPORT void logDebugHex(const char* prefix, const void* data, size_t length);

// Internal functions for level checking and raw logging with move semantics
COMMON_API_EXPORT bool shouldLogLevelFor(Logger* logger, int level);
COMMON_API_EXPORT void logRawStringFor(Logger* logger, int level, std::string&& message);
COMMON_API_EXPORT bool shouldLogLevel(int level);
COMMON_API_EXPORT void logRawString(int level, std::string&& message);

// Timer factory functions - used internally by macros
COMMON_API_EXPORT ScopedTimerInternal* createScopedTimer(const char* logMessage, uint64_t timeoutMicroseconds);
COMMON_API_EXPORT void destroyScopedTimer(ScopedTimerInternal* timer);

// High-performance template functions for formatted logging
template<typename... Args>
inline void logDebug(fmt::format_string<Args...> fmt, Args&&... args) {
    Logger* logger = detail::moduleLogger();
    constexpr int level = LOG_LEVEL_DEBUG;
    if (shouldLogLevelFor(logger, level)) {
        logRawStringFor(logger, level, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

template<typename... Args>
inline void logInfo(fmt::format_string<Args...> fmt, Args&&... args) {
    Logger* logger = detail::moduleLogger();
    constexpr int level = LOG_LEVEL_INFO;
    if (shouldLogLevelFor(logger, level)) {
        logRawStringFor(logger, level, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

template<typename... Args>
inline void logWarn(fmt::format_string<Args...> fmt, Args&&... args) {
    Logger* logger = detail::moduleLogger();
    constexpr int level = LOG_LEVEL_WARN;
    if (shouldLogLevelFor(logger, level)) {
        logRawStringFor(logger, level, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

template<typename... Args>
inline void logError(fmt::format_string<Args...> fmt, Args&&... args) {
    Logger* logger = detail::moduleLogger();
    constexpr int level = LOG_LEVEL_ERROR;
    if (shouldLogLevelFor(logger, level)) {
        logRawStringFor(logger, level, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

// Overloads for string logging with move semantics
inline void logDebug(std::string&& message) {
    Logger* logger = detail::moduleLogger();
    constexpr int level = LOG_LEVEL_DEBUG;
    if (shouldLogLevelFor(logger, level)) {
        logRawStringFor(logger, level, std::move(message));
    }
}

inline void logInfo(std::string&& message) {
    Logger* logger = detail::moduleLogger();
    constexpr int level = LOG_LEVEL_INFO;
    if (shouldLogLevelFor(logger, level)) {
        logRawStringFor(logger, level, std::move(message));
    }
}

inline void logWarn(std::string&& message) {
    Logger* logger = detail::moduleLogger();
    constexpr int level = LOG_LEVEL_WARN;
    if (shouldLogLevelFor(logger, level)) {
        logRawStringFor(logger, level, std::move(message));
    }
}

inline void logError(std::string&& message) {
    Logger* logger = detail::moduleLogger();
    constexpr int level = LOG_LEVEL_ERROR;
    if (shouldLogLevelFor(logger, level)) {
        logRawStringFor(logger, level, std::move(message));
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
#define LOG_SET_DEFAULT_LOGGER(loggerName) \
    do { \
        Common::detail::setModuleLoggerName(loggerName); \
        Common::setDefaultLoggerName(loggerName); \
    } while (0)

#define LOG_INIT(logFile, level, maxSize, maxFiles) \
    Common::logInitFor(Common::detail::moduleLogger(), logFile, level, maxSize, maxFiles)

#define LOG_INIT_WITH_MODE(logFile, level, maxSize, maxFiles, fileMode) \
    Common::logInitFor(Common::detail::moduleLogger(), logFile, level, maxSize, maxFiles, fileMode)

#define LOG_START(toConsole) \
    Common::logStartFor(Common::detail::moduleLogger(), toConsole)

#define LOG_STOP() \
    Common::logStopFor(Common::detail::moduleLogger())

#define LOG_SOFTWARE_INFO(softwareName, version, author, platform) \
    Common::logSoftwareInfoFor(Common::detail::moduleLogger(), softwareName, version, author, platform)

#define LOG_HEX(level, prefix, data, length) \
    Common::logHexFor(Common::detail::moduleLogger(), level, prefix, data, length)

#define LOG_DEBUG_HEX(prefix, data, length) \
    Common::logHexFor(Common::detail::moduleLogger(), LOG_LEVEL_DEBUG, prefix, data, length)

// High-performance logging macros
#define LOG_DEBUG(fmt, ...) Common::logDebug(FMT_STRING(fmt), ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  Common::logInfo(FMT_STRING(fmt), ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Common::logWarn(FMT_STRING(fmt), ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Common::logError(FMT_STRING(fmt), ##__VA_ARGS__)

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
