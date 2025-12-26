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
* Version: v2.2.0 (Steady Clock Refactor)
* Date: 2022-03-19
*----------------------------------------------------------------------------*/

/**
* @file heartbeat_manager.h
* @brief A thread-safe idle-timeout (keep-alive) manager.
*
* The HeartbeatManager class provides a robust implementation of an idle-timeout
* mechanism. It executes a callback function only when a specified interval
* has passed without any activity. Activity is signaled by calling the
* `updateTimestamp()` method.
*
* This version uses std::chrono::steady_clock for timing, making it robust
* against system time changes.
*
* Features:
* - Idle-timeout logic for keep-alive scenarios.
* - Thread-safe implementation with atomic operations for start/stop control.
* - Configurable intervals with millisecond precision.
* - Optional startup delay.
* - Pause and resume functionality.
* - Clean shutdown with proper thread joining.
*/

#ifndef COMMON_HEARTBEAT_MANAGER_H
#define COMMON_HEARTBEAT_MANAGER_H

#include <atomic>
#include <thread>
#include <functional>
#include <condition_variable>
#include <chrono> // Added for std::chrono

#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT HeartbeatManager {
public:
  // Class constant, default heartbeat timeout (ms)
  static constexpr uint32_t DEFAULT_HEARTBEAT_TIMEOUT_MS = 1000;
  static constexpr uint32_t MIN_HEARTBEAT_TIMEOUT_MS = 5;
  static constexpr uint32_t MAX_HEARTBEAT_TIMEOUT_MS = 3600000;

  using HeartbeatCallback = std::function<void()>;

  // Default constructor, only sets timeout
  explicit HeartbeatManager(uint32_t interval = DEFAULT_HEARTBEAT_TIMEOUT_MS);

  // Disable copy constructor and assignment operator
  HeartbeatManager(const HeartbeatManager&) = delete;
  HeartbeatManager& operator=(const HeartbeatManager&) = delete;

  // Destructor, ensures thread-safe shutdown
  ~HeartbeatManager();

  // Set heartbeat callback function
  void setCallback(HeartbeatCallback callback);

  bool isRunning() const { return is_running_.load(std::memory_order_acquire); }
  bool isPaused() const { return is_paused_.load(std::memory_order_acquire); }

  void pause();
  void resume();

  // Start heartbeat
  bool start(uint32_t delay_ms = 0);

  // Stop heartbeat
  void stop();

  // Resets the idle timer. Call this when activity occurs.
  void updateTimestamp();

private:
  // Heartbeat thread function
  void heartbeatLoop();

  // Atomic flags to control thread state with memory order
  std::atomic_bool is_running_;
  std::atomic_bool is_paused_;

  // Store callback function
  HeartbeatCallback callback_;

  uint32_t delay_time_ms_;
  uint32_t interval_time_ms_;

  // Timestamp of the last known activity, now using a steady_clock time_point
  std::atomic<std::chrono::steady_clock::time_point> last_update_time_;

  // Variables for thread synchronization
  std::mutex mutex_;
  std::condition_variable cv_;

  // Heartbeat thread
  std::mutex thread_mutex_;
  std::thread heartbeat_thread_;
};

}

#endif // COMMON_HEARTBEAT_MANAGER_H
