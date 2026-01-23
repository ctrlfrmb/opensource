/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2025 FigKey. All rights reserved.
*
* Author: leiwei
* Date: 2025-01-16
*----------------------------------------------------------------------------*/

#ifndef COMMON_SIGNAL_GENERATOR_H
#define COMMON_SIGNAL_GENERATOR_H

#include "common_global.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace Common {

/**
 * @enum SignalType
 * @brief Defines the supported signal generation algorithms.
 */
enum class SignalType : uint8_t {
    None = 0,
    Sine,           ///< 正弦: A*sin(w*n + p) + k
    Triangle,       ///< 三角: Periodic triangle wave
    Square,         ///< 矩形: Periodic square wave
    Arithmetic,     ///< 等差: Linear ramp (sawtooth)
    Geometric,      ///< 等比: Exponential growth
    Random,         ///< 随机: Random noise
    CustomSequence  ///< 自定义: User defined list
};

/**
 * @class SignalGenerator
 * @brief Abstract base class for all signal generators.
 */
class COMMON_API_EXPORT SignalGenerator {
public:
    virtual ~SignalGenerator() = default;

    /**
     * @brief Calculate the signal value at step n.
     * @param n The time step index (0, 1, 2...).
     * @return The calculated signal value.
     */
    virtual double calculate(uint64_t n) = 0;

    /**
     * @brief Get the type of this generator.
     */
    virtual SignalType type() const = 0;

    /**
     * @brief Factory method to create a generator instance by type.
     */
    static std::unique_ptr<SignalGenerator> create(SignalType type);
};

// =============================================================================
// Concrete Implementation Classes
// =============================================================================

/**
 * @brief Sine Wave Generator
 * Formula: A * sin(w * n + p) + k
 */
class COMMON_API_EXPORT SineGenerator : public SignalGenerator {
public:
    double amplitude = 10.0;       ///< A: 振幅
    double angular_velocity = 1.0; ///< w: 角速度 (rad/step)
    double phase = 0.0;            ///< p: 初相 (rad)
    double offset = 0.0;           ///< k: 偏置
    double min_val = 0.0;          ///< 下限 (Optional clamp)
    double max_val = 0.0;          ///< 上限 (Optional clamp)

    double calculate(uint64_t n) override;
    SignalType type() const override { return SignalType::Sine; }
};

/**
 * @brief Triangle Wave Generator
 * Formula based on ZLG:
 *  0 <= t < T/2 : Rising
 *  T/2 <= t < T : Falling
 */
class COMMON_API_EXPORT TriangleGenerator : public SignalGenerator {
public:
    double period = 10.0;          ///< T: 周期次数
    double amplitude = 10.0;       ///< A: 幅值
    double phase_shift = 0.0;      ///< a: 横向偏移
    double vertical_shift = 0.0;   ///< b: 纵向偏移
    double min_val = 0.0;
    double max_val = 0.0;

    double calculate(uint64_t n) override;
    SignalType type() const override { return SignalType::Triangle; }
};

/**
 * @brief Square Wave Generator
 * Formula: (n % T) < (r * T) ? a : b
 */
class COMMON_API_EXPORT SquareGenerator : public SignalGenerator {
public:
    double period = 10.0;          ///< T: 周期次数
    double duty_cycle = 0.5;       ///< r: 占空比 [0.0 - 1.0]
    double high_value = 5.0;       ///< a: 高值
    double low_value = 0.0;        ///< b: 低值

    double calculate(uint64_t n) override;
    SignalType type() const override { return SignalType::Square; }
};

/**
 * @brief Arithmetic Sequence Generator (Linear Ramp / Sawtooth)
 * Formula: Loops from min to max with step d
 */
class COMMON_API_EXPORT ArithmeticGenerator : public SignalGenerator {
public:
    double step_value = 1.0;       ///< d: 差值
    double min_value = 0.0;        ///< 下限值
    double max_value = 10.0;       ///< 上限值

    double calculate(uint64_t n) override;
    SignalType type() const override { return SignalType::Arithmetic; }
};

/**
 * @brief Geometric Sequence Generator
 * Formula: a * q^n (Loops within range)
 */
class COMMON_API_EXPORT GeometricGenerator : public SignalGenerator {
public:
    double initial_value = 1.0;    ///< a: 初始值
    double ratio = 2.0;            ///< q: 比值
    double min_value = 0.0;        ///< 下限值
    double max_value = 100.0;      ///< 上限值

    double calculate(uint64_t n) override;
    SignalType type() const override { return SignalType::Geometric; }
};

/**
 * @brief Random Value Generator
 * Formula: Random value between min and max
 */
class COMMON_API_EXPORT RandomGenerator : public SignalGenerator {
public:
    double min_value = 0.0;        ///< 下限值
    double max_value = 10.0;       ///< 上限值

    double calculate(uint64_t n) override;
    SignalType type() const override { return SignalType::Random; }
};

/**
 * @brief Custom Sequence Generator
 * Formula: Loops through a user-defined list
 */
class COMMON_API_EXPORT CustomSequenceGenerator : public SignalGenerator {
public:
    std::vector<double> sequence;  ///< 值序列

    double calculate(uint64_t n) override;
    SignalType type() const override { return SignalType::CustomSequence; }
};

} // namespace Common

#endif // COMMON_SIGNAL_GENERATOR_H
