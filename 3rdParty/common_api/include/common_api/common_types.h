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
