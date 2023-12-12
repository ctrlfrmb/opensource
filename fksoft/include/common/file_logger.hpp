/**
 * @file    file_logger.hpp
 * @ingroup opensource
 * @brief   The FileLogger class provides flexible and efficient file logging capabilities for C++ applications.
 *          It supports configurable log file paths, names, and sizes, and offers both synchronous and asynchronous logging.
 *          The class ensures proper log directory management and implements automatic log file rotation based on file size,
 *          facilitating easy log maintenance and retrieval.
 * @author  leiwei
 * @date    2023.11.08
 * Copyright (c) ctrlfrmb 2023-2033
 */

#pragma once

#ifndef OPEN_SOURCE_FILE_LOGGER_HPP
#define OPEN_SOURCE_FILE_LOGGER_HPP

#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <filesystem>  // C++17 filesystem header for directory operations

namespace opensource {
    namespace ctrlfrmb {

// Structure for configuring the file logger
struct FileLoggerConfig {
    std::string filePath;      // Path to the directory where log files will be saved
    std::string fileName;      // Base name for log files
    std::string fileExtension; // Extension for log files
    size_t maxFileSize;        // Maximum size for a single log file
    uint16_t maxFileNumber;    // Maximum number of log files allowed, Default 0 means no restrictions
    bool useAsync;             // Flag to enable asynchronous logging

    FileLoggerConfig(std::string_view path, std::string_view name,
                     std::string_view extension, size_t maxSize, uint16_t maxNum = 0, bool async = false)
            : filePath(path), fileName(name), fileExtension(extension),
              maxFileSize(maxSize), maxFileNumber(maxNum), useAsync(async) {}
};

// Class responsible for logging messages to a file
class FileLogger {
public:
    // Constructor: Initializes the logger based on the given configuration
    explicit FileLogger(const FileLoggerConfig& config)
            : config(config), currentFileSize(0), stopLogging(false) {
        if (checkLogDirectory()) {
            openNewLogFile();

            if (config.useAsync) {
                // Start a separate thread for handling log queue if asynchronous mode is enabled
                workerThread = std::thread(&FileLogger::processQueue, this);
            }
        }
    }

    // Destructor: Ensures that the logging thread is properly stopped and the log file is closed
    ~FileLogger() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopLogging = true;
        }
        condition.notify_one();
        if (config.useAsync && workerThread.joinable()) {
            workerThread.join();
        }
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    // Writes a log message to the file or enqueues it for asynchronous writing
    void write(std::string_view message) {
        if (config.useAsync) {
            std::lock_guard<std::mutex> lock(queueMutex);
            logQueue.emplace(message.begin(), message.end());
            condition.notify_one();
        } else {
            std::lock_guard<std::mutex> lock(fileMutex);
            writeToLogFile(message);
        }
    }

    // Flushes the log file to ensure all messages are written
    void flush() {
        std::lock_guard<std::mutex> lock(fileMutex);
        if (logFile.is_open()) {
            logFile.flush();
        }
    }

private:
    FileLoggerConfig config;
    std::ofstream logFile;   // Output file stream for the log file
    size_t currentFileSize;  // Current size of the log file
    std::mutex fileMutex;    // Mutex for synchronizing file access
    std::queue<std::string> logQueue; // Queue for storing log messages in asynchronous mode
    std::mutex queueMutex;   // Mutex for synchronizing access to the log queue
    std::condition_variable condition; // Condition variable for notifying the logging thread
    std::thread workerThread; // Thread for asynchronous logging
    bool stopLogging;        // Flag to signal the logging thread to stop

    // Checks if the log directory exists and creates it if necessary
    bool checkLogDirectory() const {
        std::filesystem::path dir(config.filePath);
        if (std::filesystem::exists(dir) || std::filesystem::create_directories(dir)) {
            return true;
        }
        std::cerr << "Failed to create log directory: " << config.filePath << std::endl;
        return false;
    }

    // Opens a new log file with a timestamped name
    void openNewLogFile() {
        if (config.maxFileNumber > 0) {
            manageLogFileCount();
        }

        if (logFile.is_open()) {
            logFile.close();
        }

        std::string newFileName = createLogFileName();
        logFile.open(newFileName, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Failed to open log file: " << newFileName << std::endl;
        }
        currentFileSize = 0;
    }

    // Manages the number of log files in the directory
    void manageLogFileCount() {
        auto files = getLogFiles();
        if (files.size() >= config.maxFileNumber) {
            std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
                return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
            });
            // Remove the oldest file(s) to maintain the file count limit
            while (files.size() >= config.maxFileNumber) {
                std::filesystem::remove(files.front());
                files.erase(files.begin());
            }
        }
    }

    // Gets a list of all log files in the directory
    std::vector<std::filesystem::path> getLogFiles() const {
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(config.filePath)) {
            if (entry.path().extension() == config.fileExtension) {
                files.push_back(entry.path());
            }
        }
        return files;
    }

    // Generates a filename for the log file based on the current timestamp
    std::string createLogFileName() const {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << config.filePath << "/" << config.fileName;
        ss << std::put_time(std::localtime(&now_c), "%Y%m%d%H%M%S");
        ss << milliseconds.count() << config.fileExtension;

        return ss.str();
    }

    // Processes the log queue in asynchronous mode
    void processQueue() {
        while (true) {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !logQueue.empty() || stopLogging; });

            if (stopLogging && logQueue.empty()) {
                break;
            }

            auto message = logQueue.front();
            logQueue.pop();
            lock.unlock();

            writeToLogFile(message);
        }
    }

    // Writes a log message to the file
    void writeToLogFile(std::string_view message) {
        std::unique_lock<std::mutex> lock(fileMutex);
        if (logFile.is_open()) {
            logFile << message << std::endl;
            currentFileSize += message.size() + 1;

            if (currentFileSize >= config.maxFileSize) {
                openNewLogFile();
            }
        }
    }
};

}  // namespace ctrlfrmb
}  // namespace opensource

#endif // !OPEN_SOURCE_FILE_LOGGER_HPP
