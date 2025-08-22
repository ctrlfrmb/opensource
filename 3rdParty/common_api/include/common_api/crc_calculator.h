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
* Version: v1.4.0 (Refactored CrcConfig for clarity)
* Date: 2022-08-21
*----------------------------------------------------------------------------*/

/**
* @file crc_calculator.h
* @brief Comprehensive CRC calculation implementation supporting multiple industry standards.
*
* The CrcCalculator class provides a robust implementation of various CRC algorithms.
* It features a highly configurable, unified interface for calculation and in-place
* buffer updates, supporting a wide range of industry standards.
*
* Features:
* - Support for 23 different CRC algorithms and standards.
* - Logically grouped and clearly named configuration parameters.
* - High-performance table-driven implementations for common algorithms.
* - Configurable data range for CRC calculation.
* - Endianness control for writing CRC results back to buffers.
*
* Usage example for standard CRC:
*   Common::CrcConfig config;
*   config.algorithm = Common::CrcAlgorithm::CRC16_MODBUS;
*   config.data_start_byte = 0;
*   config.data_end_byte = 5;
*   uint32_t crcValue = Common::CrcCalculator::calculate(config, data, dataLength);
*
* Usage example for CRC-8/SAE-J1850-ZERO:
*   Common::CrcConfig config;
*   config.algorithm = Common::CrcAlgorithm::CRC8_SAE_J1850_ZERO;
*   config.message_id = 0x345; // Set the CAN message ID
*   config.data_start_byte = 0;
*   config.data_end_byte = 6;
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
    CRC32_MPEG_2,
    CRC8_SAE_J1850_ZERO
};

// CRC配置结构体 (重构后)
struct CrcConfig {
    // --- 1. Core Algorithm Parameters ---
    CrcAlgorithm algorithm = CrcAlgorithm::CRC8;    ///< The selected CRC standard.
    uint8_t width_bits = 8;                         ///< Mathematical width of the CRC in bits (e.g., 8, 16, 32).
    uint32_t polynomial = 0;                        ///< The generator polynomial.
    uint32_t initial_value = 0;                     ///< Initial value of the CRC register.
    uint32_t final_xor_value = 0;                   ///< Value to XOR with the final CRC result.
    bool input_reflected = false;                   ///< Specifies if input bytes are reflected before processing.
    bool output_reflected = false;                  ///< Specifies if the final CRC result is reflected before XORing.

    // --- 2. Data Processing Range ---
    uint8_t data_start_byte = 0;                    ///< The starting byte index of the data to be checksummed.
    uint8_t data_end_byte = 7;                      ///< The ending byte index (inclusive) of the data.

    // --- 3. Result Placement in Buffer ---
    uint16_t crc_bit_position = 0;                  ///< The starting bit position to write the CRC result in the data buffer.
    bool result_is_big_endian = false;              ///< Endianness for writing the multi-byte CRC result into the buffer.

    // --- 4. Contextual Parameters for Specific Algorithms ---
    uint32_t message_id = 0;                        ///< Contextual ID, e.g., for SAE J1850.
};

// CRC计算工具类
class COMMON_API_EXPORT CrcCalculator {
public:
    // 主入口函数：根据配置计算CRC
    static uint32_t calculate(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // 更新数据中的CRC字段（按配置写入data）
    static void updateCRCData(const CrcConfig& config, uint8_t* data, uint8_t length);

    // 算法名称转换为字符串
    static QString algorithmToString(CrcAlgorithm algo);

    // 字符串转换为算法枚举
    static CrcAlgorithm stringToAlgorithm(const QString &str);

private:
    static uint32_t reflectBits(uint32_t value, int width);
    static uint32_t calculateGeneric(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // 针对高频算法的表驱动实现
    static uint32_t calculateCRC8Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC16CCITTTable(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC32Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC8SAEJ1850Table(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // 预计算的查找表
    static const uint8_t CRC8_TABLE[256];
    static const uint16_t CRC16_CCITT_TABLE[256];
    static const uint32_t CRC32_TABLE[256];
    static const uint8_t CRC8_SAE_J1850_TABLE[256];
};

} // namespace Common

#endif // COMMON_CRC_CALCULATOR_H
