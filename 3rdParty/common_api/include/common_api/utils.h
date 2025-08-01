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
 * @code
 *   // Extract a CAN signal
 *   uint64_t value = Common::Utils::getUnsignedSignalValueByLSB(
 *       canData, dataLength, startBit, signalSize);
 *
 *   // Connect socket with timeout
 *   int result = Common::Utils::connectSocketNonBlocking(fd, "192.168.1.100", 8080, 3000);
 * @endcode
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

    /**
     * @brief 崩溃/异常处理回调函数类型
     */
    typedef void (*CrashHandlerCallback)();

    /**
     * @brief 注册崩溃处理回调（如关闭设备等）
     * @param cb 崩溃处理回调函数指针
     */
    static void registerCrashHandler(CrashHandlerCallback cb);

    /**
     * @brief 启用全局崩溃/异常捕获（main函数最早调用）
     */
    static void setupCrashHandler();

    /**
     * @brief 获取当前线程ID字符串
     * @return 线程ID的字符串表示
     */
    static std::string getThreadIdString();

    /**
     * @brief 设置进程为高优先级
     */
    static void setProcessHighPriority();

    //=============================================================================
    // Time & Data Processing
    //=============================================================================

    /**
     * @brief 获取从程序启动到现在的毫秒数
     * @return 毫秒数
     */
    static uint64_t getCurrentMillisecondsFast();

    /**
     * @brief 高性能时间字符串获取（使用线程本地存储）
     * @return 时间字符串指针
     */
    static const char* getCurrentTimeStringFast();

    /**
     * @brief 计算数据校验和
     * @param data 数据向量
     * @return 校验和值
     */
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);

    /**
     * @brief 将字节数组转换为十六进制字符串
     * @param data 字节数组指针
     * @param length 数组长度
     * @return 十六进制字符串
     */
    static std::string bytesToHexString(const uint8_t* data, size_t length);

    /**
     * @brief 确保文件路径存在（创建必要的目录）
     * @param filePath 文件路径
     * @return true=成功, false=失败
     */
    static bool ensureFilePath(const std::string& filePath);

    /**
     * @brief 判断文件是否存在
     * @param filePath 文件路径
     * @return true=存在, false=不存在
     */
    static bool fileExists(const std::string& filePath);

    //=============================================================================
    // CAN Bus Signal Processing
    //=============================================================================

    /**
     * @brief 符号扩展辅助函数
     * @param rawValue 原始值
     * @param signalSize 信号大小（位数）
     * @return 符号扩展后的有符号值
     */
    static inline int64_t signalRawValueToSigned(uint64_t rawValue, uint16_t signalSize) {
        if ((rawValue & (1ULL << (signalSize - 1))) && signalSize < 64) {
            // 符号扩展 - 将高位都置为1
            rawValue |= (~0ULL << signalSize);
        }
        return static_cast<int64_t>(rawValue);
    }

    /**
     * @brief 字节取反辅助函数
     * @param x 输入字节
     * @return 取反后的字节
     */
    static constexpr uint8_t invert_u8(uint8_t x) { return static_cast<uint8_t>(~x); }

    /**
     * @brief 获取信号的实际起始位（一般为DBC文件使用）
     * @param startBit 起始位
     * @param signalSize 信号大小
     * @param isBigEndian 是否为大端格式
     * @return 实际起始位
     */
    static uint16_t getSignalActualStartBit(uint16_t startBit, uint16_t signalSize, bool isBigEndian);

    /**
     * @brief LSB格式设置信号值（Intel格式，小端）
     * @param data 数据缓冲区
     * @param dataLen 数据长度
     * @param startBit 起始位
     * @param signalSize 信号大小
     * @param value 要设置的值
     * @return 0=成功, <0=失败
     */
    static int setSignalValueByLSB(uint8_t* data, uint8_t dataLen, uint16_t startBit,
                                  uint16_t signalSize, uint64_t value);

    /**
     * @brief LSB格式获取无符号信号值（Intel格式，小端）
     * @param data 数据缓冲区
     * @param dataLen 数据长度
     * @param startBit 起始位
     * @param signalSize 信号大小
     * @return 无符号信号值
     */
    static uint64_t getUnsignedSignalValueByLSB(const uint8_t* data, uint8_t dataLen,
                                               uint16_t startBit, uint16_t signalSize);

    /**
     * @brief LSB格式获取有符号信号值（Intel格式，小端）
     * @param data 数据缓冲区
     * @param dataLen 数据长度
     * @param startBit 起始位
     * @param signalSize 信号大小
     * @return 有符号信号值
     */
    static int64_t getSignedSignalValueByLSB(const uint8_t* data, uint8_t dataLen,
                                            uint16_t startBit, uint16_t signalSize);

    /**
     * @brief MSB格式设置信号值（Motorola格式，大端）
     * @param data 数据缓冲区
     * @param dataLen 数据长度
     * @param startBit 起始位
     * @param signalSize 信号大小
     * @param value 要设置的值
     * @return 0=成功, <0=失败
     */
    static int setSignalValueByMSB(uint8_t* data, uint8_t dataLen, uint16_t startBit,
                                  uint16_t signalSize, uint64_t value);

    /**
     * @brief MSB格式获取无符号信号值（Motorola格式，大端）
     * @param data 数据缓冲区
     * @param dataLen 数据长度
     * @param startBit 起始位
     * @param signalSize 信号大小
     * @return 无符号信号值
     */
    static uint64_t getUnsignedSignalValueByMSB(const uint8_t* data, uint8_t dataLen,
                                               uint16_t startBit, uint16_t signalSize);

    /**
     * @brief MSB格式获取有符号信号值（Motorola格式，大端）
     * @param data 数据缓冲区
     * @param dataLen 数据长度
     * @param startBit 起始位
     * @param signalSize 信号大小
     * @return 有符号信号值
     */
    static int64_t getSignedSignalValueByMSB(const uint8_t* data, uint8_t dataLen,
                                            uint16_t startBit, uint16_t signalSize);

    /**
     * @brief 基于位列表的信号设置
     * @param data 数据缓冲区
     * @param dataLen 数据长度
     * @param bitList 位列表
     * @param value 要设置的值
     * @return 0=成功, <0=失败
     */
    static int setSignalValueByBitList(uint8_t* data, uint8_t dataLen,
                                      const std::vector<uint16_t>& bitList, uint64_t value);

    /**
     * @brief 获取信号实际位集合
     * @param startBit 起始位
     * @param signalSize 信号大小
     * @param isBigEndian 是否为大端格式
     * @return 位集合
     */
    static std::unordered_set<uint16_t> getSignalActualSetBits(uint16_t startBit, uint16_t signalSize, bool isBigEndian);

    /**
     * @brief 获取信号实际位数组
     * @param startBit 起始位
     * @param size 信号大小
     * @param isBigEndian 是否为大端格式
     * @return 位数组
     */
    static std::vector<uint16_t> getSignalActualArrayBits(uint16_t startBit, uint16_t size, bool isBigEndian);

    //=============================================================================
    // Socket Network Operations
    //=============================================================================

    /**
     * @brief 设置socket阻塞/非阻塞模式
     * @param fd socket文件描述符
     * @param blocking true=阻塞模式, false=非阻塞模式
     * @return 0=成功, >0=错误码, -1=参数错误
     */
    static int setSocketBlockingMode(int fd, bool blocking);

    /**
     * @brief 非阻塞连接socket（带超时）
     * @param fd socket文件描述符
     * @param ip 目标IP地址
     * @param port 目标端口
     * @param timeoutMs 超时时间（毫秒）
     * @return 0=成功, >0=错误码, <0=特定错误（-1=参数错误, -2=IP无效, -3=超时等）
     */
    static int connectSocketNonBlocking(int fd, const std::string& ip, int port, int timeoutMs);

    /**
     * @brief 设置socket关闭时的linger行为
     * @param fd socket文件描述符
     * @param enable 是否启用linger
     * @param seconds linger超时时间（秒）
     * @return true=成功, false=失败
     */
    static bool setSocketLinger(int fd, bool enable, int seconds);

    /**
     * @brief 优雅关闭socket
     * @param fd socket文件描述符
     * @return true=成功, false=失败
     */
    static bool gracefullyCloseSocket(int fd);
};

}

#endif // COMMON_UTILS_H
