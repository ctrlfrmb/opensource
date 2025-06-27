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
* Version: v1.0.8
* Date: 2022-11-05
*----------------------------------------------------------------------------*/

/**
* @file unifiedtime_manager.h
* @brief Multi-device timestamp synchronization and formatting system
*
* The UnifiedTimeManager provides a comprehensive solution for managing and
* synchronizing timestamps from different devices with varying time bases. It
* handles conversion between device-specific time domains and provides multiple
* formatting options for timestamp display in user interfaces.
*
* Features:
* - Management of timestamps from multiple devices with different time bases
* - Conversion between device-specific, relative, and absolute time domains
* - Multiple timestamp display formats (absolute, relative, delta, time of day)
* - Thread-safe implementation with read-write locking
* - Microsecond precision with configurable time scale factor
* - Tracking of time offsets between devices
* - Support for device registration and base time updates
*
* Usage example:
*   auto timeManager = new Common::UnifiedTimeManager();
*   timeManager->initialize(QDateTime::currentDateTime(), 1000000.0);
*
*   // Register a device with its starting timestamp
*   timeManager->registerDevice(1, deviceStartTimestamp);
*
*   // Format timestamps for display
*   QString timestamp = timeManager->formatTimestamp(
*       1, currentDeviceTimestamp, Common::TIMESTAMP_MODE_RELATIVE);
*/

#ifndef COMMON_UNIFIED_TIME_MANAGER_H
#define COMMON_UNIFIED_TIME_MANAGER_H

#include <QDateTime>
#include <QMap>
#include <QReadWriteLock>
#include <QString>

#include "common_global.h"

namespace Common {

/**
 * @brief Timestamp display mode enumeration
 *
 * Defines various formats for displaying timestamps in the UI
 */
typedef enum {
    TIMESTAMP_MODE_ABSOLUTE = 0,     ///< Absolute time (2025-05-07 14:30:22.123456)
    TIMESTAMP_MODE_RELATIVE = 1,     ///< Relative time (3.123456s) - Time since device started
    TIMESTAMP_MODE_RELATIVE_GLOBAL = 2, ///< Global relative time - Relative to global reference time
    TIMESTAMP_MODE_DELTA = 3,        ///< Delta time (Δ 0.001234s) - Difference from previous message
    TIMESTAMP_MODE_TIME_OF_DAY = 4   ///< Time of day (14:30:22.123456)
} TimestampDisplayMode;

/**
 * @brief Unified Time Manager class
 *
 * Manages and synchronizes timestamps from multiple devices with different
 * time bases. Provides various formatting options for display purposes.
 */
class COMMON_API_EXPORT UnifiedTimeManager {
public:
    /**
     * @brief Default constructor
     */
    UnifiedTimeManager();

    /**
     * @brief Destructor
     */
    ~UnifiedTimeManager();

    /**
     * @brief Initialize the time manager
     *
     * @param globalReferenceTime The global reference time
     * @param timeScaleFactor Time scale factor (1.0=seconds, 1000.0=milliseconds, 1000000.0=microseconds)
     */
    void initialize(const QDateTime& globalReferenceTime = QDateTime::currentDateTime(),
                   double timeScaleFactor = 1000000.0);

    /**
     * @brief Reset the time manager state
     *
     * Clears all registered devices and timestamp history
     */
    void reset();

    /**
     * @brief Register a device with its base timestamp
     *
     * @param deviceId Unique identifier for the device
     * @param deviceBaseTimeMicros Base timestamp for the device
     */
    void registerDevice(int deviceId, uint64_t deviceBaseTimeMicros);

    /**
     * @brief Unregister a device
     *
     * @param deviceId Device identifier to remove
     */
    void unregisterDevice(int deviceId);

    /**
     * @brief Update a device's base timestamp
     *
     * @param deviceId Device identifier
     * @param newBaseTimeMicros New base timestamp value
     */
    void updateDeviceBaseTime(int deviceId, uint64_t newBaseTimeMicros);

    /**
     * @brief Get absolute system time from device timestamp
     *
     * @param deviceId Device identifier
     * @param deviceTimestamp Device-specific timestamp
     * @return QDateTime representing system time
     */
    QDateTime getAbsoluteTime(int deviceId, uint64_t deviceTimestamp);

    /**
     * @brief Get seconds elapsed since device base time
     *
     * @param deviceId Device identifier
     * @param deviceTimestamp Device-specific timestamp
     * @return Seconds elapsed since device base time
     */
    double getRelativeSeconds(int deviceId, uint64_t deviceTimestamp);

    /**
     * @brief Get seconds elapsed since global reference time
     *
     * @param deviceId Device identifier
     * @param deviceTimestamp Device-specific timestamp
     * @return Seconds elapsed since global reference time
     */
    double getGlobalRelativeSeconds(int deviceId, uint64_t deviceTimestamp);

    /**
     * @brief Get seconds elapsed since previous timestamp
     *
     * @param deviceId Device identifier
     * @param deviceTimestamp Device-specific timestamp
     * @return Seconds elapsed since previous timestamp
     */
    double getDeltaSeconds(int deviceId, uint64_t deviceTimestamp);

    /**
     * @brief Format timestamp according to specified display mode
     *
     * @param deviceId Device identifier
     * @param deviceTimestamp Device-specific timestamp
     * @param mode Timestamp display mode
     * @return Formatted timestamp string
     */
    QString formatTimestamp(int deviceId, uint64_t deviceTimestamp,
                           TimestampDisplayMode mode = TIMESTAMP_MODE_RELATIVE);

    /**
     * @brief Get device time offset from global reference
     *
     * @param deviceId Device identifier
     * @return Offset in seconds
     */
    double getDeviceOffsetSeconds(int deviceId);

    /**
     * @brief Set time precision scale factor
     *
     * @param factor Scale factor (1.0=seconds, 1000.0=milliseconds, 1000000.0=microseconds)
     */
    void setTimeScaleFactor(double factor);

    /**
     * @brief Get current time precision scale factor
     *
     * @return Current scale factor
     */
    double getTimeScaleFactor() const;

    /**
     * @brief Get list of all registered device IDs
     *
     * @return List of device IDs
     */
    QList<int> getRegisteredDevices() const;

    /**
     * @brief Check if a device is registered
     *
     * @param deviceId Device identifier to check
     * @return True if device is registered
     */
    bool isDeviceRegistered(int deviceId) const;

    /**
     * @brief Reset the delta time calculator
     *
     * Resets the internal state used for calculating time differences
     */
    void resetDeltaTimeCalculator();

private:
    /**
     * @brief Device time information structure
     */
    struct DeviceTimeInfo {
        uint64_t baseTimestamp;       ///< Device base timestamp
        QDateTime registrationTime;   ///< Device registration system time
    };

    QDateTime m_globalReferenceTime;      ///< Global reference time point
    double m_timeScaleFactor;             ///< Time precision factor
    mutable QReadWriteLock m_lock;        ///< Thread safety lock
    QMap<int, DeviceTimeInfo> m_devices;  ///< Device information map
    QMap<int, uint64_t> m_lastTimestamps; ///< Last timestamp for each device
};

} // namespace Common

#endif // COMMON_UNIFIED_TIME_MANAGER_H
