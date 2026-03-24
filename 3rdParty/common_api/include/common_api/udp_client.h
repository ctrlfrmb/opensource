/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2022-2042 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v4.0.0 (Simplified API + Source Filtering)
* Date: 2022-09-08
*----------------------------------------------------------------------------*/

/**
* @file udp_client.h
* @brief High-performance asynchronous UDP client optimized for high-throughput
*        data acquisition using a memory pool to eliminate runtime allocations.
*
* v4.0.0 Changes:
* - Simplified API: Removed redundant vector<char>/string overloads for
*   send(), sendTo(), broadcast(). Only const char* + size_t versions remain.
* - Added kernel-level source address filtering (filter_by_remote) via connect()
*   for multi-instance scenarios sharing the same local port.
* - Fixed bindSocket() to properly bind INADDR_ANY when local_ip is empty
*   but local_port is specified.
* - Connected socket optimization: uses send()/recv() instead of sendto()/recvfrom().
*
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
#ifdef _WIN32
    using socklen_t = int;
#else
    using socklen_t = unsigned int;
#endif

    static constexpr int INVALID_SOCKET_FD = -1;
    static constexpr int RECEIVE_BUFFER_SIZE = 1536;
    static constexpr size_t DEFAULT_QUEUE_SIZE = 2000;
    static constexpr int DEFAULT_READ_TIMEOUT_MS = 100;

    struct DataBuffer {
        char data[RECEIVE_BUFFER_SIZE];
        int data_len = 0;
        std::string from_ip;
        int from_port = 0;
    };

    using DataBufferPtr = std::unique_ptr<DataBuffer, std::function<void(DataBuffer*)>>;

    struct ConnectConfig {
        std::string local_ip;
        int local_port{0};
        std::string server_ip;
        int server_port{0};
        int read_timeout{DEFAULT_READ_TIMEOUT_MS};
        bool enable_broadcast{false};
        bool enable_reuse_addr{true};

        // true (default): Only raw data is stored. from_ip/from_port will be empty/zero.
        // false: from_ip and from_port will be populated (slower due to address conversion).
        bool store_raw_data{false};

        size_t max_queue_size{DEFAULT_QUEUE_SIZE};

        int send_buffer_size{0};
        int recv_buffer_size{8 * 1024 * 1024}; // 8M

        size_t memory_pool_size{64};

        // Windows-specific: Disable UDP CONNRESET reporting (SIO_UDP_CONNRESET)
        bool disable_connection_reset_report{false};

        // =====================================================================
        // Kernel-level source address filtering via connect()
        // =====================================================================
        // When true AND server_ip/server_port are set:
        //   Calls connect() on the UDP socket after bind(). This tells the
        //   kernel to ONLY deliver packets from the specified remote address
        //   to this socket. Essential when multiple sockets bind to the same
        //   local port (SO_REUSEADDR) but communicate with different devices.
        //
        // When false (default): Standard behavior, receives from any source.
        //
        // IMPORTANT: When enabled, sendTo() to a DIFFERENT address will fail.
        //   Only send() to the configured default server will work.
        //   broadcast() is also unavailable on a connected socket.
        // =====================================================================
        bool filter_by_remote{false};
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

    // =========================================================================
    // Send Operations (simplified: only const char* + size_t versions)
    // =========================================================================

    /// @brief Send data to the default server configured in ConnectConfig
    bool send(const char* data, size_t length);

    /// @brief Send data to a specific target address
    bool sendTo(const char* data, size_t length, const std::string& targetIp, int targetPort);

    /// @brief Broadcast data (requires enable_broadcast=true, incompatible with filter_by_remote)
    bool broadcast(const char* data, size_t length, int targetPort,
                   const std::string& broadcastIp = "255.255.255.255");

    // =========================================================================
    // High-Performance Receive Operations
    // =========================================================================

    bool receive(DataBufferPtr& buffer);
    size_t receiveBulk(std::vector<DataBufferPtr>& buffers, size_t maxCount = 100);
    void clearReceiveQueue();
    size_t getReceiveQueueSize() const;

    // =========================================================================
    // Getters
    // =========================================================================

    const ConnectConfig& getConfig() const;
    std::string getLocalIp() const;
    int getLocalPort() const;

private:
    bool initNetworkLibrary();
    void cleanupNetworkLibrary();

    int createSocket();
    bool setSocketOptions(int fd);
    bool bindSocket(int fd);
    bool connectSocket(int fd);
    void closeSocket();
    bool closeTempSocket(int fd);

    void receiveThreadFunc();
    void drainSocket(int fd, sockaddr_in& fromAddr, socklen_t& fromAddrLen);

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
    std::atomic_bool is_connected_{false};
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
