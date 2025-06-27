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
* Version: v1.2.0
* Date: 2022-07-23
*----------------------------------------------------------------------------*/

/**
* @file crc_calculator.h
* @brief Comprehensive CRC calculation implementation supporting multiple industry standards
*
* The CrcCalculator class provides a robust implementation of various CRC algorithms
* used in communication protocols and data verification. It supports a wide range of
* standard CRC variants including CRC-8, CRC-16, and CRC-32 families with optimized
* table-driven implementations for the most common algorithms.
*
* Features:
* - Support for 21 different CRC algorithms and standards
* - Configurable parameters including polynomial, init value, reflection settings
* - High-performance table-driven implementations for common algorithms
* - Byte position and range selection for CRC calculation
* - Big and little endian support for input/output formatting
* - In-place CRC calculation and updating for data buffers
*
* Usage example:
*   Common::CrcConfig config;
*   config.algorithm = Common::CrcAlgorithm::CRC16_MODBUS;
*   config.start_byte = 0;
*   config.end_byte = 5;
*
*   uint32_t crcValue = Common::CrcCalculator::calculate(config, data, dataLength);
*/

#ifndef COMMON_CRC_CALCULATOR_H
#define COMMON_CRC_CALCULATOR_H

#include <cstdint>
#include <vector>
#include <QString>
#include <unordered_map>

#include "common_global.h"

namespace Common {

// CRC算法枚举
enum class CrcAlgorithm : uint8_t {
    CRC4_ITU,
    CRC5_EPC,
    CRC5_ITU,
    CRC5_USB,
    CRC6_ITU,
    CRC7_MMC,
    CRC8,
    CRC8_ITU,
    CRC8_ROHC,
    CRC8_MAXIM,
    CRC16_IBM,
    CRC16_MAXIM,
    CRC16_USB,
    CRC16_MODBUS,
    CRC16_CCITT,
    CRC16_CCITT_FALSE,
    CRC16_X25,
    CRC16_XMODEM,
    CRC16_DNP,
    CRC32,
    CRC32_MPEG_2
};

// CRC配置结构体
struct CrcConfig {
    uint32_t crc_byte_pos = 0;                          // CRC值起始位(bit)
    uint32_t crc_byte_len = 8;                          // CRC值长度(bit)
    bool isBigEndian = false;                           // 是否为大端
    CrcAlgorithm algorithm = CrcAlgorithm::CRC8;        // CRC算法类型
    uint32_t polynomial = 0;                            // 多项式
    uint32_t init_value = 0;                            // 初始值
    bool input_reflected = false;                       // 输入反转
    bool output_reflected = false;                      // 输出反转
    uint32_t xor_out = 0;                               // 结果异或值
    int start_byte = 0;                                 // 起始字节
    int end_byte = 7;                                   // 结束字节
    int width = 8;                                      // CRC宽度(bit)
};

// CRC计算工具类
class COMMON_API_EXPORT CrcCalculator {
public:
    // 主入口函数：根据配置计算CRC
    static uint32_t calculate(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // 更新数据中的CRC字段（按配置写入data）
    static void updateCRCData(const CrcConfig& config, uint8_t* data, uint8_t length);

    // 算法名称转换为字符串
    static QString algorithmToString(CrcAlgorithm algo) {
        switch(algo) {
            case CrcAlgorithm::CRC4_ITU: return "CRC-4/ITU";
            case CrcAlgorithm::CRC5_EPC: return "CRC-5/EPC";
            case CrcAlgorithm::CRC5_ITU: return "CRC-5/ITU";
            case CrcAlgorithm::CRC5_USB: return "CRC-5/USB";
            case CrcAlgorithm::CRC6_ITU: return "CRC-6/ITU";
            case CrcAlgorithm::CRC7_MMC: return "CRC-7/MMC";
            case CrcAlgorithm::CRC8: return "CRC-8";
            case CrcAlgorithm::CRC8_ITU: return "CRC-8/ITU";
            case CrcAlgorithm::CRC8_ROHC: return "CRC-8/ROHC";
            case CrcAlgorithm::CRC8_MAXIM: return "CRC-8/MAXIM";
            case CrcAlgorithm::CRC16_IBM: return "CRC-16/IBM";
            case CrcAlgorithm::CRC16_MAXIM: return "CRC-16/MAXIM";
            case CrcAlgorithm::CRC16_USB: return "CRC-16/USB";
            case CrcAlgorithm::CRC16_MODBUS: return "CRC-16/MODBUS";
            case CrcAlgorithm::CRC16_CCITT: return "CRC-16/CCITT";
            case CrcAlgorithm::CRC16_CCITT_FALSE: return "CRC-16/CCITT-FALSE";
            case CrcAlgorithm::CRC16_X25: return "CRC-16/X25";
            case CrcAlgorithm::CRC16_XMODEM: return "CRC-16/XMODEM";
            case CrcAlgorithm::CRC16_DNP: return "CRC-16/DNP";
            case CrcAlgorithm::CRC32: return "CRC-32";
            case CrcAlgorithm::CRC32_MPEG_2: return "CRC-32/MPEG-2";
        }
        return "CRC-8";
    }

    // 字符串转换为算法枚举
    static CrcAlgorithm stringToAlgorithm(const QString &str) {
        if(str == "CRC-4/ITU") return CrcAlgorithm::CRC4_ITU;
        if(str == "CRC-5/EPC") return CrcAlgorithm::CRC5_EPC;
        if(str == "CRC-5/ITU") return CrcAlgorithm::CRC5_ITU;
        if(str == "CRC-5/USB") return CrcAlgorithm::CRC5_USB;
        if(str == "CRC-6/ITU") return CrcAlgorithm::CRC6_ITU;
        if(str == "CRC-7/MMC") return CrcAlgorithm::CRC7_MMC;
        if(str == "CRC-8") return CrcAlgorithm::CRC8;
        if(str == "CRC-8/ITU") return CrcAlgorithm::CRC8_ITU;
        if(str == "CRC-8/ROHC") return CrcAlgorithm::CRC8_ROHC;
        if(str == "CRC-8/MAXIM") return CrcAlgorithm::CRC8_MAXIM;
        if(str == "CRC-16/IBM") return CrcAlgorithm::CRC16_IBM;
        if(str == "CRC-16/MAXIM") return CrcAlgorithm::CRC16_MAXIM;
        if(str == "CRC-16/USB") return CrcAlgorithm::CRC16_USB;
        if(str == "CRC-16/MODBUS") return CrcAlgorithm::CRC16_MODBUS;
        if(str == "CRC-16/CCITT") return CrcAlgorithm::CRC16_CCITT;
        if(str == "CRC-16/CCITT-FALSE") return CrcAlgorithm::CRC16_CCITT_FALSE;
        if(str == "CRC-16/X25") return CrcAlgorithm::CRC16_X25;
        if(str == "CRC-16/XMODEM") return CrcAlgorithm::CRC16_XMODEM;
        if(str == "CRC-16/DNP") return CrcAlgorithm::CRC16_DNP;
        if(str == "CRC-32") return CrcAlgorithm::CRC32;
        if(str == "CRC-32/MPEG-2") return CrcAlgorithm::CRC32_MPEG_2;
        return CrcAlgorithm::CRC8;
    }

private:
    static uint32_t reflectBits(uint32_t value, int width);
    static uint32_t calculateGeneric(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // 针对高频算法的表驱动实现
    static uint32_t calculateCRC8Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC16CCITTTable(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC32Table(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // 预计算的查找表
    static const uint8_t CRC8_TABLE[256];
    static const uint16_t CRC16_CCITT_TABLE[256];
    static const uint32_t CRC32_TABLE[256];
};

} // namespace Common

#endif // COMMON_CRC_CALCULATOR_H
