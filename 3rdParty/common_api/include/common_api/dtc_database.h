/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2021-2041 leiwei. All rights reserved.
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
* Date: 2021-03-01
*----------------------------------------------------------------------------*/

/**
 * @file dtc_database.h
 * @brief Diagnostic Trouble Code (DTC) lookup database shared by OBD-II and UDS.
 *
 * Provides a singleton DTC lookup database loaded from JSON configuration files.
 * DTC encoding follows SAE J2012 / ISO 15031-6 / ISO 14229-1 Annex D.
 *
 * The 2-byte DTC wire encoding is identical for both OBD-II (Mode 03/07/0A)
 * and UDS (Service 0x19). This database serves both protocols with a single
 * unified code/description mapping.
 *
 * Encoding format (SAE J2012):
 *   Byte 1: [Type(2b)][D1(2b)][D2(4b)]
 *   Byte 2: [D3(4b)][D4(4b)]
 *
 *   Type bits: 00=P (Powertrain), 01=C (Chassis), 10=B (Body), 11=U (Network)
 *   D1 bits:   00=0 (SAE standard), 01=1 (Manufacturer), 10=2 (SAE), 11=3 (SAE/Mfr)
 *
 *   Example: P0123 → Byte1=0x01, Byte2=0x23
 *            U0100 → Byte1=0xC1, Byte2=0x00
 *            B0001 → Byte1=0x80, Byte2=0x01
 *
 * File format (JSON):
 *   {
 *     "dtcs": [
 *       { "code": "P0100", "en": "Mass Air Flow Circuit", "zh": "空气流量传感器电路" },
 *       ...
 *     ]
 *   }
 *
 * Typical coverage:
 *   P0xxx  ~800 codes  (Powertrain standard)
 *   P2xxx  ~600 codes  (Powertrain extended)
 *   P3xxx  ~400 codes  (Powertrain extended)
 *   B0/2xxx ~700 codes (Body)
 *   C0/2xxx ~300 codes (Chassis)
 *   U0/2/3xxx ~800 codes (Network)
 *   Total: ~3600+ SAE standard codes
 *
 * Usage example:
 *   // Load at application startup
 *   Common::DtcDatabase::instance().loadFromDirectory("config/dtc");
 *   Common::DtcDatabase::instance().setLanguage("zh");
 *
 *   // Lookup by code string
 *   std::string desc = Common::DtcDatabase::instance().description("P0123");
 *   // → "节气门/踏板位置传感器A电路输入过高"
 *
 *   // Lookup by wire bytes (from OBD-II Mode 03 or UDS 0x19 response)
 *   std::string desc2 = Common::DtcDatabase::instance().description(0x01, 0x23);
 *   // → same result
 *
 *   // Decode wire bytes to code string
 *   std::string code = Common::DtcDatabase::decode(0xC1, 0x00);
 *   // → "U0100"
 */

#ifndef COMMON_DTC_DATABASE_H
#define COMMON_DTC_DATABASE_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "common_global.h"

namespace Common {

// ===================================================================
// DTC Definition — one entry in the database
// ===================================================================

/**
 * @struct DtcDefinition
 * @brief Holds the code string and bilingual description for a single DTC.
 */
struct DtcDefinition {
    std::string code;           ///< Standard code string, e.g. "P0123", "U0100"
    std::string name;           ///< English description from SAE J2012
    std::string name_zh;        ///< Chinese description (translated)
};

// ===================================================================
// DTC System — the top-level letter classification
// ===================================================================

/**
 * @enum DtcSystem
 * @brief The four DTC system categories defined by SAE J2012.
 */
enum class DtcSystem {
    Powertrain,     ///< P — Engine, Transmission, Hybrid/EV drivetrain
    Body,           ///< B — Lighting, HVAC, Airbag, Doors, Seats, etc.
    Chassis,        ///< C — ABS, Steering, Suspension, Brakes
    Network,        ///< U — CAN/LIN/FlexRay/Ethernet communication
    Unknown
};

// ===================================================================
// DTC Category — sub-system classification based on code digits
// ===================================================================

/**
 * @enum DtcCategory
 * @brief Sub-system category derived from the DTC code digits.
 *
 * For Powertrain P0xxx codes, the third character determines the sub-system:
 *   P00xx = Fuel/Air Aux, P01xx = Fuel/Air, P02xx = Injector, P03xx = Ignition,
 *   P04xx = Aux Emission, P05xx = Speed/Idle, P06xx = ECU, P07xx = Transmission,
 *   P08xx = Transmission Ext, P09xx = Hybrid/EV.
 *
 * Codes with '1' as the second character (P1xxx, B1xxx, C1xxx, U1xxx) are
 * manufacturer-specific and not defined in the SAE standard.
 */
enum class DtcCategory {
    // Powertrain P0xxx sub-groups
    FuelAirAux,             ///< P00xx — Fuel and Air Metering (Auxiliary Emission Controls)
    FuelAirMetering,        ///< P01xx — Fuel and Air Metering
    FuelAirInjector,        ///< P02xx — Fuel and Air Metering (Injector Circuit)
    IgnitionMisfire,        ///< P03xx — Ignition System / Misfire
    AuxEmission,            ///< P04xx — Auxiliary Emission Controls
    SpeedIdleAux,           ///< P05xx — Vehicle Speed / Idle Control / Auxiliary Inputs
    EcuOutput,              ///< P06xx — Computer and Output Circuit
    Transmission,           ///< P07xx — Transmission
    TransmissionExt,        ///< P08xx — Transmission (Extended)
    HybridEV,               ///< P09xx — Hybrid/EV Propulsion

    // Powertrain extended
    PowertrainExtended,     ///< P2xxx, P3xxx — SAE extended Powertrain codes

    // Body, Chassis, Network
    BodyGeneral,            ///< B0xxx, B2xxx — Body system codes
    ChassisGeneral,         ///< C0xxx, C2xxx — Chassis system codes
    NetworkComm,            ///< U0xxx, U2xxx, U3xxx — Network communication codes

    // Manufacturer-specific
    ManufacturerSpecific,   ///< x1xxx — Manufacturer-specific (not in SAE J2012)

    Unknown
};

// ===================================================================
// DTC Database — Singleton
// ===================================================================

/**
 * @class DtcDatabase
 * @brief Singleton DTC lookup database with bilingual descriptions.
 *
 * This class loads DTC definitions from JSON files and provides O(1) lookup
 * by code string or by 2-byte wire encoding. It also provides static utility
 * functions for DTC encoding/decoding that work without loading any data files.
 *
 * Thread safety:
 *   - loadFromDirectory() / loadFromFile() should be called once at startup
 *     before any concurrent access.
 *   - All const lookup methods (find, description) are thread-safe for
 *     concurrent reads after loading is complete.
 *   - setLanguage() is not synchronized; call it before concurrent reads.
 *   - All static methods (decode, encode, systemFromCode, etc.) are
 *     inherently thread-safe.
 */
class COMMON_API_EXPORT DtcDatabase {
public:
    /**
     * @brief Returns the singleton instance.
     * @return Reference to the global DtcDatabase instance.
     */
    static DtcDatabase& instance();

    // -----------------------------------------------------------------
    // Loading
    // -----------------------------------------------------------------

    /**
     * @brief Loads all DTC JSON files from a directory.
     *
     * Scans the directory for files matching the pattern "dtc_*.json" and
     * merges all DTC definitions into the internal map. Files are processed
     * in alphabetical order. Duplicate codes are overwritten silently.
     *
     * @param dirPath Path to the directory containing DTC JSON files.
     * @return true if at least one file was loaded successfully.
     */
    bool loadFromDirectory(const std::string& dirPath);

    /**
     * @brief Loads DTC definitions from a single JSON file.
     *
     * The JSON file must contain a top-level object with a "dtcs" array.
     * Each element must have "code" (string), "en" (string), and
     * optionally "zh" (string) fields.
     *
     * @param filePath Path to the JSON file.
     * @return true on success.
     */
    bool loadFromFile(const std::string& filePath);

    // -----------------------------------------------------------------
    // Lookup
    // -----------------------------------------------------------------

    /**
     * @brief Finds a DTC definition by its standard code string.
     * @param code DTC code, e.g. "P0123", "U0100". Case-sensitive.
     * @return Pointer to the definition, or nullptr if not found.
     */
    const DtcDefinition* find(const std::string& code) const;

    /**
     * @brief Finds a DTC definition by its 2-byte wire encoding.
     *
     * This is a convenience overload that first decodes the bytes to a
     * code string, then performs the lookup.
     *
     * @param byte1 High byte of the DTC (e.g. 0x01 for P0123).
     * @param byte2 Low byte of the DTC (e.g. 0x23 for P0123).
     * @return Pointer to the definition, or nullptr if not found.
     */
    const DtcDefinition* find(uint8_t byte1, uint8_t byte2) const;

    /**
     * @brief Returns the localized description for a DTC code.
     *
     * Uses the language set by setLanguage(). Falls back to English if
     * the Chinese description is empty or language is not "zh".
     *
     * @param code DTC code string, e.g. "P0123".
     * @return Localized description, or empty string if not found.
     */
    std::string description(const std::string& code) const;

    /**
     * @brief Returns the localized description for a DTC by wire bytes.
     * @param byte1 High byte.
     * @param byte2 Low byte.
     * @return Localized description, or empty string if not found.
     */
    std::string description(uint8_t byte1, uint8_t byte2) const;

    // -----------------------------------------------------------------
    // Language
    // -----------------------------------------------------------------

    /**
     * @brief Sets the language for localized lookups.
     * @param lang Language code: "en" for English (default), "zh" for Chinese.
     */
    void setLanguage(const std::string& lang) { m_language = lang; }

    /**
     * @brief Gets the current language setting.
     * @return Current language code.
     */
    std::string language() const { return m_language; }

    // -----------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------

    /**
     * @brief Checks if the database has been loaded.
     * @return true if at least one file has been loaded successfully.
     */
    bool isLoaded() const { return m_loaded; }

    /**
     * @brief Returns the total number of DTC definitions in the database.
     * @return Number of loaded DTC entries.
     */
    size_t size() const { return m_dtcs.size(); }

    // =================================================================
    // Static Utilities — DTC Encoding / Decoding (SAE J2012)
    //
    // These functions operate purely on the DTC code format and do NOT
    // require the database to be loaded. They can be used standalone.
    // =================================================================

    /**
     * @brief Decodes 2 wire bytes into a DTC code string.
     *
     * Implements the SAE J2012 / ISO 15031-6 encoding scheme:
     *   Bits [15:14] → Type letter (P/C/B/U)
     *   Bits [13:12] → First digit (0-3)
     *   Bits [11:8]  → Second digit (0-F, typically 0-9)
     *   Bits [7:4]   → Third digit (0-F)
     *   Bits [3:0]   → Fourth digit (0-F)
     *
     * @param byte1 High byte.
     * @param byte2 Low byte.
     * @return DTC code string, e.g. "P0123", "U0100".
     */
    static std::string decode(uint8_t byte1, uint8_t byte2);

    /**
     * @brief Encodes a DTC code string into 2 wire bytes.
     *
     * @param code DTC code string (must be exactly 5 characters: letter + 4 hex digits).
     * @param[out] byte1 High byte output.
     * @param[out] byte2 Low byte output.
     * @return true on success, false if the code format is invalid.
     */
    static bool encode(const std::string& code, uint8_t& byte1, uint8_t& byte2);

    /**
     * @brief Determines the system type from a DTC code string.
     * @param code DTC code string, e.g. "P0123".
     * @return The DtcSystem enum value.
     */
    static DtcSystem systemFromCode(const std::string& code);

    /**
     * @brief Determines the sub-system category from a DTC code string.
     * @param code DTC code string, e.g. "P0123".
     * @return The DtcCategory enum value.
     */
    static DtcCategory categoryFromCode(const std::string& code);

    /**
     * @brief Returns a localized display name for a DTC category.
     * @param category The category enum value.
     * @param lang Language code: "en" or "zh".
     * @return Category display name string.
     */
    static std::string categoryName(DtcCategory category, const std::string& lang = "en");

    /**
     * @brief Returns a localized display name for a DTC system.
     * @param system The system enum value.
     * @param lang Language code: "en" or "zh".
     * @return System display name string.
     */
    static std::string systemName(DtcSystem system, const std::string& lang = "en");

private:
    DtcDatabase() = default;
    ~DtcDatabase() = default;

    DtcDatabase(const DtcDatabase&) = delete;
    DtcDatabase& operator=(const DtcDatabase&) = delete;

    /**
     * @brief Returns the localized name from a DtcDefinition based on current language.
     * @param def The DTC definition.
     * @return Localized description string.
     */
    std::string localizedName(const DtcDefinition& def) const;

    /// DTC definitions indexed by code string (e.g. "P0123")
    std::unordered_map<std::string, DtcDefinition> m_dtcs;

    /// Current language for localized lookups ("en" or "zh")
    std::string m_language = "en";

    /// Whether at least one file has been loaded
    bool m_loaded = false;
};

} // namespace Common

#endif // COMMON_DTC_DATABASE_H
