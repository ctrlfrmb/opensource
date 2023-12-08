/**
 * @file    thread_pool.hpp
 * @ingroup opensource
 * @brief   Thread pool management module with following features:
 *          - Maintain a fixed number of running threads (min_thread).
 *          - Dynamically adjust the thread pool based on the number of tasks (max_thread is the upper limit).
 *          - Configuration of the number of thread pools and the recovery time by users.
 * @author  leiwei
 * @date    2023.09.05
 * Copyright (c) ctrlfrmb 2023-2033
 */

#pragma once

#ifndef OPEN_SOURCE_THREAD_POOL_HPP
#define OPEN_SOURCE_THREAD_POOL_HPP

#include <mutex>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>
#include <memory>
#include <chrono>

// Define to use dynamic thread adjustment based on CPU load
#define USE_DYNAMIC_ADJUST_THREAD_BY_CPU
#ifdef USE_DYNAMIC_ADJUST_THREAD_BY_CPU
#if defined(__linux__)
#include <fstream>
#include <sstream>
#elif defined(_WIN32)
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#endif
#endif

// Define for thread pool debugging
#define USE_THREAD_POOL_DEBUG
#ifdef USE_THREAD_POOL_DEBUG
#include <iostream>
#endif

#define THREAD_NAME_FIXED "ctrlfrmb_thread"

namespace opensource {
namespace ctrlfrmb {

#ifdef USE_DYNAMIC_ADJUST_THREAD_BY_CPU
#if defined(__linux__)
// Get CPU load for each core on Linux
std::vector<float> getLinuxCPULoad() {
    std::ifstream procStat("/proc/stat");
    std::string line;
    std::vector<float> cpuLoads;

    while (std::getline(procStat, line)) {
        if (line.compare(0, 3, "cpu") == 0 && isdigit(line[3])) {
            std::istringstream iss(line);
            std::string cpu;
            unsigned long long user, nice, system, idle;
            iss >> cpu >> user >> nice >> system >> idle;

            static std::unordered_map<std::string, std::pair<unsigned long long, unsigned long long>> prevCpuTimes;
            auto& prevCpuTime = prevCpuTimes[cpu];
            unsigned long long prevIdle = prevCpuTime.first, prevTotal = prevCpuTime.second;

            unsigned long long total = user + nice + system + idle;
            float cpuLoad = static_cast<float>(total - idle - prevTotal + prevIdle) / static_cast<float>(total - prevTotal);
            cpuLoads.push_back(cpuLoad);
            std::cout<<cpu<<":"<<cpuLoad*100<<"%"<<std::endl;
            prevCpuTime = {idle, total};
        }
    }

    return cpuLoads;
}
#elif defined(_WIN32)
// Get CPU load for each core on Windows
std::vector<float> getWindowsCPULoad() {
    PDH_STATUS status;
    PDH_HQUERY query;
    std::vector<float> cpuLoads;

    // Open a query
    status = PdhOpenQuery(NULL, 0, &query);
    if (status != ERROR_SUCCESS) {
        // Error handling
        return cpuLoads;
    }

    // Get the number of processors in the system
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int numProcessors = sysInfo.dwNumberOfProcessors;

    std::vector<PDH_HCOUNTER> counters(numProcessors);

    // Add a counter for each processor
    for (int i = 0; i < numProcessors; ++i) {
        std::string counterPath = "\\Processor(" + std::to_string(i) + ")\\% Processor Time";
        status = PdhAddCounter(query, counterPath.c_str(), 0, &counters[i]);
        if (status != ERROR_SUCCESS) {
            // Error handling
            PdhCloseQuery(query);
            return cpuLoads;
        }
    }

    // Collect data
    status = PdhCollectQueryData(query);
    if (status != ERROR_SUCCESS) {
        // Error handling
        PdhCloseQuery(query);
        return cpuLoads;
    }

    // Wait a short time to get valid data
    Sleep(1000);

    // Collect data again
    status = PdhCollectQueryData(query);
    if (status != ERROR_SUCCESS) {
        // Error handling
        PdhCloseQuery(query);
        return cpuLoads;
    }

    // Retrieve the CPU load for each processor
    for (auto& counter : counters) {
        PDH_FMT_COUNTERVALUE counterValue;
        status = PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &counterValue);
        if (status != ERROR_SUCCESS) {
            // Error handling
            continue;
        }
        cpuLoads.push_back(static_cast<float>(counterValue.doubleValue));
    }

    // Close the query
    PdhCloseQuery(query);
    return cpuLoads;
}
#endif
#endif

/***
 * leiwei 2022.3.4
 * c++11 thread pool (multi-thread safe)
 * Partially sourced from the Internet
***/
// Thread safe implementation of a Queue using std::queue
template<typename T>
class SafeQueue {
private:
    std::queue<T> queue_; // Using template function to construct the queue
    std::mutex mutex_;    // Mutex for access synchronization

public:
    SafeQueue() = default;
    ~SafeQueue() = default;

    // Returns whether the queue is empty
    bool empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // Clears the queue
    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!queue_.empty()) {
            std::queue<T>().swap(queue_);
        }
    }

    // Returns the size of the queue
    size_t size() {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

    // Adds an element to the queue
    void enqueue(T &t) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.emplace(t);
    }

    // Removes an element from the queue
    bool dequeue(T &t) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return false;
        }

        t = std::move(queue_.front()); // Take the front element, return its value and use right-value reference
        queue_.pop(); // Pop the first element in the queue

        return true;
    }
};

class ThreadPool {
public:
    struct ThreadInfo {
        std::thread handle;                 // Thread handle
        bool is_exit;                       // Whether the thread has exited
        ThreadInfo(std::thread&& t, bool&& flag) : handle(std::move(t)), is_exit(std::move(flag)) {}
    };

private:
    // Thread pool constructor
    ThreadPool() {}

    // Thread pool destructor
    ~ThreadPool() {
        shutdown();
    }

    // Inner thread worker class
    class ThreadWorker {
    private:
        ThreadPool &ppool_; // Parent thread pool
        const bool is_fixed_; // Whether it is a fixed thread

    public:
        // Constructor
        ThreadWorker(ThreadPool *pool, bool&& fix) : ppool_(*pool), is_fixed_(std::move(fix)) {}

        // Overload () operator
        void operator()() {
            std::function<void()> func; // Define base function class func
            bool dequeued{false};       // Whether dequeuing elements from the queue
            bool is_exit{!is_fixed_};   // Whether the thread needs to exit

    #ifdef USE_THREAD_POOL_DEBUG
            std::cout << "############# " << (is_fixed_ ? "fixed" : "temp") << " thread [" << std::this_thread::get_id() << "] is start #################" << std::endl;
    #endif

            // Thread worker execution loop
            while (!ppool_.shutdown_) {
                {
                    std::unique_lock<std::mutex> lock(ppool_.conditional_mutex_);
                    is_exit = false;

                    // Block the current thread if the task queue is empty
                    if (ppool_.queue_.empty()) {
#ifdef USE_DYNAMIC_ADJUST_THREAD_BY_CPU
                        ppool_.adjust(true);
#endif

                        if (is_fixed_) {
                            ppool_.conditional_lock_.wait(lock); // Fixed threads wait indefinitely for a condition variable notification
                        } else {
                            // Temporary threads wait with a timeout for a condition variable notification, exit after timeout
                            if (std::cv_status::timeout == ppool_.conditional_lock_.wait_for(lock, std::chrono::seconds(ppool_.wait_max_time_))) {
                                is_exit = true;
                            }
                        }

                        if (ppool_.shutdown_) {
                            break;
                        }
                    }
                }

                // Thread exit logic for temporary threads
                if (is_exit) {
                    std::unique_lock<std::mutex> lock(ppool_.thread_mutex_);
                    for (auto& t : ppool_.threads_) {
                        if (t.handle.get_id() == std::this_thread::get_id()) {
                            t.is_exit = true;
#ifdef USE_THREAD_POOL_DEBUG
                            std::cout << "############# temp thread [" << std::this_thread::get_id() << "] is stop by timeout #################" << std::endl;
#endif
                            return;
                        }
                    }
                }

                // Dequeue and execute work function
                dequeued = ppool_.queue_.dequeue(func);
                if (dequeued) {
                    func();
                }
            }

#ifdef USE_THREAD_POOL_DEBUG
            std::cout << "############# " << (is_fixed_ ? "fixed" : "temp") << " thread [" << std::this_thread::get_id() << "] is stop by shutdown #################" << std::endl;
#endif
        }
    };

    std::mutex thread_mutex_; // Mutex for work thread queue
    std::vector<ThreadInfo> threads_; // Work thread queue and stop running state

    uint8_t max_thread_{8};   // Maximum number of work threads
    uint8_t min_thread_{2};   // Minimum number of work threads
    std::atomic<bool> shutdown_{false}; // Whether the thread pool is closed

    SafeQueue<std::function<void()>> queue_; // Safe queue for execution functions, i.e., task queue
    std::mutex conditional_mutex_; // Mutex for sleeping threads
    std::condition_variable conditional_lock_; // Thread condition lock for sleeping or waking up threads

    uint32_t wait_max_time_{600}; // Thread auto exit wait time in seconds

    std::once_flag init_flag_; // Flag to ensure init is called only once

#ifdef USE_DYNAMIC_ADJUST_THREAD_BY_CPU
    bool enable_dynamic_adjust_{true}; // enable dynamic adjustment temp thread
#endif

public:
    // Delete copy constructor and assignment operator to ensure uniqueness of the thread pool
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // Retrieve an instance of the thread pool (singleton pattern)
    static ThreadPool &Instance() {
        static ThreadPool obj;
        return obj;
    }

    // Set parameters for the thread pool's operation
    int set(uint8_t max_thread, uint8_t min_thread, uint32_t wait_max_time) {
        if (max_thread < 1)
            return -1;

        if (min_thread > max_thread_)
            return -2;

        if (wait_max_time < 1)
            return -3;

        max_thread_ = max_thread;
        min_thread_ = min_thread;
        wait_max_time_ = wait_max_time;
        return 0;
    }

#ifdef USE_DYNAMIC_ADJUST_THREAD_BY_CPU
    // Adjust the thread pool dynamically based on CPU load
    void adjust(bool is_add) {
        if (!enable_dynamic_adjust_)
            return;

        static std::atomic<std::uint8_t> counter = 0;
        if (is_add && ((counter > 0) || (threads_.size() == 1))) {
#if defined(__linux__)
            auto cpus = getLinuxCPULoad();
#elif defined(_WIN32)
            auto cpus = getWindowsCPULoad();
#endif
            std::unique_lock<std::mutex> lock(thread_mutex_);
            max_thread_ = cpus.size();
            counter = 0;
            return;
        }

        if (!is_add && (counter > 0)) {
            counter = 0;
            return;
        }

        ++counter;
    }
#endif

    // Clear the thread pool by removing exited threads
    void clear() {
        std::unique_lock<std::mutex> lock(thread_mutex_);
        if (threads_.size() > min_thread_) {
            for (auto it = threads_.begin(); it != threads_.end(); ) {
                if (it->is_exit) {
                    if (it->handle.joinable()) {
                        it->handle.join();
                    }
                    it = threads_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    // Initialize the thread pool
    void init() {
        std::unique_lock<std::mutex> lock(thread_mutex_);
        // Allocate work threads
        for (decltype(min_thread_) i = 0; i < min_thread_; ++i) {
            threads_.emplace_back(std::thread(ThreadWorker(this, true)), false);
            pthread_setname_np(threads_.back().handle.native_handle(), THREAD_NAME_FIXED); // Set thread name
        }
    }

    // Shutdown the thread pool, waiting for threads to finish their current task
    void shutdown() {
        shutdown_ = true;
        queue_.clear();
        conditional_lock_.notify_all(); // Notify all working threads to wake up

        {
            std::unique_lock<std::mutex> lock(thread_mutex_);
            if (!threads_.empty()) {
                for (auto& t : threads_) {
                    if (t.handle.joinable()) { // Check if the thread is waiting
                        t.handle.join(); // Join the thread to the waiting queue
                    }
                }
                std::vector<ThreadInfo>().swap(threads_); // Clear the threads vector
            }
        }
    }

    // Submit a function to be executed asynchronously by the pool
    template<typename F, typename... Args>
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
        std::call_once(init_flag_, &ThreadPool::init, this);  // Ensure init is called only once

        // Create a function with bounded parameters ready to execute
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f),
                                                               std::forward<Args>(args)...); // Bind function and arguments

        // Encapsulate it into a shared pointer to enable copy construction
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        // Wrap packaged task into a void function
        std::function<void()> wrapper_func = [task_ptr]() {
            (*task_ptr)();
        };

        clear();

#ifdef USE_DYNAMIC_ADJUST_THREAD_BY_CPU
        adjust(false);
#endif

        // Enqueue the function and wake up a waiting thread
        {
            std::unique_lock<std::mutex> lock(thread_mutex_);
            if ((queue_.size() >= threads_.size()) && (threads_.size() < max_thread_)) {
#ifdef USE_THREAD_POOL_DEBUG
                std::cout << "####### Task size " << queue_.size() << " greater than thread size " << threads_.size() << " #######\n";
#endif
                threads_.emplace_back(std::thread(ThreadWorker(this, false)), false);
                pthread_setname_np(threads_.back().handle.native_handle(), THREAD_NAME_FIXED); // Set thread name
            }
        }

        queue_.enqueue(wrapper_func);
        conditional_lock_.notify_one();

        // Return the future associated with the task
        return task_ptr->get_future();
    }
};
}
}

#endif // !OPEN_SOURCE_THREAD_POOL_HPP
