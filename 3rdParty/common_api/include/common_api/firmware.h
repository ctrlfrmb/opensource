/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2022-2042 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v1.1.0
* Date: 2025-01-01
*----------------------------------------------------------------------------*/

#ifndef COMMON_FIRMWARE_H
#define COMMON_FIRMWARE_H

#include "common_types.h"
#include "common_global.h"

namespace Common {

/**
 * @brief Enumeration of supported firmware file formats.
 */
enum class FirmwareType {
    Unknown,
    Bin,        // Raw binary file (.bin, .img)
    IntelHex,   // Intel Hex format (.hex)
    MotorolaS19 // Motorola S-Record (.s19, .mot, .srec)
};

/**
 * @class Firmware
 * @brief A generic class for loading and parsing ECU firmware files.
 *
 * This class handles the reading of firmware files from disk into memory (RAM).
 * It automatically detects file formats (Hex/S19/Bin) and parses them into
 * a contiguous memory buffer. Gaps in address ranges (common in Hex/S19)
 * are automatically padded with 0xFF.
 */
class COMMON_API_EXPORT Firmware {
public:
    Firmware();
    ~Firmware();

    // Movable but not copyable to prevent accidental large memory copies
    Firmware(Firmware&& other) noexcept;
    Firmware& operator=(Firmware&& other) noexcept;
    Firmware(const Firmware&) = delete;
    Firmware& operator=(const Firmware&) = delete;

    /**
     * @brief Loads a firmware file from the specified path.
     *
     * @param file_path The absolute or relative path to the firmware file.
     * @param manual_start_addr Optional start address. Only used for .bin files
     *                          since they lack internal address information.
     * @return true if the file was successfully loaded and parsed, false otherwise.
     */
    bool load(const std::string& file_path, uint32_t manual_start_addr = 0);

    /**
     * @brief Clears the loaded data and resets the state.
     */
    void clear();

    // --- Accessors ---

    /** @brief Checks if a valid firmware image is loaded. */
    bool isValid() const { return valid_; }

    /** @brief Returns the file type detected during load. */
    FirmwareType getType() const { return type_; }

    /** @brief Returns the file path. */
    const std::string& getFilePath() const { return file_path_; }

    /** @brief Returns the logical start address of the firmware. */
    uint32_t getStartAddress() const { return start_address_; }

    /** @brief Returns the logical end address of the firmware. */
    uint32_t getEndAddress() const { return start_address_ + static_cast<uint32_t>(data_.size()) - 1; }

    /** @brief Returns the total size of the firmware data in bytes. */
    size_t getSize() const { return data_.size(); }

    /**
     * @brief Returns the default CRC32 checksum calculated during load.
     * Uses Standard CRC32 (Poly 0x04C11DB7).
     */
    uint32_t getChecksum() const { return checksum_; }

    /**
     * @brief Calculates a checksum using a specific configuration.
     * Useful if the ECU expects a different algorithm (e.g., CRC16, CRC8) than the default.
     *
     * @param config The CRC algorithm configuration.
     * @return The calculated checksum result.
     */
    uint32_t calculateChecksum(const CrcConfig& config) const;

    /** @brief Returns a constant reference to the data buffer. */
    const std::vector<uint8_t>& getData() const { return data_; }

    /** @brief Returns a raw pointer to the data buffer (useful for C APIs). */
    const uint8_t* getRawData() const { return data_.data(); }

private:
    // Internal parsing logic
    bool parseBin(const std::string& path, uint32_t start_addr);
    bool parseHex(const std::string& path);
    bool parseS19(const std::string& path);

    // Member variables (using trailing underscore style)
    bool valid_;
    std::string file_path_;
    FirmwareType type_;
    uint32_t start_address_;
    uint32_t checksum_;
    std::vector<uint8_t> data_;
};

} // namespace Common

#endif // COMMON_FIRMWARE_H
