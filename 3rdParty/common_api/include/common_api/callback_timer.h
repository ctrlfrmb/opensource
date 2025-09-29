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
* Version: v3.4.0
* Date: 2022-04-19
*----------------------------------------------------------------------------*/

/**
* @file callback_timer.h
* @brief An adaptive high-precision callback timer with multiple timing strategies.
*
* This timer supports four timing strategies:
* - TIMER_STRATEGY_AUTO: Automatically selects the optimal strategy based on interval
*   (<= 1000µs uses TIMER_STRATEGY_HIGH_FREQUENCY_BUSY_WAIT, > 1000µs uses TIMER_STRATEGY_LOW_FREQUENCY)
* - TIMER_STRATEGY_LOW_FREQUENCY: Uses OS kernel-level timer for excellent efficiency
*   and drift-free millisecond precision (near-zero CPU usage)
* - TIMER_STRATEGY_HIGH_FREQUENCY_SLEEP: Uses hybrid strategy with kernel sleep + busy-wait
*   for balanced precision and efficiency
* - TIMER_STRATEGY_HIGH_FREQUENCY_BUSY_WAIT: Uses pure spin-wait for maximum precision
*   and minimal jitter at the cost of high CPU usage on one core
*/

#ifndef COMMON_CALLBACK_TIMER_H
#define COMMON_CALLBACK_TIMER_H

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

#include "common_global.h"

namespace Common {

/**
 * @brief Timer strategy enumeration
 */
typedef enum {
    TIMER_STRATEGY_AUTO = 0,                    // Auto-select based on interval
    TIMER_STRATEGY_LOW_FREQUENCY,               // Kernel timer for low frequency (> 1ms)
    TIMER_STRATEGY_HIGH_FREQUENCY_SLEEP,        // Hybrid strategy with kernel sleep + busy-wait
    TIMER_STRATEGY_HIGH_FREQUENCY_BUSY_WAIT     // Pure busy-wait for maximum precision
} TimerStrategy;

class COMMON_API_EXPORT CallbackTimer {
public:
    using TimerCallback = std::function<int(uint64_t count)>;

    CallbackTimer();
    ~CallbackTimer();

    CallbackTimer(const CallbackTimer&) = delete;
    CallbackTimer& operator=(const CallbackTimer&) = delete;

    /**
     * @brief Set timer callback function
     * @param callback Callback function to be called on each timer tick
     */
    void setCallback(TimerCallback callback);

    /**
     * @brief Set timer strategy (optional)
     * @param strategy Timer strategy to use (TIMER_STRATEGY_AUTO by default)
     */
    void setTimerStrategy(int strategy);

    /**
     * @brief Start the timer
     * @param interval_microseconds Timer interval in microseconds
     * @return true if started successfully, false otherwise
     */
    bool start(int interval_microseconds = 1000);

    /**
     * @brief Stop the timer
     */
    void stop();

    /**
     * @brief Check if timer is running
     * @return true if timer is running, false otherwise
     */
    bool isRunning() const { return is_running_.load(std::memory_order_acquire); }

    /**
     * @brief Get current timer strategy
     * @return Current timer strategy
     */
    int getTimerStrategy() const { return static_cast<int>(timer_strategy_); }

private:
    void timerThread_LowFrequency();
    void timerThread_HighFrequencySleep();
    void timerThread_HighFrequencyBusyWait();
    void optimizeThreadForTiming();
    TimerStrategy selectOptimalStrategy(int interval_us) const;

    std::atomic_bool is_running_;
    TimerCallback callback_;
    int tick_interval_us_;
    TimerStrategy timer_strategy_;

    std::thread timer_thread_;
    std::mutex thread_mutex_;

    static std::atomic<int> high_freq_timer_count_;
    int assigned_cpu_core_;
};

}

#endif // COMMON_CALLBACK_TIMER_H
