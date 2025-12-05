/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2022-2042 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v3.1.0
* Date: 2022-10-09
*----------------------------------------------------------------------------*/

/**
* @file crc_calculator.h
* @brief Comprehensive checksum calculation implementation supporting multiple industry standards.
*/

#ifndef COMMON_CRC_CALCULATOR_H
#define COMMON_CRC_CALCULATOR_H

#include "common_types.h"
#include "common_global.h"

namespace Common {

/**
 * @class CrcCalculator
 * @brief A utility class for calculating various checksums.
 */
class COMMON_API_EXPORT CrcCalculator {
public:
    static std::vector<std::pair<std::string, CrcAlgorithm>> getAlgorithms();
    static CrcConfig getAlgorithmDefaults(CrcAlgorithm algorithm);

    /**
     * @brief Calculates CRC based on start/end byte logic in config (max 255 bytes).
     * Used mainly for CAN signals processing where byte order might need reversal.
     */
    static uint32_t calculate(const CrcConfig& config, const uint8_t* data, uint8_t length);

    /**
     * @brief Calculates CRC for a linear buffer (File/Stream).
     * Ignores config.data_start_byte and config.data_end_byte.
     * Iterates strictly from 0 to length-1.
     *
     * @param config The CRC algorithm configuration.
     * @param data Pointer to the data buffer.
     * @param length Size of the buffer (supports > 255 bytes).
     * @return The calculated CRC value.
     */
    static uint32_t calculateBuffer(const CrcConfig& config, const uint8_t* data, size_t length);

private:
    // --- Legacy Internal Implementations (uint8_t length) ---
    static uint32_t calculateGeneric(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC8Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC16CCITTTable(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC32Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC8SAEJ1850CustomTable(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint8_t calculateSum(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint8_t calculateXor(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // --- Buffer Internal Implementations (size_t length) ---
    static uint32_t calculateBufferGeneric(const CrcConfig& config, const uint8_t* data, size_t length);

    // Overloaded table methods for size_t to reuse table data but support large loops
    static uint32_t calculateCRC8Table(const CrcConfig& config, const uint8_t* data, size_t length);
    static uint32_t calculateCRC16CCITTTable(const CrcConfig& config, const uint8_t* data, size_t length);
    static uint32_t calculateCRC32Table(const CrcConfig& config, const uint8_t* data, size_t length);
    static uint32_t calculateCRC8SAEJ1850CustomTable(const CrcConfig& config, const uint8_t* data, size_t length);

    // --- Shared Helper ---
    static uint32_t reflectBits(uint32_t value, int width);

    // Lookup Tables (Defined in crc_table.cpp)
    static const uint8_t CRC8_TABLE[256];
    static const uint16_t CRC16_CCITT_TABLE[256];
    static const uint32_t CRC32_TABLE[256];
    static const uint8_t CRC8_SAE_J1850_TABLE[256];
};

} // namespace Common

#endif // COMMON_CRC_CALCULATOR_H
