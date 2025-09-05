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
* Date: 2022-08-28
*----------------------------------------------------------------------------*/

/**
* @file crc_calculator.h
* @brief Comprehensive checksum calculation implementation supporting multiple industry standards.
*
* The CrcCalculator class provides a robust implementation of various CRC, SUM, and XOR algorithms.
* It features a highly configurable, unified interface for calculating checksum values based on
* a wide range of industry standards.
*
* Features:
* - Support for 23 different CRC algorithms and standards.
* - Support for 8-bit Summation (SUM/AND) and XOR checksums.
* - Logically grouped and clearly named configuration parameters.
* - High-performance table-driven implementations for common algorithms.
* - Configurable data range for calculation.
*/

#ifndef COMMON_CRC_CALCULATOR_H
#define COMMON_CRC_CALCULATOR_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include "common_global.h"

namespace Common {

// Algorithm enumeration
enum class CrcAlgorithm : uint8_t {
    CUSTOM_SUM,
    CUSTOM_XOR,
    CRC4_ITU,
    CRC5_EPC,
    CRC5_ITU,
    CRC5_USB,
    CRC6_ITU,
    CRC7_MMC,
    CRC8,
    CRC8_SAE_J1850_ZERO,
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

// CRC configuration structure
struct CrcConfig {
    // --- 1. Core Algorithm Parameters ---
    CrcAlgorithm algorithm = CrcAlgorithm::CRC8;    ///< The selected standard.
    uint8_t width_bits = 8;                         ///< Mathematical width in bits (e.g., 8, 16, 32).
    uint32_t polynomial = 0;                        ///< The generator polynomial.
    uint32_t initial_value = 0;                     ///< Initial value of the register.
    uint32_t final_xor_value = 0;                   ///< Value to XOR with the final result.
    bool input_reflected = false;                   ///< Specifies if input bytes are reflected before processing.
    bool output_reflected = false;                  ///< Specifies if the final result is reflected before XORing.

    // --- 2. Data Processing Range ---
    uint8_t data_start_byte = 0;                    ///< The starting byte index of the data to be checksummed.
    uint8_t data_end_byte = 7;                      ///< The ending byte index (inclusive) of the data.

    // --- 3. CRC Signal Placement ---
    uint16_t signal_start_bit = 0;                  ///< The original start bit of the CRC signal as defined in the DBC file.
    bool signal_is_big_endian = false;              ///< The bit order of the CRC signal itself (true for Motorola, false for Intel).

    // --- 4. Contextual Parameters for Specific Algorithms ---
    uint32_t message_id = 0;                        ///< Contextual ID, e.g., for SAE J1850.
};

// Checksum calculation utility class
class COMMON_API_EXPORT CrcCalculator {
public:
    // Get all supported algorithms
    static std::vector<std::pair<std::string, CrcAlgorithm>> getAlgorithms();

    // Main entry function: Calculate checksum based on configuration
    static uint32_t calculate(const CrcConfig& config, const uint8_t* data, uint8_t length);

private:
    static uint32_t reflectBits(uint32_t value, int width);
    static uint32_t calculateGeneric(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // Table-driven implementations for high-frequency algorithms
    static uint32_t calculateCRC8Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC16CCITTTable(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC32Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC8SAEJ1850Table(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // Private calculation functions for SUM and XOR
    static uint8_t calculateSum(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint8_t calculateXor(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // Pre-computed lookup tables
    static const uint8_t CRC8_TABLE[256];
    static const uint16_t CRC16_CCITT_TABLE[256];
    static const uint32_t CRC32_TABLE[256];
    static const uint8_t CRC8_SAE_J1850_TABLE[256];
};

} // namespace Common

#endif // COMMON_CRC_CALCULATOR_H
