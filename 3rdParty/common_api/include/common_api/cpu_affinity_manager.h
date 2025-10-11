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
* Version: v1.0.0
* Date: 2022-05-12
*----------------------------------------------------------------------------*/

/**
* @file cpu_affinity_manager.h
* @brief A cross-platform RAII utility for managing thread-to-CPU core affinity.
*
* The CpuAffinityManager class provides a robust, exception-safe mechanism for
* binding the current thread to a specific CPU core. It is designed using the
* RAII (Resource Acquisition Is Initialization) pattern, which automatically
* saves the thread's original affinity upon creation and restores it upon
* destruction. This simplifies thread management in high-performance and
* real-time applications.
*
* Features:
* - RAII-style management for automatic cleanup and exception safety.
* - Cross-platform support for Windows and POSIX-compliant systems (Linux, macOS).
* - Intelligent core selection: can automatically bind to the least busy CPU core.
* - Option to bind to a user-specified CPU core.
* - Static utility functions for querying system CPU information.
*/

#ifndef COMMON_CPU_AFFINITY_MANAGER_H
#define COMMON_CPU_AFFINITY_MANAGER_H

#include "common_global.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

namespace Common {

class COMMON_API_EXPORT CpuAffinityManager {
public:
    /**
     * @brief Constructs the manager and binds the current thread to a CPU core.
     * @param core_id The target CPU core ID.
     *                - If >= 0, attempts to bind to the specified core.
     *                - If -1 (default), finds the least busy core and binds to it.
     */
    explicit CpuAffinityManager(int core_id = -1);

    /**
     * @brief Destructor that automatically restores the thread's original CPU affinity.
     */
    ~CpuAffinityManager();

    // Disable copy and assignment to enforce RAII semantics.
    CpuAffinityManager(const CpuAffinityManager&) = delete;
    CpuAffinityManager& operator=(const CpuAffinityManager&) = delete;

    /**
     * @brief Checks if the thread was successfully bound to a core.
     * @return true if binding was successful, false otherwise.
     */
    bool isBound() const { return is_bound_; }

    // --- Static Utility Functions ---

    /**
     * @brief Gets the number of logical CPU processors available on the system.
     * @return The number of CPU cores.
     */
    static int getCoreCount();

    /**
     * @brief Finds the logical CPU core with the lowest current usage.
     *
     * This function samples CPU usage over a short period (e.g., 200ms) to determine
     * the least busy core. It has a one-time performance cost when called.
     *
     * @return The ID of the least busy core, or core 0 as a fallback on error.
     */
    static int findLeastBusyCore();

private:
    bool is_bound_ = false;

#ifdef _WIN32
    DWORD_PTR original_affinity_mask_ = 0;
    HANDLE thread_handle_ = nullptr;
#else
    cpu_set_t original_affinity_mask_;
    pthread_t thread_handle_;
#endif
};

} // namespace Common

#endif // COMMON_CPU_AFFINITY_MANAGER_H
