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
* Version: v2.3.0
* Date: 2022-07-16
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

    // Prevent copying and moving
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool getRecordState() const { return record_state_; }
    void setRecordState(bool value) { record_state_ = value; }

    bool getLogToConsole() const { return log_to_console_; }
    void setLogToConsole(bool value) { log_to_console_ = value; }

    uint8_t getLogLevel() const { return log_level_; }
    void setLogLevel(uint8_t value) { log_level_ = value; }

    uint8_t getLogSize() const { return log_size_; }
    void setLogSize(uint8_t value) { log_size_ = value; }

    uint8_t getLogFiles() const { return log_files_; }
    void setLogFiles(uint8_t value) { log_files_ = value; }

    QString getLogFilePath() const { return log_file_path_; }
    void setLogFilePath(const QString& value) { log_file_path_ = value; }

    QString getCacheFilePath() const { return cache_file_path_; }
    void setCacheFilePath(const QString& value) { cache_file_path_ = value; }

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

    // Configuration member variables
    bool record_state_;
    bool log_to_console_;
    uint8_t log_level_;
    uint8_t log_size_;
    uint8_t log_files_;
    QString log_file_path_;
    QString cache_file_path_;
};

}

#endif // COMMON_CONFIG_MANAGER_H
