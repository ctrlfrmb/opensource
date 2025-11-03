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
* Version: v3.5.0
* Date: 2022-05-22
*----------------------------------------------------------------------------*/

/**
* @file callback_timer.h
* @brief An adaptive high-precision callback timer with multiple timing strategies.
*
* This timer supports four timing strategies:
* - TIMER_STRATEGY_AUTO: Automatically selects the optimal strategy based on interval.
*   (<= 5000µs uses TIMER_STRATEGY_HIGH_FREQUENCY_BUSY_WAIT, > 5000µs uses TIMER_STRATEGY_LOW_FREQUENCY)
* - TIMER_STRATEGY_LOW_FREQUENCY: Uses OS kernel-level timer for excellent efficiency
*   and drift-free millisecond precision (near-zero CPU usage).
* - TIMER_STRATEGY_HIGH_FREQUENCY_SLEEP: Uses a hybrid strategy with kernel sleep + busy-wait
*   for balanced precision and efficiency.
* - TIMER_STRATEGY_HIGH_FREQUENCY_BUSY_WAIT: Uses pure spin-wait for maximum precision
*   and minimal jitter at the cost of high CPU usage on one core.
*
* For high-frequency strategies, CPU core affinity can be enabled to further reduce
* scheduling jitter and improve timing accuracy.
*/

#ifndef COMMON_CALLBACK_TIMER_H
#define COMMON_CALLBACK_TIMER_H

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

#include "common_global.h"
#include "common_types.h"

namespace Common {

class COMMON_API_EXPORT CallbackTimer {
public:
    using TimerCallback = std::function<int(uint64_t count)>;

    CallbackTimer();
    ~CallbackTimer();

    CallbackTimer(const CallbackTimer&) = delete;
    CallbackTimer& operator=(const CallbackTimer&) = delete;

    /**
     * @brief Sets the callback function to be executed on each timer tick.
     * @param callback The function to be called. It receives the current tick count
     *                 and should return 0 to continue, or non-zero to stop the timer.
     */
    void setCallback(TimerCallback callback);

    /**
     * @brief Sets the timing strategy for the timer.
     * @param strategy The desired timing strategy (e.g., TIMER_STRATEGY_AUTO).
     *                 This must be set before calling start().
     */
    void setTimerStrategy(int strategy);

    /**
     * @brief Enables or disables CPU core affinity for high-frequency timer strategies.
     *
     * When enabled, the high-frequency timer thread will be bound to the least
     * busy CPU core to minimize scheduling jitter. This has no effect on the
     * low-frequency strategy.
     *
     * @param enable true to enable CPU affinity, false to disable. Default is false.
     *               This must be set before calling start().
     */
    void enableCpuAffinity(bool enable);

    /**
     * @brief Starts the timer with a specified interval.
     * @param interval_microseconds The timer interval in microseconds.
     * @return true if the timer started successfully, false otherwise.
     */
    bool start(int interval_microseconds = 1000);

    /**
     * @brief Stops the timer and joins the timer thread.
     */
    void stop();

    /**
     * @brief Checks if the timer is currently running.
     * @return true if the timer is running, false otherwise.
     */
    bool isRunning() const { return is_running_.load(std::memory_order_acquire); }

    /**
     * @brief Gets the current timer strategy.
     * @return The integer value of the current timer strategy.
     */
    int getTimerStrategy() const { return static_cast<int>(timer_strategy_); }

private:
    void timerThread_LowFrequency();
    void timerThread_HighFrequencySleep();
    void timerThread_HighFrequencyBusyWait();
    TimerStrategy selectOptimalStrategy(int interval_us) const;

    std::atomic_bool is_running_;
    TimerCallback callback_;
    int tick_interval_us_;
    TimerStrategy timer_strategy_;
    bool cpu_affinity_enabled_;

    std::thread timer_thread_;
    std::mutex thread_mutex_;
};

} // namespace Common

#endif // COMMON_CALLBACK_TIMER_H
