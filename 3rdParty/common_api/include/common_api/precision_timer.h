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
* Version: v1.1.2
* Date: 2022-05-29
*----------------------------------------------------------------------------*/

/**
* @file precision_timer.h
* @brief High-precision cross-platform timer with millisecond accuracy
*
* The PrecisionTimer class implements a singleton high-resolution timer that provides
* accurate timing services for the application. It uses platform-specific optimizations
* to achieve consistent millisecond-level timing precision on both Windows and Unix systems.
*
* Features:
* - Singleton design pattern for application-wide timing coordination
* - Millisecond precision with minimal drift
* - Cross-platform implementation (Windows/Unix)
* - Platform-specific optimizations (Windows multimedia timers, POSIX clock_nanosleep)
* - Hybrid approach with sleep + spin wait for maximum precision
* - Thread synchronization through condition variables
* - Monotonic tick counter for timing reference
* - Multiple wait mechanisms for different timing needs
*
* Usage example:
*   auto& timer = Common::PrecisionTimer::getInstance();
*
*   // Wait for specific duration
*   timer.waitFor(100);  // Wait for 100ms
*
*   // Wait until specific tick count
*   uint64_t targetTick = timer.getTickCount() + 50;
*   timer.waitNextTick(targetTick);
*/

#ifndef COMMON_PRECISION_TIMER_H
#define COMMON_PRECISION_TIMER_H

#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT PrecisionTimer {
public:
    static PrecisionTimer& getInstance();

    void wait();

    void waitNextTick(uint64_t targetTick);
    bool waitFor(uint64_t milliseconds = 10);

    uint64_t getTickCount() const { return tick_count_.load(std::memory_order_relaxed); }

private:
    PrecisionTimer();
    ~PrecisionTimer();

    PrecisionTimer(const PrecisionTimer&) = delete;
    PrecisionTimer& operator=(const PrecisionTimer&) = delete;

    void start();
    void stop();
    void timerThread();

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> is_running_;
    std::chrono::steady_clock::time_point next_tick_;
    const std::chrono::microseconds tick_interval_;
    std::atomic<uint64_t> tick_count_{0}; //ms
    static constexpr int BASE_OFFSET{100}; //microsecond
};

}

#endif // COMMON_PRECISION_TIMER_H
