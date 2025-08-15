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

/**
 * @brief 高精度全局节拍器 (单例)
 *
 * 提供一个全局共享的、每毫T秒触发一次的定时器。
 * 其他线程可以通过等待其条件变量来与这个1ms的“心跳”同步。
 * 采用平台特定的高精度API，确保定时器在后台也能保持稳定。
 */
class COMMON_API_EXPORT PrecisionTimer {
public:
    /**
     * @brief 获取PrecisionTimer的唯一实例
     */
    static PrecisionTimer& getInstance();

    /**
     * @brief 获取从定时器启动开始经过的毫秒节拍数
     * @return 单调递增的节拍计数
     */
    uint64_t getTickCount() const { return tick_count_.load(std::memory_order_acquire); }

    /**
     * @brief 等待下一个节拍的到来
     */
    void waitNextTick();

    /**
     * @brief 等待，直到指定的节拍数到达
     * @param targetTick 目标节拍数
     */
    void waitUtilTick(uint64_t targetTick);

    /**
     * @brief 等待指定的毫秒数
     * @param milliseconds 要等待的毫秒数
     */
    void waitFor(uint64_t milliseconds = 10);

private:
    PrecisionTimer();
    ~PrecisionTimer();

    // 禁止拷贝和赋值
    PrecisionTimer(const PrecisionTimer&) = delete;
    PrecisionTimer& operator=(const PrecisionTimer&) = delete;

    void start();
    void stop();
    void timerThread();

    // 线程同步
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_bool is_running_;
    std::thread timer_thread_;
    mutable std::mutex thread_management_mutex_; // 用于保护线程启动/停止

    // 节拍计数
    std::atomic_uint64_t tick_count_;

    // 定时器周期 (1ms)
    static constexpr int TIMER_INTERVAL_MS = 1;
};

}

#endif // COMMON_PRECISION_TIMER_H

