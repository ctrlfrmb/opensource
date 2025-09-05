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
* Version: v1.1.0
* Date: 2022-09-20
*----------------------------------------------------------------------------*/

/**
* @file sequence_sender.h
* @brief High-performance sequential data sender with configurable timing and repeat control
*
* The SequenceSender class provides a high-performance sequential data transmission system
* that sends frames in a predetermined order with individual timing controls. It uses
* the CallbackTimer infrastructure for precise timing and supports flexible repeat configurations.
*
* Features:
* - Sequential frame transmission with individual interval control
* - Configurable repeat modes (fixed count or infinite loop)
* - Thread-safe frame data updates during transmission
* - Round-end delay configuration for batch processing
* - Automatic timer lifecycle management
* - High-precision timing based on CallbackTimer infrastructure
* - Memory-efficient frame management with move semantics
* - Callback-based completion notification
* - Individual frame send failure handling
*
* Usage example:
*   Common::SequenceSender sender;
*
*   // Set send callback
*   sender.setSendCallback([](const std::vector<char>& data) {
*       // Send data via network/serial/etc
*       return 0;
*   });
*
*   // Set completion callback
*   sender.setCompletionCallback([](int exitCode) {
*       // Handle completion
*   });
*
*   // Configure repeat behavior
*   sender.setConfig(false, 5, 1000); // 5 rounds, 1s delay between rounds
*
*   // Start sequence
*   Common::SendQueue frames = {...};
*   sender.start(std::move(frames));
*/

#ifndef COMMON_SEQUENCE_SENDER_H
#define COMMON_SEQUENCE_SENDER_H

#include <atomic>
#include <functional>
#include <shared_mutex>
#include <vector>
#include <memory>

#include "common_types.h"

namespace Common {

class CallbackTimer;

class COMMON_API_EXPORT SequenceSender {
public:
    // Configuration for sequence sending behavior
    struct SendConfig {
        bool is_forever{false};          // Whether to repeat forever
        uint64_t repeat_count{1};        // Number of repetitions (valid when is_forever is false)
        uint32_t round_end_delay{10};     // Delay after each round completion (ms)
    };

    explicit SequenceSender();
    ~SequenceSender();

    // Disable copy constructor and assignment operator
    SequenceSender(const SequenceSender&) = delete;
    SequenceSender& operator=(const SequenceSender&) = delete;

    /**
     * @brief Set the send callback function
     * @param callback Function to send frame data
     */
    void setSendCallback(std::function<int(const std::vector<char>&)> callback);

    /**
     * @brief Set the completion callback function
     * @param callback Function called when sequence completes or stops
     */
    void setCompletionCallback(std::function<void(int)> callback);

    /**
     * @brief Configure sending behavior
     * @param isForever Whether to repeat forever
     * @param repeatCount Number of repetitions (ignored if isForever is true)
     * @param roundEndDelay Delay between rounds in milliseconds
     */
    void setConfig(bool isForever, uint64_t repeatCount, uint32_t roundEndDelay = 0);

    /**
     * @brief Start sequential sending
     * @param sendQueue Queue of frames to send sequentially
     * @return 0 on success, negative on error
     */
    int start(const SendQueue& sendQueue);
    int start(SendQueue&& sendQueue);

    /**
     * @brief Stop the sequential sending
     */
    void stop();

    /**
     * @brief Update data for frames with specified key
     * @param key Frame key to match
     * @param data New data to set
     * @return Number of frames updated, -1 on error
     */
    int updateData(uint64_t key, const std::vector<char>& data);

    /**
     * @brief Check if sender is currently running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return is_running_.load(std::memory_order_acquire); }

private:
    int onTimerTick();
    int sendCurrentFrame();
    int handleCompletion(int exitCode);
    void clearFrameData();
    uint64_t getCurrentTime() const;

private:
    mutable std::mutex mutex_;

    // Timer management
    std::unique_ptr<CallbackTimer> timer_;

    // State management - Separate running state and stop signal
    std::atomic_bool is_running_{false};     // Externally visible running state
    std::atomic_bool should_stop_{false};    // Internal stop signal

    // Configuration
    SendConfig config_;

    // Callbacks
    std::function<int(const std::vector<char>&)> send_callback_{nullptr};
    std::function<void(int)> completion_callback_{nullptr};

    // Frame management with thread safety
    mutable std::shared_mutex frames_mutex_;
    SendQueue frames_;
    size_t total_frames_{0};

    // Current execution state (only accessed in timer thread)
    uint64_t current_round_{0};
    size_t current_frame_index_{0};
    uint64_t next_send_time_{0};      // Next scheduled send timestamp
};

}

#endif // COMMON_SEQUENCE_SENDER_H
