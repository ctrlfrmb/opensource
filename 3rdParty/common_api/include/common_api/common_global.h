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
    uint32_t period{50};  // Send cycle time (ms)
    uint32_t interval{0};  // 1. When sending sequentially, it indicates the sending interval.
                           // 2. When sending periodically, it indicates the first frame delay time
};

using SendQueue = std::vector<SendFrame>;

/**
 * @brief 构造通用Key
 * @param type 类型 (8位)
 * @param group 组 (8位)
 * @param messageId/row 消息ID (32位)
 * @return 64位通用Key
 */
COMMON_API_EXPORT uint64_t makeUtilsKey(uint8_t type, uint8_t group, uint32_t messageId);

/**
 * @brief 解析通用Key
 * @param key 64位通用Key
 * @param type 输出类型
 * @param group 输出组
 * @param messageId/row 输出消息ID
 */
COMMON_API_EXPORT void parseUtilsKey(uint64_t key, uint8_t& type, uint8_t& group, uint32_t& messageId);

/**
 * @brief 解析通用送Key (简化版本)
 * @param key 64位通用Key
 * @param type 输出类型
 * @param group 输出组
 */
COMMON_API_EXPORT void parseUtilsKey(uint64_t key, uint8_t& type, uint8_t& group);

}

#endif // COMMON_GLOBAL_H
