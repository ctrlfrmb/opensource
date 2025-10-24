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
* Version: v2.1.0
* Date: 2022-09-24
*----------------------------------------------------------------------------*/

/**
* @file sequence_sender.h
* @brief High-performance sequential data sender with tick-based timing control
*
* The SequenceSender class provides a high-performance sequential data transmission system
* that sends frames in a predetermined order. It uses a tick-based scheduling mechanism
* powered by the CallbackTimer infrastructure for precise, drift-free timing.
*
* Features:
* - Tick-based sequential frame transmission for robust timing.
* - Configurable repeat modes (fixed count or infinite loop).
* - Thread-safe frame data updates during transmission.
* - Round-end delay configuration for batch processing.
* - Automatic timer lifecycle management.
* - Callback-based completion notification.
* - Frame send counter for external monitoring.
*/

#ifndef COMMON_SEQUENCE_SENDER_H
#define COMMON_SEQUENCE_SENDER_H

#include <atomic>
#include <functional>
#include <shared_mutex>
#include <vector>
#include <memory>

#include "common_types.h"
#include "common_global.h"

namespace Common {

class CallbackTimer;

class COMMON_API_EXPORT SequenceSender {
public:
    static constexpr uint32_t DEFAULT_DELAY_TIME = 10; //ms

    // Configuration for sequence sending behavior
    struct SendConfig {
        bool is_forever{false};          // Whether to repeat forever
        uint64_t repeat_count{1};        // Number of repetitions (valid when is_forever is false)
        uint32_t round_end_delay{10};     // Delay after each round completion (in milliseconds, i.e., ticks)
    };

    explicit SequenceSender();
    ~SequenceSender();

    // Disable copy constructor and assignment operator
    SequenceSender(const SequenceSender&) = delete;
    SequenceSender& operator=(const SequenceSender&) = delete;

    void setSendCallback(SendCallback callback);
    void setCompletionCallback(std::function<void(int)> callback);
    void setConfig(bool isForever, uint64_t repeatCount, uint32_t roundEndDelay = 0);

    int start(const SendQueue& sendQueue);
    int start(SendQueue&& sendQueue);
    void stop();

    int updateData(uint64_t key, const std::vector<char>& data);
    bool isRunning() const { return is_running_.load(std::memory_order_acquire); }

private:
    int onTimerTick(uint64_t counter);
    int sendCurrentFrame();
    int handleCompletion(int exitCode); // must be return -1;
    void clearFrameData();

private:
    mutable std::mutex mutex_;

    // Timer management
    std::unique_ptr<CallbackTimer> timer_;

    // State management
    std::atomic_bool is_running_{false};

    // Configuration
    SendConfig config_;

    // Callbacks
    SendCallback send_callback_{nullptr};
    std::function<void(int)> completion_callback_{nullptr};

    // Frame management
    mutable std::shared_mutex frames_mutex_;
    SendQueue frames_;
    size_t total_frames_{0};

    // Current execution state (only accessed in timer thread)
    uint64_t current_round_{0};
    size_t current_frame_index_{0};

    // Tick-based scheduling state
    uint64_t current_tick_{0};      // Stores the latest tick from the timer callback
    uint64_t next_send_tick_{0};    // The target tick for the next send operation
};

}

#endif // COMMON_SEQUENCE_SENDER_H
