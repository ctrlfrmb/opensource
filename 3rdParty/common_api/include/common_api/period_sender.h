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
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v3.0.0
* Date: 2022-10-15
*----------------------------------------------------------------------------*/

/**
* @file period_sender.h
* @brief High-performance periodic data sender with thread-safe frame management
*
* The PeriodSender class provides a high-performance periodic data transmission system
* that manages multiple data frames with independent timing configurations. It uses
* modern C++ concurrency features to achieve efficient thread-safe operations.
*
* Features:
* - Thread-safe frame management with std::shared_mutex for optimal read/write performance
* - Individual timing control for each frame (period and initial delay)
* - Atomic operations for execution time tracking to minimize lock contention
* - Configurable send buffer with overflow protection
* - Key-based frame identification (type + group + messageId composition)
* - Batch frame operations for efficient bulk management
* - Automatic timer lifecycle management (start/stop based on frame presence)
* - High-precision timing based on CallbackTimer infrastructure
* - Memory-efficient frame container with reserve/clear operations
* - Protocol and channel-based selective clearing capabilities
*
* Usage example:
*   Common::PeriodSender sender;
*
*   // Set send callback
*   sender.setSendCallback([](const std::vector<char>& data) {
*       // Send data via network/serial/etc
*       return 0;
*   });
*
*   // Create and add frame
*   Common::SendFrame frame;
*   frame.key = Common::PeriodSender::makeKey(0x01, 0x02, 0x1234);
*   frame.data = {0x01, 0x02, 0x03, 0x04};
*   frame.period = 100; // Send every 100ms
*
*   sender.addFrame(std::move(frame));
*
*   // Timer automatically starts and manages sending
*/

#ifndef COMMON_PERIOD_SENDER_H
#define COMMON_PERIOD_SENDER_H

#include <atomic>
#include <thread>
#include <functional>
#include <shared_mutex>
#include <map>
#include <vector>

#include "common_types.h"

namespace Common {

class CallbackTimer;

class COMMON_API_EXPORT PeriodSender {

public:
    static constexpr size_t DEFAULT_MAX_FRAMES = 0xFF;          // Default maximum of 255 messages
    static constexpr uint32_t DEFAULT_PERIOD = 100;             // Default period of 100ms
    static constexpr size_t DEFAULT_SEND_BUFFER_SIZE = 1024;    // Default 1KB buffer
    static constexpr size_t MIN_SEND_BUFFER_SIZE = 256;         // Minimum buffer size of 256 bytes
    static constexpr size_t MAX_SEND_BUFFER_SIZE = 64 * 1024;   // Maximum buffer size of 64KB
    static constexpr size_t MIN_MAX_FRAMES = 1;                 // Minimum frame count of 1
    static constexpr size_t MAX_MAX_FRAMES = 1024;              // Maximum frame count of 1024

    explicit PeriodSender();
    ~PeriodSender();

    // Disable copy and assignment for safety
    PeriodSender(const PeriodSender&) = delete;
    PeriodSender& operator=(const PeriodSender&) = delete;

    void setSendCallback(std::function<int(const std::vector<char>&)> callback);

    /**
     * @brief Set send buffer size
     * @param size Buffer size in bytes, range: [MIN_SEND_BUFFER_SIZE, MAX_SEND_BUFFER_SIZE]
     * @return true=success, false=failure (sender running or invalid parameter)
     */
    bool setSendBufferSize(size_t size);

    /**
     * @brief Get current send buffer size
     * @return Buffer size in bytes
     */
    size_t getSendBufferSize() const { return send_buffer_size_; }

    /**
     * @brief Set maximum frame count
     * @param maxFrames Maximum frame count, range: [MIN_MAX_FRAMES, MAX_MAX_FRAMES]
     * @return true=success, false=failure (sender running or invalid parameter)
     */
    bool setMaxFrames(size_t maxFrames);

    /**
     * @brief Get maximum frame count
     * @return Maximum frame count
     */
    size_t getMaxFrames() const { return max_frames_; }

    /**
     * @brief Get current frame count
     * @return Current frame count
     */
    size_t getCurrentFrameCount() const { return frame_container_.size(); }

    /**
     * @brief Get buffer usage ratio
     * @return Buffer usage ratio (0.0-1.0)
     */
    double getBufferUsageRatio() const {
        return static_cast<double>(send_buffer_.size()) / send_buffer_size_;
    }

    void resetCounter() { frame_container_.counter_.store(0, std::memory_order_release); }
    uint64_t getCounter() const { return frame_container_.counter_.load(std::memory_order_acquire); }

    // --- Add frames (rvalue reference versions, for temporary objects) ---
    int addFrame(SendFrame&& frame);
    int addFrames(SendQueue&& frames);
    int updateData(uint64_t key, std::vector<char>&& data);

    // --- Add frames (lvalue reference versions, for existing objects) ---
    int addFrame(const SendFrame& frame);
    int addFrames(const SendQueue& frames);
    int updateData(uint64_t key, const std::vector<char>& data);

    // --- Delete operations ---
    bool removeFrame(uint64_t key);
    int clear();
    int clear(uint16_t type);
    int clear(uint16_t type, uint16_t group);

private:
    // Frame management container
    struct FrameContainer {
        mutable std::shared_mutex mutex_;
        std::atomic_uint64_t counter_{0};
        std::map<uint64_t, SendFrame> frames_;                     // Original frame data (using map for stability)
        std::map<uint64_t, std::atomic<uint64_t>> next_exec_times_; // Next execution timestamp - using std::map to avoid atomic move issues

        void clear() {
            std::unique_lock<std::shared_mutex> lock(mutex_);  // Write lock
            frames_.clear();
            next_exec_times_.clear();
        }

        size_t size() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);  // Read lock
            return frames_.size();
        }

        bool empty() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);  // Read lock
            return frames_.empty();
        }
    };

    // Check if configuration can be modified (not running)
    bool canModifyConfig() const;

    // Thread function
    int senderLoop();  // Unified sender thread

    // Frame processing
    bool addToContainer(SendFrame&& frame);
    bool addToContainer(const SendFrame& frame);
    int updateFrameInContainer(uint64_t key, const std::vector<char>& data);
    bool removeFromContainer(uint64_t key);
    int clearContainer(uint16_t type, int group = -1);

private:
    std::unique_ptr<CallbackTimer> timer_;
    std::function<int(const std::vector<char>&)> send_callback_;
    FrameContainer frame_container_;         // Unified frame container
    std::vector<char> send_buffer_;

    // Configuration parameters
    size_t send_buffer_size_;                 // Send buffer size
    size_t max_frames_;                      // Maximum frame count
};

}

#endif // COMMON_PERIOD_SENDER_H
