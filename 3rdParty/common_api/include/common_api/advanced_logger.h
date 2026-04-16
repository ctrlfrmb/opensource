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
* Date: 2022-10-21
*----------------------------------------------------------------------------*/

/**
* @file advanced_logger.h
* @brief A high-performance, asynchronous, and configurable data recorder C++ class.
*
* The AdvancedLogger class provides a flexible and efficient way to record
* data to files. It operates asynchronously, using a dedicated writer thread
* to minimize application latency. It is designed for high-throughput scenarios
* where data integrity and performance are critical.
*
* Supports two write modes:
*
* - **ASYNC** (default): Uses a lock-free concurrent queue (moodycamel::ConcurrentQueue).
*   Provides maximum throughput for high-frequency multi-threaded logging.
*   Note: Does NOT guarantee strict global FIFO order across different producer
*   threads, as the lock-free queue uses per-thread sub-queues internally.
*   Order within a single producer thread IS guaranteed.
*
* - **ORDERED**: Uses a mutex-protected std::vector with O(1) swap draining.
*   Guarantees strict global FIFO order across all producer threads.
*   Suitable for scenarios where log ordering is critical (e.g., UDS diagnostic
*   protocol logging, transaction tracing). The mutex overhead is negligible
*   for low-to-moderate throughput scenarios (< 10,000 entries/sec).
*
* Other Features:
* - Configurable file rotation modes: INCREMENTING and ROLLING.
* - Customizable file naming patterns, including timestamps and indices.
* - Command-line style string for easy and extensible configuration.
* - Optimized interfaces for logging plain text, raw data, and hexadecimal data.
* - Batch writing and custom stream buffering for superior I/O performance.
* - Provides an interface to retrieve the last error message for diagnostics.
*
* C++ Usage Example:
* @code
*   // High-throughput mode (default ASYNC)
*   Common::AdvancedLogger logger;
*   logger.setConfig("--baseFileName app_log --logDir ./logs");
*   logger.start();
*   logger.log("This is a high-throughput message.");
*   logger.stop();
*
*   // Ordered mode for protocol logging
*   Common::AdvancedLogger orderedLogger;
*   orderedLogger.setConfig("--baseFileName uds_trace --logDir ./logs --writeMode ORDERED");
*   orderedLogger.start();
*   // Multiple threads can call logRawData — output is strictly FIFO ordered.
*   orderedLogger.logRawData("2025-01-01 00:00:00.001 TX 0x723 ...\n");
*   orderedLogger.stop();
* @endcode
*/

#ifndef COMMON_ADVANCED_LOGGER_H
#define COMMON_ADVANCED_LOGGER_H
#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <fstream>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "concurrentqueue.h"
#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT AdvancedLogger {
public:
    /**
     * @brief Defines how log entries are queued internally.
     *
     * - ASYNC:   Lock-free concurrent queue. Maximum throughput.
     *            Does NOT guarantee cross-thread ordering.
     * - ORDERED: Mutex-protected queue with O(1) swap drain.
     *            Guarantees strict global FIFO across all threads.
     */
    enum class WriteMode { ASYNC, ORDERED };

    enum class LogRotationMode { INCREMENTING, ROLLING };
    enum class FileNamePattern { BASE_TIME_INDEX_EXT, BASE_INDEX_EXT, BASE_EXT };

    AdvancedLogger();
    ~AdvancedLogger();

    AdvancedLogger(const AdvancedLogger&) = delete;
    AdvancedLogger& operator=(const AdvancedLogger&) = delete;

    /**
     * @brief Configures the logger using a command-line style string.
     *
     * Supported keys:
     *   --logDir <path>           Log file directory (default: ".")
     *   --baseFileName <name>     Base name for log files (default: "app_log")
     *   --fileExtension <ext>     File extension (default: ".log")
     *   --maxSizeMB <size>        Max file size in MB before rotation (default: 10)
     *   --maxFiles <count>        Max number of rotated files (default: 10)
     *   --logTag <tag>            Optional tag inserted into each log() call
     *   --rotationMode <mode>     INCREMENTING (default) or ROLLING
     *   --namePattern <pattern>   BASE_TIME_INDEX_EXT (default), BASE_INDEX_EXT, or BASE_EXT
     *   --writeMode <mode>        ASYNC (default) or ORDERED
     *
     * @param commands The configuration string.
     * @return 0 on success, a negative value on failure.
     */
    int setConfig(const std::string& commands);

    /**
     * @brief Starts the logger and its writer thread.
     * @return 0 on success, a negative value on failure.
     */
    int start();

    /**
     * @brief Stops the logger, flushes all remaining data, and joins the writer thread.
     */
    void stop();

    /**
     * @brief Returns the full path of the currently active log file.
     */
    std::string getCurrentLogPath() const;

    /**
     * @brief Returns the last error message for diagnostics.
     */
    std::string getLastError() const;

    /**
     * @brief Logs a message with an auto-generated timestamp prefix and optional tag.
     * Format: "<timestamp><tag><message>\n"
     * @param message The message to log.
     */
    void log(const char* message);

    /**
     * @brief Logs a raw C-string as-is (no timestamp, no newline added).
     * @param data The data string to log.
     */
    void logData(const char* data);

    /**
     * @brief Logs a pre-formatted string by move (no timestamp, no newline added).
     * This is the most efficient interface — avoids copying entirely.
     * @param data The data string to log (moved).
     */
    void logRawData(std::string&& data);

    /**
     * @brief Logs binary data as uppercase hex with a timestamp and prefix.
     * Format: "<timestamp> <prefix> <hex_string>\n"
     * @param prefix A label such as "TX" or "RX".
     * @param data Pointer to the binary data.
     * @param length Number of bytes to log.
     */
    void logHex(const char* prefix, const uint8_t* data, size_t length);

    /**
     * @brief Logs binary data as uppercase hex only (no timestamp, no prefix).
     * @param data Pointer to the binary data.
     * @param length Number of bytes to log.
     */
    void logDataHex(const uint8_t* data, size_t length);

private:
    /**
     * @brief Internal unified enqueue method. Dispatches to the active queue
     * based on the configured WriteMode.
     * @param item The log entry to enqueue (moved).
     */
    void enqueueItem(std::string&& item);

    /**
     * @brief The writer thread function. Periodically drains the active queue
     * and writes batched data to the log file.
     */
    void writerThreadFunc();

    void openNewFile();
    void handleRotation();
    void performIncrementingRotation();
    void performRollingRotation();
    std::string generateFilePath();
    int parseConfig(const std::string& commands);
    std::string getCurrentTimestampForFile();

    // --- Configuration ---
    std::string logDir_{"."};
    std::string baseFileName_{"app_log"};
    std::string fileExtension_{".log"};
    size_t maxFileSize_{10 * 1024 * 1024};
    int maxFiles_{10};
    LogRotationMode rotationMode_{LogRotationMode::INCREMENTING};
    FileNamePattern namePattern_{FileNamePattern::BASE_TIME_INDEX_EXT};
    std::string logTag_{""};
    WriteMode writeMode_{WriteMode::ASYNC};

    // --- State ---
    std::atomic<bool> isRunning_{false};
    std::ofstream logStream_;
    std::string currentFilePath_;
    size_t bytesWritten_{0};
    int fileIndex_{1};
    std::string timeStampForFile_;

    // --- Writer Thread ---
    std::thread writerThread_;
    std::mutex workerMutex_;
    std::condition_variable workerCv_;

    // --- ASYNC Mode: Lock-free concurrent queue ---
    // Used only when writeMode_ == ASYNC.
    // High throughput, but does not guarantee cross-thread FIFO ordering.
    moodycamel::ConcurrentQueue<std::string> asyncQueue_;

    // --- ORDERED Mode: Mutex-protected vector ---
    // Used only when writeMode_ == ORDERED.
    // Strict global FIFO. Protected by orderedMutex_.
    // Writer thread drains via O(1) swap, so the lock is held only briefly.
    std::vector<std::string> orderedQueue_;
    std::mutex orderedMutex_;
    std::condition_variable orderedCv_;

    // --- Performance Optimization ---
    std::vector<std::string> localBatch_;
    std::string batchBuffer_;
    std::vector<char> fileStreamBuffer_;

    std::string lastError_;

    // --- Constants ---
    static constexpr size_t LOG_QUEUE_SIZE = 20000;
    static constexpr size_t BATCH_BUFFER_RESERVE_BYTES = 256 * 1024;
    static constexpr size_t FILE_STREAM_BUFFER_BYTES = 256 * 1024;
    static constexpr int MAX_ROLLING_FILES = 100;
    static constexpr int WRITER_LOOP_INTERVAL_MS = 100;
};

} // namespace Common

#endif // COMMON_ADVANCED_LOGGER_H
