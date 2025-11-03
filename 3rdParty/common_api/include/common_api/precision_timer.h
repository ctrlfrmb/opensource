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
* Version: v1.1.2
* Date: 2022-05-29
*----------------------------------------------------------------------------*/

/**
* @file precision_timer.h
* @brief High-precision cross-platform timer with millisecond accuracy
*
* The PrecisionTimer class implements a singleton high-resolution timer that provides
* accurate timing services for the application. It uses a hybrid sleep/spin-wait
* strategy to achieve consistent millisecond-level timing precision with low CPU usage
* on both Windows and Unix-like systems.
*
* Features:
* - Singleton design pattern for application-wide timing coordination
* - Millisecond precision with minimal drift and jitter
* - Cross-platform implementation (Windows/Unix)
* - Hybrid approach (kernel sleep + spin-wait) for balanced precision and efficiency
* - Thread synchronization through condition variables
* - Monotonically increasing tick counter for timing reference
*
* Usage example:
*   auto& timer = Common::PrecisionTimer::getInstance();
*
*   // Wait for a specific duration
*   timer.waitFor(100);  // Wait for 100ms
*
*   // Wait until a specific tick count is reached
*   uint64_t targetTick = timer.getTickCount() + 50;
*   timer.waitUtilTick(targetTick);
*/
#ifndef COMMON_PRECISION_TIMER_H
#define COMMON_PRECISION_TIMER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "common_global.h"

namespace Common {

/**
 * @brief High-precision global timer (singleton)
 *
 * Provides a globally shared timer that triggers every millisecond.
 * Other threads can synchronize with this 1ms "heartbeat" by waiting on its condition variable.
 * Uses a platform-optimized hybrid strategy to ensure the timer remains stable and accurate.
 */
class COMMON_API_EXPORT PrecisionTimer {
public:
    /**
     * @brief Get the singleton instance of PrecisionTimer
     * @return Reference to the global PrecisionTimer instance
     */
    static PrecisionTimer& getInstance();

    void start();
    void stop();

    /**
     * @brief Get the number of millisecond ticks since timer start
     * @return Monotonically increasing tick count
     */
    uint64_t getTickCount() const { return tick_count_.load(std::memory_order_acquire); }

    /**
     * @brief Wait for a specified number of milliseconds
     * @param milliseconds Number of milliseconds to wait
     */
    void waitFor(uint64_t milliseconds = 10);

    /**
     * @brief Wait for the next tick to arrive
     */
    void waitNextTick();

    /**
     * @brief Wait until a specific tick count is reached
     * @param targetTick Target tick count to wait for
     */
    void waitUtilTick(uint64_t targetTick);

private:
    PrecisionTimer();
    ~PrecisionTimer();

    // Prevent copying and assignment
    PrecisionTimer(const PrecisionTimer&) = delete;
    PrecisionTimer& operator=(const PrecisionTimer&) = delete;

    void timerThread();

    // Thread synchronization
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_bool is_running_;
    std::thread timer_thread_;
    mutable std::mutex thread_mutex_;

    // Tick counter
    std::atomic_uint64_t tick_count_;

    // Timer period (1ms)
    static constexpr int TIMER_INTERVAL_MS = 1;
};

} // namespace Common

#endif // COMMON_PRECISION_TIMER_H
