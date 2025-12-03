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
* Version: v1.0.4
* Date: 2022-10-21
*----------------------------------------------------------------------------*/

/**
* @file advanced_logger.h
* @brief A high-performance, asynchronous, and configurable data recorder C++ class.
*
* The AdvancedLogger class provides a flexible and efficient way to record
* data to files. It operates asynchronously, using a lock-free queue to minimize
* application latency. It is designed for high-throughput scenarios where data
* integrity and performance are critical.
*
* Features:
* - Asynchronous logging with a dedicated writer thread.
* - High-performance lock-free queue (moodycamel::ConcurrentQueue).
* - Configurable file rotation modes: INCREMENTING and ROLLING.
* - Customizable file naming patterns, including timestamps and indices.
* - Command-line style string for easy and extensible configuration.
* - Optimized interfaces for logging plain text, raw data, and hexadecimal data.
* - Batch writing and custom stream buffering for superior I/O performance.
* - Provides an interface to retrieve the last error message for diagnostics.
*
* C++ Usage Example:
*   Common::AdvancedLogger logger;
*   if (logger.setConfig("--baseFileName UDS_Log --logDir ./logs") != 0) {
*       std::cerr << "Config failed: " << logger.getLastError() << std::endl;
*       return;
*   }
*   if (logger.start() != 0) {
*       std::cerr << "Start failed: " << logger.getLastError() << std::endl;
*       return;
*   }
*   logger.log("This is a formatted message.");
*   logger.stop();
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
    enum class LogRotationMode { INCREMENTING, ROLLING };
    enum class FileNamePattern { BASE_TIME_INDEX_EXT, BASE_INDEX_EXT, BASE_EXT };

    AdvancedLogger();
    ~AdvancedLogger();

    AdvancedLogger(const AdvancedLogger&) = delete;
    AdvancedLogger& operator=(const AdvancedLogger&) = delete;

    int setConfig(const std::string& commands);
    int start();
    void stop();
    std::string getCurrentLogPath() const;

    std::string getLastError() const;

    void log(const char* message);
    void logData(const char* data);
    void logRawData(std::string&& data);

    void logHex(const char* prefix, const uint8_t* data, size_t length);
    void logDataHex(const uint8_t* data, size_t length);

private:
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

    // --- State ---
    std::atomic<bool> isRunning_{false};
    std::ofstream logStream_;
    std::string currentFilePath_;
    size_t bytesWritten_{0};
    int fileIndex_{1};
    std::string timeStampForFile_;

    // --- Asynchronous Core ---
    std::thread writerThread_;
    std::mutex workerMutex_;
    std::condition_variable workerCv_;
    moodycamel::ConcurrentQueue<std::string> logQueue_;

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
