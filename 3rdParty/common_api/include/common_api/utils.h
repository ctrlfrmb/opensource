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
* Version: v1.4.2
* Date: 2022-08-17
*----------------------------------------------------------------------------*/

/**
* @file utils.h
* @brief Comprehensive utility functions for common tasks and network operations
*
* The Utils class provides a wide range of utility functions organized into
* several categories:
*
* 1. System & Process Management
*    - Cross-platform crash/exception handling
*    - Process priority management
*    - Thread identification utilities
*
* 2. Time & Data Processing
*    - High-performance timestamp generation
*    - Checksum calculation and hex string formatting
*    - File system operations with error handling
*
* 3. CAN Bus Signal Processing
*    - Comprehensive bit-level operations for both big and little endian formats
*    - Signal extraction and insertion for CAN frame processing
*    - Binary data manipulations with endianness support
*
* 4. Socket Network Operations
*    - Socket blocking mode control
*    - Non-blocking connection with timeout
*    - Socket linger configuration
*    - Graceful socket closure
*
* Usage example:
*   // Extract a CAN signal
*   uint64_t value = Common::Utils::getUnsignedSignalValueByLSB(
*       canData, dataLength, startBit, signalSize);
*
*   // Connect socket with timeout
*   int result = Common::Utils::connectSocketNonBlocking(fd, "192.168.1.100", 8080, 3000);
*/

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <vector>
#include <string>
#include <unordered_set>
#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT Utils {
public:
    //=============================================================================
    // System & Process Management
    //=============================================================================

    // 崩溃/异常处理回调函数类型
    typedef void (*CrashHandlerCallback)();

    // 注册崩溃处理回调（如关闭设备等）
    static void registerCrashHandler(CrashHandlerCallback cb);

    // 启用全局崩溃/异常捕获（main函数最早调用）
    static void setupCrashHandler();

    // 获取当前线程ID字符串
    static std::string getThreadIdString();

    // 设置进程为高优先级
    static void setProcessHighPriority();

    //=============================================================================
    // Time & Data Processing
    //=============================================================================

    // 高性能时间字符串获取（使用线程本地存储）
    static const char* getCurrentTimeStringFast();

    // 计算数据校验和
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);

    // 将字节数组转换为十六进制字符串
    static std::string bytesToHexString(const uint8_t* data, size_t length);

    // 确保文件路径存在（创建必要的目录）
    static bool ensureFilePath(const std::string& filePath);

    //=============================================================================
    // CAN Bus Signal Processing
    //=============================================================================

    // 辅助函数：符号扩展
    static inline int64_t signalRawValueToSigned(uint64_t rawValue, uint16_t signalSize) {
        if ((rawValue & (1ULL << (signalSize - 1))) && signalSize < 64) {
            // 符号扩展 - 将高位都置为1
            rawValue |= (~0ULL << signalSize);
        }
        return static_cast<int64_t>(rawValue);
    }

    // 字节取反辅助函数
    static constexpr uint8_t invert_u8(uint8_t x) { return static_cast<uint8_t>(~x); }

    // 获取信号的实际起始位（一般为DBC文件使用）
    static uint16_t getSignalActualStartBit(uint16_t startBit, uint16_t signalSize, bool isBigEndian);

    // LSB格式信号处理（Intel格式，小端）
    static int setSignalValueByLSB(uint8_t* data, uint8_t dataLen, uint16_t startBit,
                                  uint16_t signalSize, uint64_t value);
    static uint64_t getUnsignedSignalValueByLSB(const uint8_t* data, uint8_t dataLen,
                                               uint16_t startBit, uint16_t signalSize);
    static int64_t getSignedSignalValueByLSB(const uint8_t* data, uint8_t dataLen,
                                            uint16_t startBit, uint16_t signalSize);

    // MSB格式信号处理（Motorola格式，大端）
    static int setSignalValueByMSB(uint8_t* data, uint8_t dataLen, uint16_t startBit,
                                  uint16_t signalSize, uint64_t value);
    static uint64_t getUnsignedSignalValueByMSB(const uint8_t* data, uint8_t dataLen,
                                               uint16_t startBit, uint16_t signalSize);
    static int64_t getSignedSignalValueByMSB(const uint8_t* data, uint8_t dataLen,
                                            uint16_t startBit, uint16_t signalSize);

    // 基于位列表的信号处理
    static int setSignalValueByBitList(uint8_t* data, uint8_t dataLen,
                                      const std::vector<uint16_t>& bitList, uint64_t value);

    // 信号位计算辅助函数
    static std::unordered_set<uint16_t> getSignalActualSetBits(uint16_t startBit, uint16_t signalSize, bool isBigEndian);
    static std::vector<uint16_t> getSignalActualArrayBits(uint16_t startBit, uint16_t size, bool isBigEndian);

    //=============================================================================
    // Socket Network Operations
    //=============================================================================

    // 设置socket阻塞/非阻塞模式
    // @param fd: socket文件描述符
    // @param blocking: true=阻塞模式, false=非阻塞模式
    // @return: 0=成功, >0=错误码, -1=参数错误
    static int setSocketBlockingMode(int fd, bool blocking);

    // 非阻塞连接socket（带超时）
    // @param fd: socket文件描述符
    // @param ip: 目标IP地址
    // @param port: 目标端口
    // @param timeoutMs: 超时时间（毫秒）
    // @return: 0=成功, >0=错误码, <0=特定错误（-1=参数错误, -2=IP无效, -3=超时等）
    static int connectSocketNonBlocking(int fd, const std::string& ip, int port, int timeoutMs);

    // 设置socket关闭时的linger行为
    // @param fd: socket文件描述符
    // @param enable: 是否启用linger
    // @param seconds: linger超时时间（秒）
    // @return: true=成功, false=失败
    static bool setSocketLinger(int fd, bool enable, int seconds);

    // 优雅关闭socket
    // @param fd: socket文件描述符
    // @return: true=成功, false=失败
    static bool gracefullyCloseSocket(int fd);
};

}

#endif // COMMON_UTILS_H
