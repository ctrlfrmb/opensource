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
* Version: v1.0.0
* Date: 2022-08-15
*----------------------------------------------------------------------------*/

/**
* @file callback_timer.h
* @brief High-precision callback timer with configurable intervals
*
* The CallbackTimer class provides a high-precision timer that executes a single
* callback function at configurable intervals. This design follows the principle
* of single responsibility - one timer, one task.
*
* Features:
* - Configurable timing intervals (milliseconds to microseconds precision)
* - Single callback function per timer instance
* - Cross-platform implementation (Windows/Unix)
* - Platform-specific optimizations for maximum precision
* - Hybrid approach with sleep + spin wait for sub-millisecond accuracy
* - Thread-safe callback management
* - Individual start/stop control per timer instance
* - Memory-order guarantees for thread synchronization
* - Automatic cleanup on destruction
*
* Usage example:
*   Common::CallbackTimer timer;
*
*   // Set callback function
*   timer.setCallback([]() {
*       // High-frequency task
*   });
*
*   // Start timer with 500 microsecond interval
*   timer.start(500);
*
*   // Later when done
*   timer.stop();
*/

#ifndef COMMON_CALLBACK_TIMER_H
#define COMMON_CALLBACK_TIMER_H

#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT CallbackTimer {
public:
    using TimerCallback = std::function<void()>;

    // Default constructor
    CallbackTimer();

    // Destructor ensures proper cleanup
    ~CallbackTimer();

    // Disable copy constructor and assignment operator
    CallbackTimer(const CallbackTimer&) = delete;
    CallbackTimer& operator=(const CallbackTimer&) = delete;

    /**
     * @brief Set the callback function
     * @param callback Function to be executed periodically
     */
    void setCallback(TimerCallback callback);

    /**
     * @brief Start the timer with specified interval
     * @param interval_microseconds Timer interval in microseconds (default: 1000µs = 1ms)
     * @return true if timer started successfully, false if already running or no callback set
     */
    bool start(uint64_t interval_microseconds = 1000);

    /**
     * @brief Stop the timer
     */
    void stop();

    /**
     * @brief Check if timer is currently running
     * @return true if timer is running, false otherwise
     */
    bool isRunning() const { return is_running_.load(std::memory_order_acquire); }

    uint64_t getTickCount() const { return tick_count_.load(std::memory_order_acquire); }

private:
    void timerThread();

    // Thread synchronization
    std::atomic_bool is_running_;
    // Callback function
    TimerCallback callback_;
    // Timing control
    std::chrono::microseconds tick_interval_;
    std::atomic_uint64_t tick_count_;

    // Timer thread
    mutable std::mutex thread_mutex_;
    std::thread timer_thread_;

    // Platform-specific constants
    static constexpr uint8_t BASE_OFFSET = 100; // microseconds for spin-wait threshold
};

}

#endif // COMMON_CALLBACK_TIMER_H
