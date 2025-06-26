/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2023 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei
* Version: v2.2.0
* Date: 2023-07-16
*----------------------------------------------------------------------------*/

/**
* @file config_manager.h
* @brief Configuration management for common settings across the application
*
* The ConfigManager class implements a Singleton pattern to provide centralized
* management of application configuration settings. It handles loading and saving
* configuration data from/to INI files, with support for logging parameters,
* file paths, and other application settings.
*
* Features:
* - Singleton design pattern ensures a single configuration instance
* - Automatic loading of configuration values from INI file
* - Getter/setter methods for all configuration parameters
* - Support for custom key-value pairs
* - Persistent storage of configuration changes
*
* Usage example:
*   auto& config = Common::ConfigManager::getInstance();
*   if (config.getLogToConsole()) {
*       // Output logs to console
*   }
*/

#ifndef COMMON_CONFIG_MANAGER_H
#define COMMON_CONFIG_MANAGER_H

#include <QString>

#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT ConfigManager
{
public:
    static ConfigManager& getInstance();

    // 禁用拷贝和移动
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool getRecordState() const { return recordState_; }
    void setRecordState(bool value) { recordState_ = value; }

    bool getLogToConsole() const { return logToConsole_; }
    void setLogToConsole(bool value) { logToConsole_ = value; }

    uint8_t getLogLevel() const { return logLevel_; }
    void setLogLevel(uint8_t value) { logLevel_ = value; }

    uint8_t getLogSize() const { return logSize_; }
    void setLogSize(uint8_t value) { logSize_ = value; }

    uint8_t getLogFiles() const { return logFiles_; }
    void setLogFiles(uint8_t value) { logFiles_ = value; }

    QString getLogFilePath() const { return logFilePath_; }
    void setLogFilePath(const QString& value) { logFilePath_ = value; }

    QString getCacheFilePath() const { return cacheFilePath_; }
    void setCacheFilePath(const QString& value) { cacheFilePath_ = value; }

    QString getValue(const QString& key) const;
    void setValue(const QString& key, const QString& value);

    void loadConfig();
    void saveConfig();

private:
    ConfigManager();
    ~ConfigManager() = default;

    const QString COMMON_CONFIG_FILE{"config/common_config.ini"};
    const QString CACHE_FILE_PATH = "cache/common_cache.dat";
    const QString LOG_FILE_PATH = "logs/test.log";

    // 配置项成员变量
    bool recordState_;
    bool logToConsole_;
    uint8_t logLevel_;
    uint8_t logSize_;
    uint8_t logFiles_;
    QString logFilePath_;
    QString cacheFilePath_;
};

}

#endif // COMMON_CONFIG_MANAGER_H
