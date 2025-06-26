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
* @file tcp_client.h
* @brief High-performance asynchronous TCP client with automatic reconnection
*
* The TCPClient class provides a robust, thread-safe TCP client implementation with
* automatic reconnection capabilities and efficient data handling. It features
* lock-free queues, atomic operations, and comprehensive error handling for
* reliable network communication in production environments.
*
* Features:
* - Thread-safe asynchronous TCP communication
* - Automatic reconnection with exponential backoff
* - High-performance lock-free receive queue
* - Configurable socket options (TCP_NODELAY, SO_KEEPALIVE, SO_LINGER)
* - Graceful connection shutdown with proper cleanup
* - Cross-platform support (Windows/Linux/Unix)
* - Flexible callback system for data/error/reconnect events
* - Local IP binding support for multi-homed systems
* - Bulk data operations for improved throughput
* - Comprehensive error handling with detailed error codes
* - Memory-efficient buffering with configurable queue capacity
* - Atomic state management to prevent race conditions
*
* Usage example:
*   Common::TCPClient client;
*
*   // Set up callbacks
*   client.setReceiveCallback([](const std::vector<uint8_t>& data) {
*       // Handle received data
*   });
*
*   // Configure and connect
*   Common::TCPClient::ConnectConfig config;
*   config.serverIp = "192.168.1.100";
*   config.serverPort = 8080;
*   config.autoReconnect = true;
*
*   if (client.connect(config)) {
*       client.send("Hello", 5);
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
#include <condition_variable>
#include <concurrentqueue.h>

#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT TCPClient {
public:
    static constexpr int INVALID_SOCKET_FD = -1;
    static constexpr int READ_BUFFER_SIZE = 4096;
    static constexpr size_t DEFAULT_QUEUE_CAPACITY = 1048576; // 默认缓存1M
    static constexpr int DEFAULT_READ_TIMEOUT_MS = 30; // 默认读超时30毫秒

    // 连接配置
    struct ConnectConfig {
        std::string localIp;          // 本地绑定IP，可为空
        std::string serverIp;         // 服务器IP
        int serverPort{0};            // 服务器端口
        int connectTimeout{2000};     // 连接超时时间(毫秒)
        int readTimeout{DEFAULT_READ_TIMEOUT_MS}; // 读超时时间(毫秒)
        bool autoReconnect{false};     // 是否自动重连
        int reconnectInterval{1000};  // 重连间隔(毫秒)
        size_t queueCapacity{DEFAULT_QUEUE_CAPACITY};  // 队列容量
    };

    // 回调函数类型
    using ErrorCallback = std::function<void(int errorCode, const std::string& errorMsg)>;
    using ReceiveCallback = std::function<void(const std::vector<uint8_t>& data)>;
    using ReconnectCallback = std::function<void()>;

    // 错误码
    enum ErrorCode {
        TCP_ERROR_NONE = 0,
        TCP_ERROR_SOCKET_CREATE_FAILED = -4001,
        TCP_ERROR_BIND_FAILED = -4002,
        TCP_ERROR_CONNECT_FAILED = -4003,
        TCP_ERROR_CONNECT_TIMEOUT = -4004,
        TCP_ERROR_SEND_FAILED = -4005,
        TCP_ERROR_CONNECTION_CLOSED = -4006,
        TCP_ERROR_SOCKET_RECEIVE = -4007,
        TCP_ERROR_SOCKET_SET_FAILED = -4008,
        TCP_ERROR_UNKNOWN = -4010
    };

public:
    TCPClient();
    ~TCPClient();

    // 禁止拷贝
    TCPClient(const TCPClient&) = delete;
    TCPClient& operator=(const TCPClient&) = delete;

    // 设置回调
    void setErrorCallback(ErrorCallback callback);
    void setReceiveCallback(ReceiveCallback callback);
    void setReconnectCallback(ReconnectCallback callback);
    void setAutoReconnect(bool autoReconnect);

    // 连接服务器
    bool connect(const ConnectConfig& config);

    // 断开连接
    void disconnect();

    // 发送数据
    bool send(const char* data, size_t length);

    // 检查连接状态
    bool isConnected() const;

    // 获取接收队列中的所有数据
    bool receive(std::vector<uint8_t>& data);

    // 清空接收队列
    void clearReceiveQueue();

    // 获取队列大小
    size_t getQueueSize() const;

    // 获取连接信息
    const ConnectConfig& getConfig() const;

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

    // 设置TCP选项
    bool setTcpNoDelay(int fd, bool enable);
    bool setKeepAlive(int fd, bool enable, int idle = 60, int interval = 5, int count = 3);
    bool setLinger(int fd, bool enable, int seconds = 0);

    // 优雅关闭套接字
    void gracefulCloseSocket();

    // 关闭套接字
    void closeSocket();
    int closeTempSocket(int fd);

    // 尝试连接服务器
    int tryConnect();

    // 重连处理（同步）
    bool handleReconnect();

    // 接收线程函数
    void receiveThreadFunc();

    // 触发错误回调
    void triggerErrorCallback(ErrorCode code, const std::string& message);

    // 触发重连成功回调
    void triggerReconnectCallback();

    // 获取系统错误码
    int getLastSocketError() const;

private:
    std::mutex m_mutex;

    // 连接配置
    ConnectConfig m_config;

    // 套接字描述符
    std::atomic_int m_socketFd{INVALID_SOCKET_FD};

    // 线程控制标志
    std::atomic_bool m_running{false};

    // 接收线程
    std::thread m_receiveThread;

    // 发送互斥锁
    std::mutex m_sendMutex;

    // 接收数据队列 - 直接存储字节
    moodycamel::ConcurrentQueue<uint8_t> m_receiveQueue;

    // 回调函数
    ErrorCallback m_errorCallback{nullptr};
    ReceiveCallback m_dataCallback{nullptr};
    ReconnectCallback m_reconnectCallback{nullptr};
};

}

#endif // COMMON_TCP_CLIENT_H
