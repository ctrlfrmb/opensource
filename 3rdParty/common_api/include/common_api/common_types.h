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
* Version: v1.0.0
* Date: 2022-01-12
*----------------------------------------------------------------------------*/

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <vector>
#include <functional>

namespace Common {

/**
 * @brief Timer strategy enumeration
 */
typedef enum {
    TIMER_STRATEGY_AUTO = 0,                    // Auto-select based on interval
    TIMER_STRATEGY_LOW_FREQUENCY,               // Kernel timer for low frequency (> 1ms)
    TIMER_STRATEGY_HIGH_FREQUENCY_SLEEP,        // Hybrid strategy with kernel sleep + busy-wait
    TIMER_STRATEGY_HIGH_FREQUENCY_BUSY_WAIT     // Pure busy-wait for maximum precision
} TimerStrategy;

/**
 * @enum CrcAlgorithm
 * @brief Defines the supported checksum algorithms.
 */
enum class CrcAlgorithm : uint8_t {
    // --- Custom Checksums ---
    CUSTOM_SUM,
    CUSTOM_XOR,
    CUSTOM_CRC,

    // --- Standard CRC Algorithms ---
    CRC4_ITU,
    CRC5_EPC,
    CRC5_ITU,
    CRC5_USB,
    CRC6_ITU,
    CRC7_MMC,
    CRC8_STANDARD,
    CRC8_SAE_J1850,
    CRC8_SAE_J1850_CUSTOM,
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
 */
struct CrcConfig {
    // --- 1. Core Algorithm Parameters ---
    CrcAlgorithm algorithm = CrcAlgorithm::CRC8_STANDARD;
    uint8_t width_bits = 8;
    uint32_t polynomial = 0;
    uint32_t initial_value = 0;
    uint32_t final_xor_value = 0;
    bool input_reflected = false;
    bool output_reflected = false;

    // --- 2. Data Processing Range (Used ONLY for Legacy calculate() interface) ---
    uint8_t data_start_byte = 0;
    uint8_t data_end_byte = 6;

    // --- 3. CRC Signal Placement ---
    uint16_t signal_start_bit = 56;
    bool signal_is_big_endian = false;

    // --- 4. Contextual Parameters for Specific Algorithms ---
    uint32_t message_id = 0;
};

struct SendFrame {
    uint64_t key{0}; //type(uint8_t) + group(uint8_t) + message id/row(uint32_t)
    std::vector<char> data; // send data
    uint32_t period{50};    // Send cycle time (ms)
    uint64_t delay{0};      // Delay sending time (ms)
};

using SendQueue = std::vector<SendFrame>;
using SendCallback = std::function<int(const std::vector<char>&, int)>;

}

#endif // COMMON_TYPES_H
