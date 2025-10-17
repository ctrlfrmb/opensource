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
* Version: v3.1.0 (API Refinement)
* Date: 2022-09-08
*----------------------------------------------------------------------------*/

/**
* @file udp_client.h
* @brief High-performance asynchronous UDP client optimized for high-throughput
*        data acquisition using a memory pool to eliminate runtime allocations.
*
* This revised UDPClient class is engineered for demanding applications that
* require zero-packet-loss reception of high-frequency UDP datagrams. It replaces
* the previous dual-mode queue system with a unified, high-performance memory
* pool architecture.
*
* The core optimization is the pre-allocation of a pool of fixed-size buffers.
* The dedicated receive thread borrows a buffer from this pool, receives data
* directly into it, and then passes a smart pointer to the buffer into a
* lock-free queue. The consumer thread retrieves this smart pointer, processes
* the data, and upon the smart pointer's destruction, the buffer is automatically
* returned to the pool for reuse. This completely eliminates dynamic memory
* allocation and data copying in the critical receive path.
*
* Features:
* - Memory Pool Architecture: Eliminates runtime memory allocation for received data.
* - Zero-Copy (in user-space): Data is received directly into its final buffer.
* - Thread-safe asynchronous communication with a dedicated receive thread.
* - High-performance lock-free queue for passing buffer pointers.
* - Designed for high-throughput (100+ Mbps) and low-latency scenarios.
* - Broadcast and multicast support.
* - Cross-platform support (Windows/Linux/Unix).
*
*   Usage Example (High-Performance Mode):
*   Common::UDPClient::ConnectConfig config;
*   config.server_ip = "192.168.1.100";
*   config.server_port = 8080;
*   config.store_raw_data = true; // Explicitly set for max performance
*   config.recv_buffer_size = 8 * 1024 * 1024;
*   config.memory_pool_size = 4096;
*   client.start(config);
*
*   Common::UDPClient::DataBufferPtr buffer;
*   if (client.receive(buffer)) {
*       // Process data using buffer->data and buffer->data_len
*       // In this mode, buffer->from_ip and buffer->from_port will be empty/zero.
*   }
*/

#ifndef COMMON_UDP_CLIENT_H
#define COMMON_UDP_CLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <concurrentqueue.h>

#include "common_global.h"

struct sockaddr_in;

namespace Common {

class COMMON_API_EXPORT UDPClient {
public:
    static constexpr int INVALID_SOCKET_FD = -1;
    static constexpr int RECEIVE_BUFFER_SIZE = 1536; // MTU is typically 1500.
    static constexpr size_t DEFAULT_QUEUE_SIZE = 2000;
    static constexpr int DEFAULT_READ_TIMEOUT_MS = 30;

    // Refined DataBuffer to use platform-independent types.
    // This struct is pre-allocated in a memory pool. It no longer exposes
    // platform-specific types like sockaddr_in.
    struct DataBuffer {
        char data[RECEIVE_BUFFER_SIZE]; // Pre-allocated buffer for one UDP datagram.
        int data_len = 0;               // Actual length of the received data.
        std::string from_ip;            // Sender's IP address string.
        int from_port = 0;              // Sender's port.
    };

    // Smart pointer for managing DataBuffer lifetime.
    using DataBufferPtr = std::unique_ptr<DataBuffer, std::function<void(DataBuffer*)>>;

    // Client configuration
    struct ConnectConfig {
        std::string local_ip;
        int local_port{0};
        std::string server_ip;
        int server_port{0};
        int read_timeout{DEFAULT_READ_TIMEOUT_MS};
        bool enable_broadcast{false};
        bool enable_reuse_addr{true};

        // This flag now controls whether sender address info is processed.
        // true (default, recommended for performance): Only raw data is stored.
        //     from_ip and from_port in DataBuffer will be empty/zero.
        //     This avoids costly address-to-string conversions in the receive thread.
        // false: from_ip and from_port will be populated. Use only when sender
        //     information is strictly required, as it impacts performance.
        bool store_raw_data{false};

        size_t max_queue_size{DEFAULT_QUEUE_SIZE};

        int send_buffer_size{0};
        int recv_buffer_size{8 * 1024 * 1024}; // 8M

        // Size of the memory pool.
        size_t memory_pool_size{64};
    };

    using ErrorCallback = std::function<void(int errorCode, const std::string& errorMsg)>;

public:
    UDPClient();
    ~UDPClient();

    UDPClient(const UDPClient&) = delete;
    UDPClient& operator=(const UDPClient&) = delete;

    void setErrorCallback(ErrorCallback callback);
    bool start(const ConnectConfig& config);
    void stop();
    bool isRunning() const;

    // --- Send Operations ---
    bool send(const std::vector<char>& data);
    bool send(const std::string& data);
    bool send(const char* data, size_t length);
    bool sendTo(const std::vector<char>& data, const std::string& targetIp, int targetPort);
    bool sendTo(const std::string& data, const std::string& targetIp, int targetPort);
    bool sendTo(const char* data, size_t length, const std::string& targetIp, int targetPort);
    bool broadcast(const std::vector<char>& data, int targetPort, const std::string& broadcastIp = "255.255.255.255");
    bool broadcast(const std::string& data, int targetPort, const std::string& broadcastIp = "255.255.255.255");
    bool broadcast(const char* data, size_t length, int targetPort, const std::string& broadcastIp = "255.255.255.255");

    // --- High-Performance Receive Operations ---
    bool receive(DataBufferPtr& buffer);
    size_t receiveBulk(std::vector<DataBufferPtr>& buffers, size_t maxCount = 100);
    void clearReceiveQueue();
    size_t getReceiveQueueSize() const;

    // --- Getters ---
    const ConnectConfig& getConfig() const;
    std::string getLocalIp() const;
    int getLocalPort() const;

private:
    bool initNetworkLibrary();
    void cleanupNetworkLibrary();

    int createSocket();
    bool setSocketOptions(int fd);
    bool bindSocket(int fd);
    void closeSocket();
    bool closeTempSocket(int fd);

    void receiveThreadFunc();

    void initMemoryPool();
    void cleanupMemoryPool();

    void triggerErrorCallback(int errorCode, const std::string& message);
    bool parseAddress(const std::string& ip, int port, sockaddr_in& addr);
    bool sendToImpl(const char* data, size_t length, const sockaddr_in& targetAddr);

private:
    std::mutex thread_mutex_;
    ConnectConfig config_;
    std::atomic_int socket_fd_{INVALID_SOCKET_FD};
    std::atomic_bool is_running_{false};
    std::thread receive_thread_;
    ErrorCallback error_callback_{nullptr};
    std::atomic<int> local_port_{0};
    std::string local_ip_;
    std::unique_ptr<sockaddr_in> server_addr_;

    moodycamel::ConcurrentQueue<DataBufferPtr> receive_queue_;
    moodycamel::ConcurrentQueue<DataBuffer*> memory_pool_;
};

} // namespace Common

#endif // COMMON_UDP_CLIENT_H
