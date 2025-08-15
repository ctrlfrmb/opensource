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
* Version: v2.0.0
* Date: 2022-10-15
*----------------------------------------------------------------------------*/

/**
* @file period_sender.h
* @brief High-performance periodic data sender with thread-safe frame management
*
* The PeriodSender class provides a high-performance periodic data transmission system
* that manages multiple data frames with independent timing configurations. It uses
* modern C++ concurrency features to achieve efficient thread-safe operations.
*
* Features:
* - Thread-safe frame management with std::shared_mutex for optimal read/write performance
* - Individual timing control for each frame (period and initial delay)
* - Atomic operations for execution time tracking to minimize lock contention
* - Configurable send buffer with overflow protection
* - Key-based frame identification (type + group + messageId composition)
* - Batch frame operations for efficient bulk management
* - Automatic timer lifecycle management (start/stop based on frame presence)
* - High-precision timing based on CallbackTimer infrastructure
* - Memory-efficient frame container with reserve/clear operations
* - Protocol and channel-based selective clearing capabilities
*
* Usage example:
*   Common::PeriodSender sender;
*
*   // Set send callback
*   sender.setSendCallback([](const std::vector<char>& data) {
*       // Send data via network/serial/etc
*       return 0;
*   });
*
*   // Create and add frame
*   Common::SendFrame frame;
*   frame.key = Common::PeriodSender::makeKey(0x01, 0x02, 0x1234);
*   frame.data = {0x01, 0x02, 0x03, 0x04};
*   frame.period = 100; // Send every 100ms
*
*   sender.addFrame(std::move(frame));
*
*   // Timer automatically starts and manages sending
*/

#ifndef COMMON_PERIOD_SENDER_H
#define COMMON_PERIOD_SENDER_H

#include <atomic>
#include <thread>
#include <functional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "common_global.h"

namespace Common {

class CallbackTimer;

class COMMON_API_EXPORT PeriodSender {

public:
    static constexpr size_t DEFAULT_MAX_FRAMES = 0xFF;          // 默认最大255条消息
    static constexpr uint32_t DEFAULT_PERIOD = 100;             // 默认周期100ms
    static constexpr size_t DEFAULT_SEND_BUFFER_SIZE = 1024;    // 默认1KB缓冲区
    static constexpr size_t MIN_SEND_BUFFER_SIZE = 256;         // 最小缓冲区256字节
    static constexpr size_t MAX_SEND_BUFFER_SIZE = 64 * 1024;   // 最大缓冲区64KB
    static constexpr size_t MIN_MAX_FRAMES = 1;                 // 最小帧数1条
    static constexpr size_t MAX_MAX_FRAMES = 1024;              // 最大帧数1024条

    explicit PeriodSender();
    ~PeriodSender();

    void setSendCallback(std::function<int(const std::vector<char>&)> callback);

    /**
     * @brief 设置发送缓冲区大小
     * @param size 缓冲区大小（字节），范围：[MIN_SEND_BUFFER_SIZE, MAX_SEND_BUFFER_SIZE]
     * @return true=成功, false=失败（运行中或参数无效）
     */
    bool setSendBufferSize(size_t size);

    /**
     * @brief 获取当前发送缓冲区大小
     * @return 缓冲区大小（字节）
     */
    size_t getSendBufferSize() const { return sendBufferSize_; }

    /**
     * @brief 设置最大帧数量
     * @param maxFrames 最大帧数量，范围：[MIN_MAX_FRAMES, MAX_MAX_FRAMES]
     * @return true=成功, false=失败（运行中或参数无效）
     */
    bool setMaxFrames(size_t maxFrames);

    /**
     * @brief 获取最大帧数量
     * @return 最大帧数量
     */
    size_t getMaxFrames() const { return maxFrames_; }

    /**
     * @brief 获取当前帧数量
     * @return 当前帧数量
     */
    size_t getCurrentFrameCount() const { return frameContainer_.size(); }

    /**
     * @brief 获取缓冲区使用情况
     * @return 缓冲区使用率（0.0-1.0）
     */
    double getBufferUsageRatio() const {
        return static_cast<double>(sendBuffer_.size()) / sendBufferSize_;
    }

    void resetCounter() { frameContainer_.counter.store(0, std::memory_order_release); }
    uint64_t getCounter() const { return frameContainer_.counter.load(std::memory_order_acquire); }

    // --- 添加帧 (右值引用版本，用于临时对象) ---
    int addFrame(SendFrame&& frame);
    int addFrames(SendQueue&& frames);
    int updateData(uint64_t key, std::vector<char>&& data);

    // --- 添加帧 (左值引用版本，用于已存在对象 ---
    int addFrame(const SendFrame& frame);
    int addFrames(const SendQueue& frames);
    int updateData(uint64_t key, const std::vector<char>& data);

    // --- 删除操作 ---
    bool removeFrame(uint64_t key);
    int clear();
    int clear(uint16_t type);
    int clear(uint16_t type, uint16_t group);

private:
    // 帧管理容器
    struct FrameContainer {
        mutable std::shared_mutex mutex;
        std::atomic_uint64_t counter{0};
        std::unordered_map<uint64_t, SendFrame> frames;                     // 原始帧数据
        std::unordered_map<uint64_t, std::atomic_uint64_t> nextExecTimes;   // 下一次执行时间点 - 使用原子操作

        void clear() {
            std::unique_lock<std::shared_mutex> lock(mutex);  // 写锁
            frames.clear();
            nextExecTimes.clear();
        }

        size_t size() const {
            std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
            return frames.size();
        }

        bool empty() const {
            std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
            return frames.empty();
        }
    };

    // 检查是否可以修改配置（未运行状态）
    bool canModifyConfig() const;

    // 线程函数
    int senderLoop();  // 统一的发送线程

    // 帧处理
    bool addToContainer(SendFrame&& frame);
    bool addToContainer(const SendFrame& frame);
    int updateFrameInContainer(uint64_t key, const std::vector<char>& data);
    bool removeFromContainer(uint64_t key);
    int clearContainer(uint16_t type, int group = -1);

private:
    std::unique_ptr<CallbackTimer> timer_;
    std::function<int(const std::vector<char>&)> sendCallback_;
    FrameContainer frameContainer_;         // 统一的帧容器
    std::vector<char> sendBuffer_;

    // 配置参数
    size_t sendBufferSize_;                 // 发送缓冲区大小
    size_t maxFrames_;                      // 最大帧数量
};

}

#endif // COMMON_PERIOD_SENDER_H
