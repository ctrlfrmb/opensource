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
* Version: v2.1.0
* Date: 2022-09-05
*----------------------------------------------------------------------------*/

/**
* @file tcp_client.h
* @brief High-performance asynchronous TCP client with automatic reconnection
*        and a bounded receive queue.
*
* The TCPClient class provides a robust, thread-safe TCP client implementation with
* asynchronous reconnection capabilities and efficient data handling. It features
* a lock-free queue with a configurable size limit to prevent uncontrolled memory
* growth, atomic operations, and comprehensive error handling for reliable network
* communication in production environments.
*
* Data is received in a dedicated background thread and stored in an internal
* queue. The user must call the receive() method from their own thread to
* consume the data.
*
* Features:
* - Thread-safe asynchronous TCP communication
* - Asynchronous reconnection with exponential backoff (non-blocking)
* - High-performance lock-free receive queue with a logical size limit
* - Automatic discarding of oldest data when the queue limit is exceeded
* - Configurable socket options using Utils class functions
* - Graceful connection shutdown with proper cleanup
* - Cross-platform support (Windows/Linux/Unix)
* - Flexible callback system for error and reconnect events
* - Local IP binding support for multi-homed systems
* - Bulk data operations for improved throughput
* - Unified error handling with UTILS_SOCKET_* error codes
* - Atomic state management to prevent race conditions
* - Separate reconnection thread for improved responsiveness
*
* Usage example:
*   Common::TCPClient client;
*
*   // Set up callbacks for events
*   client.setErrorCallback([](int errorCode, const std::string& message) {
*       // Handle errors using UTILS_SOCKET_* error codes
*   });
*
*   // Configure and connect
*   Common::TCPClient::ConnectConfig config;
*   config.server_ip = "192.168.1.100";
*   config.server_port = 8080;
*   config.auto_reconnect = true;
*
*   if (client.connect(config)) {
*       client.send("Hello", 5);
*
*       // In your application's main loop or a dedicated thread:
*       std::vector<char> received_data;
*       if (client.receive(received_data)) {
*           // Process received_data
*       }
*   }
*/

#ifndef COMMON_TCP_CLIENT_H
#define COMMON_TCP_CLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <concurrentqueue.h>

#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT TCPClient {
public:
    static constexpr int INVALID_SOCKET_FD = -1;
    static constexpr int READ_BUFFER_SIZE = 4096;
    static constexpr size_t DEFAULT_QUEUE_CAPACITY = 1048576; // 1M chars, for initial allocation
    static constexpr int DEFAULT_READ_TIMEOUT_MS = 30; // 30ms

    // Connection configuration
    struct ConnectConfig {
        std::string local_ip;                              // Local binding IP (optional)
        std::string server_ip;                             // Server IP
        int server_port{0};                                // Server port
        int connect_timeout{2000};                         // Connection timeout (milliseconds)
        int read_timeout{DEFAULT_READ_TIMEOUT_MS};         // Read timeout (milliseconds)
        bool auto_reconnect{false};                        // Enable auto-reconnection
        int reconnect_interval{1000};                      // Reconnection interval (milliseconds)
        int max_reconnect_interval{60000};                 // Maximum reconnection interval (milliseconds)

        // Maximum number of bytes to buffer in the receive queue.
        // If this limit is exceeded, the oldest data will be discarded to make space.
        size_t max_queue_size{DEFAULT_QUEUE_CAPACITY};

        // TCP options
        bool enable_tcp_no_delay{true};                    // Enable TCP_NODELAY
        bool enable_keep_alive{true};                      // Enable Keep-Alive
        int keep_alive_idle{60};                           // Keep-Alive idle time (seconds)
        int keep_alive_interval{5};                        // Keep-Alive interval (seconds)
        int keep_alive_count{3};                           // Keep-Alive probe count
    };

    // Callback function types for events
    using ErrorCallback = std::function<void(int errorCode, const std::string& errorMsg)>;
    using ReconnectCallback = std::function<void()>;

public:
    TCPClient();
    ~TCPClient();

    // Prohibit copying
    TCPClient(const TCPClient&) = delete;
    TCPClient& operator=(const TCPClient&) = delete;

    // Set callbacks for events (cannot be set while connected)
    void setErrorCallback(ErrorCallback callback);
    void setReconnectCallback(ReconnectCallback callback);
    void setAutoReconnect(bool autoReconnect);

    // Connect to server
    bool connect(const ConnectConfig& config);

    // Disconnect
    void disconnect();

    // Check connection status
    bool isConnected() const;

    // Send data
    bool send(const std::vector<char>& data);
    bool send(const std::string& data);
    bool send(const char* data, size_t length);

    // Receive data from the internal queue. Returns true if data was received.
    bool receive(std::vector<char>& data);
    bool receive(std::vector<char>& data, size_t maxBytes);

    // Clear the entire receive queue
    void clearReceiveQueue();

    // Get the approximate size of the receive queue in bytes
    size_t getQueueSize() const;

    // Get the current connection configuration
    const ConnectConfig& getConfig() const;

private:
    // WSA initialization and cleanup
    bool initNetworkLibrary();
    void cleanupNetworkLibrary();

    // Socket operations
    int createSocket();
    bool setSocketOptions(int fd);
    void closeSocket();
    int closeTempSocket(int fd);

    // Connection logic
    int tryConnect();

    // Asynchronous reconnection handling
    void startAsyncReconnect();
    void stopAsyncReconnect();

    // Thread functions
    void receiveThreadFunc();
    void reconnectThreadFunc();

    void pushToReceiveQueue(const char* data, size_t length);

    // Callback triggers
    void triggerErrorCallback(int errorCode, const std::string& message);
    void triggerReconnectCallback();

private:
    std::mutex thread_mutex_;
    std::mutex reconnect_mutex_;
    std::condition_variable reconnect_cv_;

    // Connection configuration
    ConnectConfig config_;

    // Socket descriptor
    std::atomic_int socket_fd_{INVALID_SOCKET_FD};

    // Thread control flags
    std::atomic_bool running_{false};
    std::atomic_bool reconnecting_{false};

    // Threads
    std::thread receive_thread_;
    std::thread reconnect_thread_;

    // Lock-free receive queue for raw bytes
    moodycamel::ConcurrentQueue<char> receive_queue_;

    // Callback functions for events
    ErrorCallback error_callback_{nullptr};
    ReconnectCallback reconnect_callback_{nullptr};

    // Reconnect counter for exponential backoff
    std::atomic_int reconnect_counter_{0};
};

} // namespace Common

#endif // COMMON_TCP_CLIENT_H
