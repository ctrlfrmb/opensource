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
* @brief Comprehensive utility functions for common tasks and bit-level operations
*
* The Utils class provides a wide range of utility functions for common programming
* tasks, with particular emphasis on bit-level operations for CAN bus signal
* processing. It includes functions for crash handling, time formatting, checksum
* calculation, and various binary data manipulations.
*
* Features:
* - Cross-platform crash/exception handling
* - High-performance timestamp generation
* - Dynamic password generation based on time
* - Comprehensive bit-level operations for both big and little endian formats
* - Signal extraction and insertion for CAN frame processing
* - Hex string formatting and binary data conversion
* - File system operations with error handling
* - Process priority management
* - Thread identification utilities
*
* Usage example:
*   // Extract a signal from CAN data
*   uint64_t value = Common::Utils::getUnsignedSignalValueByLSB(
*       canData, dataLength, startBit, signalSize);
*
*   // Format binary data as hex string
*   std::string hexData = Common::Utils::bytesToHexString(data, dataLength);
*/


// commonutils.h
#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <vector>
#include <string>
#include <unordered_set>

#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT Utils {
public:

    // 崩溃/异常捕获
    typedef void (*CrashHandlerCallback)();

    // 注册崩溃处理回调（如关闭设备等）
    static void registerCrashHandler(CrashHandlerCallback cb);

    // 启用全局崩溃/异常捕获（main函数最早调用）
    static void setupCrashHandler();

    static const char* getCurrentTimeStringFast();

    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);

    static std::string bytesToHexString(const uint8_t* data, size_t length);

    static std::string getThreadIdString();

    static void setProcessHighPriority();

    // 辅助函数：符号扩展
    static inline int64_t signalRawValueToSigned(uint64_t rawValue, uint16_t signalSize) {
        if ((rawValue & (1ULL << (signalSize - 1))) && signalSize < 64) {
            // 符号扩展 - 将高位都置为1
            rawValue |= (~0ULL << signalSize);
        }
        return static_cast<int64_t>(rawValue);
    }

    // 获取信号的实际起始位（一般为DBC文件使用）
    static uint16_t getSignalActualStartBit(uint16_t startBit, uint16_t signalSize, bool isBigEndian);

    // 设置LSB格式信号值（Intel格式，小端）
    static int setSignalValueByLSB(uint8_t* data, uint8_t dataLen, uint16_t startBit,
                                  uint16_t signalSize, uint64_t value);

    // 获取LSB格式无符号信号值（Intel格式，小端）
    static uint64_t getUnsignedSignalValueByLSB(const uint8_t* data, uint8_t dataLen,
                                        uint16_t startBit, uint16_t signalSize);
    // 获取LSB格式有符号信号值（Intel格式，小端）
    static int64_t getSignedSignalValueByLSB(const uint8_t* data, uint8_t dataLen,
                                             uint16_t startBit, uint16_t signalSize);

    // 设置MSB格式信号值（Motorola格式，大端）
    static int setSignalValueByMSB(uint8_t* data, uint8_t dataLen, uint16_t startBit,
                                  uint16_t signalSize, uint64_t value);

    // 获取MSB格式无符号信号值（Motorola格式，大端）
    static uint64_t getUnsignedSignalValueByMSB(const uint8_t* data, uint8_t dataLen,
                                        uint16_t startBit, uint16_t signalSize);
    // 获取MSB格式有符号信号值（Motorola格式，大端）
    static int64_t getSignedSignalValueByMSB(const uint8_t* data, uint8_t dataLen, uint16_t startBit, uint16_t signalSize);

    static constexpr uint8_t invert_u8(uint8_t x) { return static_cast<uint8_t>(~x); }

    static int setSignalValueByBitList(uint8_t* data, uint8_t dataLen, const std::vector<uint16_t>& bitList, uint64_t value);

    static std::unordered_set<uint16_t> getSignalActualSetBits(uint16_t startBit, uint16_t signalSize, bool isBigEndian);

    static std::vector<uint16_t> getSignalActualArrayBits(uint16_t startBit, uint16_t size, bool isBigEndian);

    static bool ensureFilePath(const std::string& filePath);

    static int setSocketBlockingMode(int fd, bool blocking);
    static int connectSocketNonBlocking(int fd, const std::string& ip, int port, int timeoutMs);
    static bool setSocketLinger(int fd, bool enable, int seconds);
    static bool gracefullyCloseSocket(int fd);

};

}

#endif // COMMON_UTILS_H
