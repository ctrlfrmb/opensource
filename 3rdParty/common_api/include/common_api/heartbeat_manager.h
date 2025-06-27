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
* Version: v1.0.3
* Date: 2022-03-17
*----------------------------------------------------------------------------*/

/**
* @file heartbeat_manager.h
* @brief Thread-safe heartbeat mechanism for periodic tasks
*
* The HeartbeatManager class provides a robust implementation of a heartbeat
* mechanism that executes callback functions at regular intervals. It uses
* modern C++ threading primitives to ensure thread safety and proper resource
* management.
*
* Features:
* - Thread-safe implementation with atomic operations
* - Configurable intervals with millisecond precision
* - Optional startup delay
* - Clean shutdown with proper thread joining
* - Responsive to stop requests through condition variables
* - Memory-order guarantees for thread synchronization
*
* Usage example:
*   auto heartbeat = new Common::HeartbeatManager(2000); // 2000ms interval
*   heartbeat->setCallback([]() {
*     // Periodic task to execute
*   });
*   heartbeat->start();
*
*   // Later when done:
*   heartbeat->stop();
*/

#ifndef COMMON_HEARTBEAT_MANAGER_H
#define COMMON_HEARTBEAT_MANAGER_H

#include <atomic>
#include <thread>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT HeartbeatManager {
public:
  // Class constant, default heartbeat timeout (ms)
  static constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 1000;

  using HeartbeatCallback = std::function<void()>;

  // Default constructor, only sets timeout
  explicit HeartbeatManager(uint32_t interval = HEARTBEAT_TIMEOUT_MS);

  // Disable copy constructor and assignment operator
  HeartbeatManager(const HeartbeatManager&) = delete;
  HeartbeatManager& operator=(const HeartbeatManager&) = delete;

  // Destructor, ensures thread-safe shutdown
  ~HeartbeatManager();

  // Set heartbeat callback function
  void setCallback(HeartbeatCallback callback);

  // Start heartbeat
  bool start(uint32_t delay_ms = 0);

  // Stop heartbeat
  void stop();

  void updateTimestamp();

private:
  uint64_t getCurrentTimeMs() const;

  uint64_t calculateNextWaitTime() const;

  // Heartbeat thread function
  void heartbeatLoop();

  // Callback function
  HeartbeatCallback heartbeatCallback_;

  uint32_t delayTimeMs_;

  // Heartbeat interval
  uint32_t intervalTimeMs_;

  std::atomic_uint64_t lastUpdateTimeMs_{0};

  // Atomic flag to control thread running state with memory order
  std::atomic<bool> running_{false};

  // Variables for thread synchronization
  std::mutex mutex_;
  std::condition_variable cv_;

  // Heartbeat thread
  std::thread heartbeatThread_;
};

}

#endif // COMMON_HEARTBEAT_MANAGER_H
