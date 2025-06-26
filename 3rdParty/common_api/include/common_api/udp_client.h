/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2023 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei
* Version: v1.0.0
* Date: 2023-12-25
*----------------------------------------------------------------------------*/

/**
* @file udp_client.h
* @brief High-performance asynchronous UDP client with default server support
*
* The UDPClient class provides a robust, thread-safe UDP client implementation
* for connectionless communication with a default server endpoint. It features
* lock-free queues, atomic operations, and comprehensive error handling for
* reliable datagram communication in production environments.
*
* Features:
* - Thread-safe asynchronous UDP communication
* - Default server endpoint configuration for simplified usage
* - High-performance lock-free receive queue
* - Broadcast and multicast support
* - Configurable socket options (SO_BROADCAST, SO_REUSEADDR)
* - Cross-platform support (Windows/Linux/Unix)
* - Flexible callback system for data/error events
* - Local IP and port binding support
* - Bulk data operations for improved throughput
* - Comprehensive error handling with detailed error codes
* - Memory-efficient buffering with configurable queue capacity
* - Atomic state management to prevent race conditions
*
* Usage example:
*   Common::UDPClient client;
*
*   // Set up callbacks
*   client.setReceiveCallback([](const std::vector<uint8_t>& data, const std::string& fromIp, int fromPort) {
*       // Handle received data
*   });
*
*   // Configure and start
*   Common::UDPClient::Config config;
*   config.serverIp = "192.168.1.100";
*   config.serverPort = 8080;
*   config.localPort = 9090;
*
*   if (client.start(config)) {
*       client.send("Hello", 5);  // Send to default server
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

// 前置声明
struct sockaddr_in;

namespace Common {

class COMMON_API_EXPORT UDPClient {
public:
    static constexpr int INVALID_SOCKET_FD = -1;
    static constexpr int READ_BUFFER_SIZE = 65536; // UDP最大包大小
    static constexpr size_t DEFAULT_QUEUE_CAPACITY = 1048576; // 默认缓存1M
    static constexpr int DEFAULT_READ_TIMEOUT_MS = 30; // 默认读超时30毫秒

    // UDP数据包结构
    struct UDPPacket {
        std::vector<uint8_t> data;
        std::string fromIp;
        int fromPort;

        UDPPacket() : fromPort(0) {}
        UDPPacket(const std::vector<uint8_t>& d, const std::string& ip, int port)
            : data(d), fromIp(ip), fromPort(port) {}
    };

    // 客户端配置
    struct Config {
        std::string localIp;              // 本地绑定IP，可为空(绑定所有接口)
        int localPort{0};                 // 本地绑定端口，0表示任意端口
        std::string serverIp;             // 默认服务器IP
        int serverPort{0};                // 默认服务器端口
        int readTimeout{DEFAULT_READ_TIMEOUT_MS}; // 读超时时间(毫秒)
        bool enableBroadcast{false};      // 是否启用广播
        bool enableReuseAddr{true};       // 是否启用地址重用
        size_t queueCapacity{DEFAULT_QUEUE_CAPACITY};  // 队列容量
    };

    // 回调函数类型
    using ErrorCallback = std::function<void(int errorCode, const std::string& errorMsg)>;
    using ReceiveCallback = std::function<void(const std::vector<uint8_t>& data, const std::string& fromIp, int fromPort)>;

    // 错误码
    enum ErrorCode {
        UDP_ERROR_NONE = 0,
        UDP_ERROR_SOCKET_CREATE_FAILED = -5001,
        UDP_ERROR_BIND_FAILED = -5002,
        UDP_ERROR_SEND_FAILED = -5003,
        UDP_ERROR_SOCKET_RECEIVE = -5004,
        UDP_ERROR_SOCKET_SET_FAILED = -5005,
        UDP_ERROR_INVALID_ADDRESS = -5006,
        UDP_ERROR_NOT_STARTED = -5007,
        UDP_ERROR_UNKNOWN = -5010
    };

public:
    UDPClient();
    ~UDPClient();

    // 禁止拷贝
    UDPClient(const UDPClient&) = delete;
    UDPClient& operator=(const UDPClient&) = delete;

    // 设置回调
    void setErrorCallback(ErrorCallback callback);
    void setReceiveCallback(ReceiveCallback callback);

    // 启动UDP客户端
    bool start(const Config& config);

    // 停止UDP客户端
    void stop();

    // 发送数据到默认服务器
    bool send(const char* data, size_t length);

    // 发送数据到指定地址
    bool sendTo(const char* data, size_t length, const std::string& targetIp, int targetPort);

    // 广播数据
    bool broadcast(const char* data, size_t length, int targetPort, const std::string& broadcastIp = "255.255.255.255");

    // 检查是否运行中
    bool isRunning() const;

    // 获取接收队列中的数据包
    bool receive(UDPPacket& packet);
    bool receive(std::vector<UDPPacket>& packets, size_t maxCount = 100);

    // 清空接收队列
    void clearReceiveQueue();

    // 获取队列大小
    size_t getQueueSize() const;

    // 获取配置信息
    const Config& getConfig() const;

    // 获取本地绑定信息
    std::string getLocalIp() const;
    int getLocalPort() const;

private:
    // 初始化网络库
    static bool initNetworkLibrary();
    static void cleanupNetworkLibrary();

    // 创建套接字
    int createSocket();

    // 设置套接字选项
    bool setSocketOptions(int fd);

    // 设置读超时
    bool setReceiveTimeout(int fd, int timeoutMs);

    // 设置广播选项
    bool setBroadcast(int fd, bool enable);

    // 设置地址重用选项
    bool setReuseAddr(int fd, bool enable);

    // 绑定套接字
    bool bindSocket(int fd);

    // 关闭套接字
    void closeSocket();
    int closeTempSocket(int fd);

    // 接收线程函数
    void receiveThreadFunc();

    // 触发错误回调
    void triggerErrorCallback(ErrorCode code, const std::string& message);

    // 获取系统错误码
    int getLastSocketError() const;

    // 解析地址
    bool parseAddress(const std::string& ip, int port, sockaddr_in& addr);

private:
    std::mutex m_mutex;

    // 客户端配置
    Config m_config;

    // 套接字描述符
    std::atomic_int m_socketFd{INVALID_SOCKET_FD};

    // 线程控制标志
    std::atomic_bool m_running{false};

    // 接收线程
    std::thread m_receiveThread;

    // 发送互斥锁
    std::mutex m_sendMutex;

    // 接收数据队列
    moodycamel::ConcurrentQueue<UDPPacket> m_receiveQueue;

    // 回调函数
    ErrorCallback m_errorCallback{nullptr};
    ReceiveCallback m_dataCallback{nullptr};

    // 本地绑定信息
    std::atomic<int> m_localPort{0};
    std::string m_localIp;

    // 默认服务器地址（缓存解析结果）
    sockaddr_in* m_serverAddr{nullptr};
};

}

#endif // COMMON_UDP_CLIENT_H
