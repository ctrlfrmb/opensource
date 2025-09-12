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
* Date: 2025-09-05
*----------------------------------------------------------------------------*/

/**
* @file udp_client.h
* @brief High-performance asynchronous UDP client with a bounded receive queue
*        and broadcast/multicast support.
*
* The UDPClient class provides a robust, thread-safe UDP client implementation
* for connectionless communication. It features a lock-free queue with a
* configurable packet limit to prevent uncontrolled memory growth, atomic
* operations, and comprehensive error handling for reliable datagram communication.
*
* Data is received in a dedicated background thread and stored in an internal
* queue of UDPPackets. The user must call the receive() method from their own
* thread to consume the data.
*
* Features:
* - Thread-safe asynchronous UDP communication
* - Default server endpoint configuration for simplified usage
* - High-performance lock-free receive queue with a logical packet limit
* - Automatic discarding of oldest packets when the queue limit is exceeded
* - Broadcast and multicast support with proper socket configuration
* - Configurable socket options using Utils class functions
* - Cross-platform support (Windows/Linux/Unix)
* - Flexible callback system for error events
* - Local IP and port binding support for multi-homed systems
* - Bulk data operations for improved throughput
* - Unified error handling with UTILS_SOCKET_* error codes
*
* Usage example:
*   Common::UDPClient client;
*
*   // Set up error callback
*   client.setErrorCallback([](int errorCode, const std::string& message) {
*       // Handle errors using UTILS_SOCKET_* error codes
*   });
*
*   // Configure and start
*   Common::UDPClient::Config config;
*   config.server_ip = "192.168.1.100";
*   config.server_port = 8080;
*   config.local_port = 9090;
*
*   if (client.start(config)) {
*       client.send("Hello", 5);
*
*       // In your application's main loop or a dedicated thread:
*       Common::UDPClient::UDPPacket packet;
*       if (client.receive(packet)) {
*           // Process received packet.data from packet.from_ip
*       }
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

// Forward declarations
struct sockaddr_in;

namespace Common {

class COMMON_API_EXPORT UDPClient {
public:
    static constexpr int INVALID_SOCKET_FD = -1;
    static constexpr int READ_BUFFER_SIZE = 65536; // UDP maximum datagram size
    static constexpr size_t DEFAULT_QUEUE_PACKET_COUNT = 2048; // Default max packets in queue
    static constexpr int DEFAULT_READ_TIMEOUT_MS = 30; // 30ms

    // UDP packet structure
    struct UDPPacket {
        std::vector<char> data;
        std::string from_ip;
        int from_port;

        UDPPacket() : from_port(0) {}
        UDPPacket(const std::vector<char>& d, const std::string& ip, int port)
            : data(d), from_ip(ip), from_port(port) {}
        UDPPacket(std::vector<char>&& d, std::string&& ip, int port)
            : data(std::move(d)), from_ip(std::move(ip)), from_port(port) {}
    };

    // Client configuration
    struct ConnectConfig {
        std::string local_ip;                              // Local binding IP (optional, empty = bind all)
        int local_port{0};                                 // Local binding port (0 = any port)
        std::string server_ip;                             // Default server IP
        int server_port{0};                                // Default server port
        int read_timeout{DEFAULT_READ_TIMEOUT_MS};         // Read timeout (milliseconds)
        bool enable_broadcast{false};                      // Enable broadcast capability
        bool enable_reuse_addr{true};                      // Enable address reuse

        // Maximum number of packets to buffer in the receive queue.
        // If this limit is exceeded, the oldest packets will be discarded.
        size_t max_queue_size{DEFAULT_QUEUE_PACKET_COUNT};

        // Advanced socket options
        int send_buffer_size{0};                           // Send buffer size (0 = system default)
        int recv_buffer_size{0};                           // Receive buffer size (0 = system default)
    };

    // Callback function type for error events
    using ErrorCallback = std::function<void(int errorCode, const std::string& errorMsg)>;

public:
    UDPClient();
    ~UDPClient();

    // Prohibit copying
    UDPClient(const UDPClient&) = delete;
    UDPClient& operator=(const UDPClient&) = delete;

    // Set error callback (cannot be set while running)
    void setErrorCallback(ErrorCallback callback);

    // Start UDP client
    bool start(const ConnectConfig& config);

    // Stop UDP client
    void stop();

    // Check if running
    bool isRunning() const;

    // Send data to the default server
    bool send(const std::vector<char>& data);
    bool send(const std::string& data);
    bool send(const char* data, size_t length);

    // Send data to a specific address
    bool sendTo(const std::vector<char>& data, const std::string& targetIp, int targetPort);
    bool sendTo(const std::string& data, const std::string& targetIp, int targetPort);
    bool sendTo(const char* data, size_t length, const std::string& targetIp, int targetPort);

    // Broadcast data
    bool broadcast(const std::vector<char>& data, int targetPort, const std::string& broadcastIp = "255.255.255.255");
    bool broadcast(const std::string& data, int targetPort, const std::string& broadcastIp = "255.255.255.255");
    bool broadcast(const char* data, size_t length, int targetPort, const std::string& broadcastIp = "255.255.255.255");

    // Receive a single packet from the queue. Returns true if a packet was received.
    bool receive(UDPPacket& packet);
    // Receive multiple packets from the queue. Returns true if at least one packet was received.
    bool receive(std::vector<UDPPacket>& packets, size_t maxCount = 100);

    // Clear the entire receive queue
    void clearReceiveQueue();

    // Get the approximate number of packets in the receive queue
    size_t getQueueSize() const;

    // Get the current configuration
    const ConnectConfig& getConfig() const;

    // Get local binding information
    std::string getLocalIp() const;
    int getLocalPort() const;

private:
    // Network library initialization
    bool initNetworkLibrary();
    void cleanupNetworkLibrary();

    // Socket operations
    int createSocket();
    bool setSocketOptions(int fd);
    bool bindSocket(int fd);
    void closeSocket();
    int closeTempSocket(int fd);

    // Thread function
    void receiveThreadFunc();

    // Pushes a received packet into the queue, enforcing the size limit.
    void pushToReceiveQueue(UDPPacket&& packet);

    // Error callback trigger
    void triggerErrorCallback(int errorCode, const std::string& message);

    // Address parsing
    bool parseAddress(const std::string& ip, int port, sockaddr_in& addr);

    // Core send implementation
    bool sendToImpl(const char* data, size_t length, const sockaddr_in& targetAddr);

private:
    std::mutex mutex_;
    ConnectConfig config_;
    std::atomic_int socket_fd_{INVALID_SOCKET_FD};
    std::atomic_bool running_{false};
    std::thread receive_thread_;
    moodycamel::ConcurrentQueue<UDPPacket> receive_queue_;
    ErrorCallback error_callback_{nullptr};
    std::atomic<int> local_port_{0};
    std::string local_ip_;
    std::unique_ptr<sockaddr_in> server_addr_;
};

} // namespace Common

#endif // COMMON_UDP_CLIENT_H
