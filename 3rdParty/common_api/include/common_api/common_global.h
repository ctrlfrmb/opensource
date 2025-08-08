#ifndef COMMON_GLOBAL_H
#define COMMON_GLOBAL_H

#include <QtCore/qglobal.h>
#include <vector>

#if defined(BUILD_COMMON_API)
#  define COMMON_API_EXPORT Q_DECL_EXPORT
#else
#  define COMMON_API_EXPORT Q_DECL_IMPORT
#endif


namespace Common {

struct SendFrame {
    uint64_t key{0}; //type(uint8_t) + group(uint8_t) + message id/row(uint32_t)
    std::vector<char> data; // send data
    uint32_t period{50};    // Send cycle time (ms)
    uint32_t delay{0};      // Delay sending time (ms)
};

using SendQueue = std::vector<SendFrame>;

}

#endif // COMMON_GLOBAL_H
