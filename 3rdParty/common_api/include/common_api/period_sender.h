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
* Version: v5.0.0
* Date: 2022-09-24
*----------------------------------------------------------------------------*/

/**
* @file period_sender.h
* @brief High-performance periodic data sender with precise phase-offset control.
*
* The PeriodSender class provides a high-performance periodic data transmission system.
* It uses a single, high-precision timer and a tick-based scheduling model to manage
* multiple data frames with independent periods and phase-offset delays, preventing
* message bunching and ensuring accurate timing, even for frames added dynamically.
*
* Features:
* - Well-encapsulated, thread-safe frame management with std::shared_mutex.
* - Precise timing control for each frame, including period and phase-offset delay.
* - Correctly handles dynamically added frames by converting relative delays to absolute target ticks.
* - Single-timer architecture for simplicity, efficiency, and correctness.
* - Tick-based counter for drift-free periodic transmission.
* - Configurable send buffer with overflow protection.
* - Key-based frame identification.
* - Batch frame operations for efficient bulk management.
* - Automatic timer lifecycle management.
*/

#ifndef COMMON_PERIOD_SENDER_H
#define COMMON_PERIOD_SENDER_H

#include <map>
#include <mutex>
#include <shared_mutex>

#include "common_types.h"
#include "common_global.h"

namespace Common {

class CallbackTimer;

class COMMON_API_EXPORT PeriodSender {
public:
    static constexpr size_t DEFAULT_MAX_FRAMES = 0xFF;
    static constexpr uint32_t DEFAULT_PERIOD_TIME = 100; //ms
    static constexpr size_t DEFAULT_SEND_BUFFER_SIZE = 1024;
    static constexpr size_t MIN_SEND_BUFFER_SIZE = 256;
    static constexpr size_t MAX_SEND_BUFFER_SIZE = 64 * 1024;
    static constexpr size_t MIN_MAX_FRAMES = 1;
    static constexpr size_t MAX_MAX_FRAMES = 1024;

    explicit PeriodSender();
    ~PeriodSender();

    PeriodSender(const PeriodSender&) = delete;
    PeriodSender& operator=(const PeriodSender&) = delete;

    void setSendCallback(SendCallback callback);
    bool setSendBufferSize(size_t size);
    size_t getSendBufferSize() const { return send_buffer_size_; }
    bool setMaxFrames(size_t maxFrames);
    size_t getMaxFrames() const { return max_frames_; }

    void setTimerStrategy(int strategy);
    void enableCpuAffinity(bool enable);

    int addFrame(SendFrame&& frame);
    int addFrames(SendQueue&& frames);
    bool updateData(uint64_t key, std::vector<char>&& data);

    int addFrame(const SendFrame& frame);
    int addFrames(const SendQueue& frames);
    bool updateData(uint64_t key, const std::vector<char>& data);

    bool removeFrame(uint64_t key);
    int clear();
    int clear(uint16_t type);
    int clear(uint16_t type, uint16_t group);

private:
    // Encapsulated, thread-safe container for frames.
    struct FramesContainer {
        mutable std::shared_mutex mutex_;
        std::map<uint64_t, SendFrame> frames_;

        size_t size() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return frames_.size();
        }
        bool empty() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return frames_.empty();
        }
        void add(SendFrame&& frame) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            frames_[frame.key] = std::move(frame);
        }
        bool remove(uint64_t key) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            return frames_.erase(key) > 0;
        }
        bool update(uint64_t key, const std::vector<char>& data) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            auto it = frames_.find(key);
            if (it != frames_.end()) {
                it->second.data = data;
                return true;
            }
            return false;
        }
        bool update(uint64_t key, std::vector<char>&& data) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            auto it = frames_.find(key);
            if (it != frames_.end()) {
                it->second.data = std::move(data);
                return true;
            }
            return false;
        }
        void clear() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            frames_.clear();
        }
    };

    bool canModifyConfig() const;
    void startTimer();
    int senderLoop(uint64_t counter);
    int clearFrames(uint16_t type, int group);
    bool isSendTime(const SendFrame& frame, uint64_t counter) const;

private:
    mutable std::mutex api_mutex_;

    int timer_strategy_;
    bool enable_cpu_affinity_;
    std::unique_ptr<CallbackTimer> sender_timer_;
    FramesContainer frames_container_;

    std::atomic_uint64_t current_timer_counter_{0};

    SendCallback send_callback_;
    std::vector<char> send_buffer_;
    size_t send_buffer_size_;
    size_t max_frames_;
};

}

#endif // COMMON_PERIOD_SENDER_H
