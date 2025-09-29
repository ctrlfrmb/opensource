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
* Version: v1.0.0
* Date: 2022-01-12
*----------------------------------------------------------------------------*/

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <vector>
#include "common_global.h"

namespace Common {

struct SendFrame {
    uint64_t key{0}; //type(uint8_t) + group(uint8_t) + message id/row(uint32_t)
    std::vector<char> data; // send data
    uint32_t period{50};    // Send cycle time (ms)
    uint64_t delay{0};      // Delay sending time (ms)
};

using SendQueue = std::vector<SendFrame>;

}

#endif // COMMON_TYPES_H
