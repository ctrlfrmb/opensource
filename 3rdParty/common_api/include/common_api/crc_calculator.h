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
* Version: v3.0.0
* Date: 2022-05-22
*----------------------------------------------------------------------------*/

/**
* @file crc_calculator.h
* @brief Comprehensive checksum calculation implementation supporting multiple industry standards.
*
* The CrcCalculator class provides a robust implementation of various CRC, SUM, and XOR algorithms.
* It features a highly configurable, unified interface for calculating checksum values based on
* a wide range of industry standards. The class architecture separates algorithm definition from
* calculation logic, ensuring correctness and flexibility.
*
* Features:
* - Support for numerous standard CRC algorithms with pre-configured parameters.
* - A fully customizable CRC option for non-standard implementations.
* - Support for 8-bit Summation (SUM) and XOR checksums.
* - High-performance table-driven implementations for common algorithms.
* - Clear separation of concerns: UI can query default parameters from this class.
*/

#ifndef COMMON_CRC_CALCULATOR_H
#define COMMON_CRC_CALCULATOR_H

#include <cstdint>
#include <vector>
#include <string>
#include "common_global.h"

namespace Common {

/**
 * @enum CrcAlgorithm
 * @brief Defines the supported checksum algorithms.
 *
 * This enumeration includes standard industry CRC algorithms, custom checksums,
 * and a fully customizable CRC option for advanced use cases.
 */
enum class CrcAlgorithm : uint8_t {
    // --- Custom Checksums ---
    CUSTOM_SUM,                 ///< Custom 8-bit summation.
    CUSTOM_XOR,                 ///< Custom 8-bit XOR.
    CUSTOM_CRC,                 ///< Fully configurable CRC parameters.

    // --- Standard CRC Algorithms ---
    CRC4_ITU,
    CRC5_EPC,
    CRC5_ITU,
    CRC5_USB,
    CRC6_ITU,
    CRC7_MMC,
    CRC8_STANDARD,              ///< Standard CRC-8 (Poly=0x07). Renamed from CRC8.
    CRC8_SAE_J1850,             ///< Pure SAE J1850 standard (Poly=0x1D, Init=0xFF, XorOut=0xFF).
    CRC8_SAE_J1850_CUSTOM,      ///< Custom SAE J1850 for specific applications (e.g., with Message ID). Renamed from CRC8_SAE_J1850_ZERO.
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

/**
 * @struct CrcConfig
 * @brief Holds all parameters required for a checksum calculation.
 *
 * This structure is used to define the behavior of the checksum algorithm,
 * including core parameters, data range, and contextual information.
 */
struct CrcConfig {
    // --- 1. Core Algorithm Parameters ---
    CrcAlgorithm algorithm = CrcAlgorithm::CRC8_STANDARD; ///< The selected standard.
    uint8_t width_bits = 8;                         ///< Mathematical width in bits (e.g., 8, 16, 32).
    uint32_t polynomial = 0;                        ///< The generator polynomial.
    uint32_t initial_value = 0;                     ///< Initial value of the register.
    uint32_t final_xor_value = 0;                   ///< Value to XOR with the final result.
    bool input_reflected = false;                   ///< Specifies if input bytes are reflected before processing.
    bool output_reflected = false;                  ///< Specifies if the final result is reflected before XORing.

    // --- 2. Data Processing Range ---
    uint8_t data_start_byte = 0;                    ///< The starting byte index of the data to be checksummed.
    uint8_t data_end_byte = 6;                      ///< The ending byte index (inclusive) of the data.

    // --- 3. CRC Signal Placement ---
    uint16_t signal_start_bit = 56;                 ///< The original start bit of the CRC signal as defined in the DBC file.
    bool signal_is_big_endian = false;              ///< The bit order of the CRC signal itself (true for Motorola, false for Intel).

    // --- 4. Contextual Parameters for Specific Algorithms ---
    uint32_t message_id = 0;                        ///< Contextual ID, e.g., for CRC8_SAE_J1850_CUSTOM.
};

/**
 * @class CrcCalculator
 * @brief A utility class for calculating various checksums.
 *
 * Provides static methods to get algorithm information, default configurations,
 * and perform checksum calculations.
 */
class COMMON_API_EXPORT CrcCalculator {
public:
    /**
     * @brief Get a list of all supported algorithms with their user-friendly names.
     * @return A vector of pairs, where each pair contains the algorithm name and its corresponding enum value.
     */
    static std::vector<std::pair<std::string, CrcAlgorithm>> getAlgorithms();

    /**
     * @brief Get the default parameter configuration for a given standard algorithm.
     * @param algorithm The algorithm for which to get the default parameters.
     * @return A CrcConfig structure populated with the standard's default values.
     */
    static CrcConfig getAlgorithmDefaults(CrcAlgorithm algorithm);

    /**
     * @brief The main entry point for calculating a checksum.
     * @param config The CrcConfig structure defining the calculation parameters.
     * @param data A pointer to the byte array to be processed.
     * @param length The total length of the data array.
     * @return The calculated checksum value.
     */
    static uint32_t calculate(const CrcConfig& config, const uint8_t* data, uint8_t length);

private:
    // Generic bit-wise calculation, serves as a fallback for non-optimized algorithms.
    static uint32_t calculateGeneric(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // Helper function to reflect the bits of a value.
    static uint32_t reflectBits(uint32_t value, int width);

    // Table-driven implementations for high-performance calculation of common algorithms.
    static uint32_t calculateCRC8Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC16CCITTTable(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC32Table(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint32_t calculateCRC8SAEJ1850CustomTable(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // Private calculation functions for SUM and XOR.
    static uint8_t calculateSum(const CrcConfig& config, const uint8_t* data, uint8_t length);
    static uint8_t calculateXor(const CrcConfig& config, const uint8_t* data, uint8_t length);

    // Pre-computed lookup tables.
    static const uint8_t CRC8_TABLE[256];
    static const uint16_t CRC16_CCITT_TABLE[256];
    static const uint32_t CRC32_TABLE[256];
    static const uint8_t CRC8_SAE_J1850_TABLE[256];
};

} // namespace Common

#endif // COMMON_CRC_CALCULATOR_H
