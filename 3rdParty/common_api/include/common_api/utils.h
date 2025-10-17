/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2022-2042 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v2.1.0
* Date: 2022-09-04
*----------------------------------------------------------------------------*/

/**
 * @file utils.h
 * @brief Comprehensive utility functions for common tasks and network operations
 *
 * The Utils class provides a wide range of utility functions organized into
 * several categories:
 *
 * 1. System & Process Management
 *    - Cross-platform crash/exception handling
 *    - Process priority management
 *    - Thread identification utilities
 *
 * 2. Time & Data Processing
 *    - High-performance timestamp generation
 *    - Checksum calculation and hex string formatting
 *    - File system operations with error handling
 *
 * 3. CAN Bus Signal Processing
 *    - Comprehensive bit-level operations for both big and little endian formats
 *    - Signal extraction and insertion for CAN frame processing
 *    - Binary data manipulations with endianness support
 *
 * 4. Socket Network Operations (Enhanced)
 *    - Socket options configuration (TCP_NODELAY, Keep-Alive, Linger, etc.)
 *    - Socket buffer management (send/receive buffer size control)
 *    - TCP connection operations with timeout support
 *    - UDP broadcast configuration
 *    - Cross-platform socket utilities with unified error codes
 *
 * Usage example:
 * @code
 *   // Extract a CAN signal
 *   uint64_t value = Common::Utils::getUnsignedSignalValueByLSB(
 *       canData, dataLength, startBit, signalSize);
 *
 *   // Configure TCP socket
 *   Common::Utils::setTcpNoDelay(fd, true);
 *   Common::Utils::setTcpKeepAlive(fd, true, 60, 5, 3);
 *
 *   // Connect socket with timeout
 *   int result = Common::Utils::connectSocketNonBlocking(fd, "192.168.1.100", 8080, 3000);
 * @endcode
 */

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <vector>
#include <map>
#include <string>
#include <unordered_set>
#include "common_global.h"

namespace Common {

class COMMON_API_EXPORT Utils {
public:
    //=============================================================================
    // System & Process Management
    //=============================================================================

    /**
     * @brief Crash/exception handler callback function type
     */
    typedef void (*CrashHandlerCallback)();

    /**
     * @brief Register crash handler callback (e.g., for device shutdown)
     * @param cb Crash handler callback function pointer
     */
    static void registerCrashHandler(CrashHandlerCallback cb);

    /**
     * @brief Enable global crash/exception capture (call at the beginning of main function)
     */
    static void setupCrashHandler();

    /**
     * @brief Get current thread ID as string
     * @return String representation of the thread ID
     */
    static std::string getThreadIdString();

    /**
     * @brief Set process to high priority
     */
    static void setProcessHighPriority();

    /**
     * @brief Sets the current thread to a high, but not real-time, priority.
     * @return 0 on success, platform-specific error code on failure.
     */
    static int setThreadHighPriority();

    /**
     * @brief Sets the current thread to the highest possible real-time priority.
     * @return 0 on success, platform-specific error code on failure.
     */
    static int setThreadRealTimePriority();

    /**
     * @brief Issues a CPU-specific instruction to pause, yielding execution to the other
     *        hyper-thread on the same core.
     */
    static void cpuPause();

    /**
     * @brief Create a generic key
     * @param type Type (16 bits)
     * @param group Group (16 bits)
     * @param messageId/row Message ID (32 bits)
     * @return 64-bit generic key
     */
    static uint64_t makeUtilsKey(uint16_t type, uint16_t group, uint32_t messageId);

    /**
     * @brief Parse a generic key
     * @param key 64-bit generic key
     * @param type Output type
     * @param group Output group
     * @param messageId/row Output message ID
     */
    static void parseUtilsKey(uint64_t key, uint16_t& type, uint16_t& group, uint32_t& messageId);

    /**
     * @brief Parse a generic key (simplified version)
     * @param key 64-bit generic key
     * @param type Output type
     * @param group Output group
     */
    static void parseUtilsKey(uint64_t key, uint16_t& type, uint16_t& group);

    //=============================================================================
    // Time & Data Processing
    //=============================================================================
    /**
     * @brief Get microseconds elapsed since program start
     * @return Microseconds count
     */
    static uint64_t getCurrentMicrosecondsFast();

    /**
     * @brief Get milliseconds elapsed since program start
     * @return Milliseconds count
     */
    static uint64_t getCurrentMillisecondsFast();

    /**
     * @brief High-performance time string retrieval
     * @return Pointer to time string
     */
    static const char* getCurrentTimeStringFast();

    /**
     * @brief Efficiently and thread-safely get a date-time string compliant with Vector ASC file header format
     * @return Pointer to string formatted as "Www Mmm dd HH:MM:SS.ms YYYY"
     */
    static const char* getASCHeaderDateString();

    /**
     * @brief Calculate data checksum
     * @param data Data vector
     * @return Checksum value
     */
    static uint8_t calculateChecksum(const std::vector<uint8_t>& data);

    /**
     * @brief Convert byte array to uppercase hexadecimal string
     * @param data Byte array pointer
     * @param len Array length
     * @return Hexadecimal string
     */
    static std::string bytesToHexStringUpper(const uint8_t* data, size_t len);
    /**
     * @brief Convert byte array to lowercase hexadecimal string
     * @param data Byte array pointer
     * @param len Array length
     * @return Hexadecimal string
     */
    static std::string bytesToHexStringLower(const uint8_t* data, size_t len);

    /**
     * @brief Ensure file path exists (create necessary directories)
     * @param filePath File path
     * @return true=success, false=failure
     */
    static bool ensureFilePath(const std::string& filePath);

    /**
     * @brief Check if file exists
     * @param filePath File path
     * @return true=exists, false=does not exist
     */
    static bool fileExists(const std::string& filePath);

    //=============================================================================
    // CAN Bus Signal Processing
    //=============================================================================

    /**
     * @brief Sign extension helper function
     * @param rawValue Raw value
     * @param signalSize Signal size (bits)
     * @return Sign-extended signed value
     */
    static inline int64_t signalRawValueToSigned(uint64_t rawValue, uint16_t signalSize) {
        if ((rawValue & (1ULL << (signalSize - 1))) && signalSize < 64) {
            // Sign extension - set all high bits to 1
            rawValue |= (~0ULL << signalSize);
        }
        return static_cast<int64_t>(rawValue);
    }

    /**
     * @brief Byte inversion helper function
     * @param x Input byte
     * @return Inverted byte
     */
    static constexpr uint8_t invert_u8(uint8_t x) { return static_cast<uint8_t>(~x); }

    /**
     * @brief Get actual start bit for a signal (typically used for DBC files)
     * @param startBit Start bit
     * @param signalSize Signal size
     * @param isBigEndian Whether big-endian format
     * @return Actual start bit
     */
    static uint16_t getSignalActualStartBit(uint16_t startBit, uint16_t signalSize, bool isBigEndian);

    /**
     * @brief Set signal value using LSB format (Intel format, little-endian)
     * @param data Data buffer
     * @param dataLen Data length
     * @param startBit Start bit
     * @param signalSize Signal size
     * @param value Value to set
     * @return 0=success, <0=failure
     */
    static int setSignalValueByLSB(uint8_t* data, uint8_t dataLen, uint16_t startBit,
                                  uint16_t signalSize, uint64_t value);

    /**
     * @brief Get unsigned signal value using LSB format (Intel format, little-endian)
     * @param data Data buffer
     * @param dataLen Data length
     * @param startBit Start bit
     * @param signalSize Signal size
     * @return Unsigned signal value
     */
    static uint64_t getUnsignedSignalValueByLSB(const uint8_t* data, uint8_t dataLen,
                                               uint16_t startBit, uint16_t signalSize);

    /**
     * @brief Get signed signal value using LSB format (Intel format, little-endian)
     * @param data Data buffer
     * @param dataLen Data length
     * @param startBit Start bit
     * @param signalSize Signal size
     * @return Signed signal value
     */
    static int64_t getSignedSignalValueByLSB(const uint8_t* data, uint8_t dataLen,
                                            uint16_t startBit, uint16_t signalSize);

    /**
     * @brief Set signal value using MSB format (Motorola format, big-endian)
     * @param data Data buffer
     * @param dataLen Data length
     * @param startBit Start bit
     * @param signalSize Signal size
     * @param value Value to set
     * @return 0=success, <0=failure
     */
    static int setSignalValueByMSB(uint8_t* data, uint8_t dataLen, uint16_t startBit,
                                  uint16_t signalSize, uint64_t value);

    /**
     * @brief Get unsigned signal value using MSB format (Motorola format, big-endian)
     * @param data Data buffer
     * @param dataLen Data length
     * @param startBit Start bit
     * @param signalSize Signal size
     * @return Unsigned signal value
     */
    static uint64_t getUnsignedSignalValueByMSB(const uint8_t* data, uint8_t dataLen,
                                               uint16_t startBit, uint16_t signalSize);

    /**
     * @brief Get signed signal value using MSB format (Motorola format, big-endian)
     * @param data Data buffer
     * @param dataLen Data length
     * @param startBit Start bit
     * @param signalSize Signal size
     * @return Signed signal value
     */
    static int64_t getSignedSignalValueByMSB(const uint8_t* data, uint8_t dataLen,
                                            uint16_t startBit, uint16_t signalSize);

    /**
     * @brief Set signal value based on bit list
     * @param data Data buffer
     * @param dataLen Data length
     * @param bitList Bit list
     * @param value Value to set
     * @return 0=success, <0=failure
     */
    static int setSignalValueByBitList(uint8_t* data, uint8_t dataLen,
                                      const std::vector<uint16_t>& bitList, uint64_t value);

    /**
     * @brief Get actual bit set for a signal
     * @param startBit Start bit
     * @param signalSize Signal size
     * @param isBigEndian Whether big-endian format
     * @return Set of bits
     */
    static std::unordered_set<uint16_t> getSignalActualSetBits(uint16_t startBit, uint16_t signalSize, bool isBigEndian);

    /**
     * @brief Get actual bit array for a signal
     * @param startBit Start bit
     * @param size Signal size
     * @param isBigEndian Whether big-endian format
     * @return Array of bits
     */
    static std::vector<uint16_t> getSignalActualArrayBits(uint16_t startBit, uint16_t size, bool isBigEndian);

    //=============================================================================
    // General Algorithm
    //=============================================================================

    /**
     * @brief Generate a mixed password based on time and day of week
     * @return std::string The generated mixed password
     */
    static std::string generateTimeDayMixPassword();

    /**
     * @brief Generate a date-based communication password (valid for 24 hours)
     * @return std::string A 9-character password (remains constant within the day)
     */
    static std::string generateDailyComPassword();

    /**
     * @brief Generate a company-specific license
     * @param companyName [in] Company or customer name used for key generation.
     * @param out_filename [out] Generated filename.
     * @param out_filecontent [out] Generated file content
     * @return bool Returns true if successfully generated, false if input is empty.
     */
   static bool generateLicenseKeyByCompany(const std::string& companyName,
                                  std::string& out_filename,
                                  std::string& out_filecontent);

    //=============================================================================
    // Socket Network Operations (Enhanced)
    //=============================================================================

    /**
     * @brief Set socket blocking/non-blocking mode
     * @param fd Socket file descriptor
     * @param blocking true=blocking mode, false=non-blocking mode
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure, see utils_socket_types.h
     */
    static int setSocketBlockingMode(int fd, bool blocking);

    /**
     * @brief Set TCP_NODELAY option (disable Nagle's algorithm)
     * @param fd Socket file descriptor
     * @param enable Whether to enable TCP_NODELAY
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setTcpNoDelay(int fd, bool enable);

    /**
     * @brief Set TCP Keep-Alive option
     * @param fd Socket file descriptor
     * @param enable Whether to enable keep-alive
     * @param idle Idle time (seconds), default 60 seconds
     * @param interval Probe interval (seconds), default 5 seconds
     * @param count Probe count, default 3 times
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setTcpKeepAlive(int fd, bool enable, int idle = 60, int interval = 5, int count = 3);

    /**
     * @brief Set SO_LINGER option (control socket close behavior)
     * @param fd Socket file descriptor
     * @param enable Whether to enable linger
     * @param seconds Linger timeout (seconds), 0=immediate close with RST
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setSocketLinger(int fd, bool enable, int seconds);

    /**
     * @brief Set SO_REUSEADDR option (address reuse)
     * @param fd Socket file descriptor
     * @param enable Whether to enable address reuse
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setReuseAddr(int fd, bool enable);

    /**
     * @brief Set SO_BROADCAST option (UDP broadcast)
     * @param fd Socket file descriptor
     * @param enable Whether to enable broadcasting
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setBroadcast(int fd, bool enable);

    /**
     * @brief Set receive timeout
     * @param fd Socket file descriptor
     * @param timeoutMs Timeout (milliseconds)
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setReceiveTimeout(int fd, int timeoutMs);

    /**
     * @brief Set send timeout
     * @param fd Socket file descriptor
     * @param timeoutMs Timeout (milliseconds)
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setSendTimeout(int fd, int timeoutMs);

    /**
     * @brief Set socket receive buffer size
     * @param fd Socket file descriptor
     * @param size Buffer size (bytes)
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setSocketReceiveBufferSize(int fd, int size);

    /**
     * @brief Set socket send buffer size
     * @param fd Socket file descriptor
     * @param size Buffer size (bytes)
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int setSocketSendBufferSize(int fd, int size);

    /**
     * @brief Get socket receive buffer size
     * @param fd Socket file descriptor
     * @return Buffer size (bytes), negative value indicates error code
     */
    static int getSocketReceiveBufferSize(int fd);

    /**
     * @brief Get socket send buffer size
     * @param fd Socket file descriptor
     * @return Buffer size (bytes), negative value indicates error code
     */
    static int getSocketSendBufferSize(int fd);

    /**
     * @brief Set recommended buffer sizes for UDP socket
     * @param fd Socket file descriptor
     * @param packetSize Expected packet size
     * @param maxPackets Maximum number of cached packets, default 64
     * @return true=success, false=failure
     */
    static bool setUdpRecommendedBufferSizes(int fd, int packetSize, int maxPackets = 64);

    /**
     * @brief Connect socket non-blockingly (with timeout)
     * @param fd Socket file descriptor
     * @param ip Target IP address
     * @param port Target port
     * @param timeoutMs Timeout (milliseconds)
     * @return Error code: UTILS_SOCKET_SUCCESS=success, other=failure
     */
    static int connectSocketNonBlocking(int fd, const std::string& ip, int port, int timeoutMs);

    /**
     * @brief Gracefully close socket (set linger then shutdown+close)
     * @param fd Socket file descriptor
     * @return true=success, false=failure
     */
    static bool gracefullyCloseSocket(int fd);

    /**
     * @brief Get last socket error code (cross-platform)
     * @return Socket error code
     */
    static int getLastSocketError();

    /**
     * @brief Check if error code is a "would block" type error
     * @param errorCode Error code
     * @return true=is blocking error, false=is not
     */
    static bool isWouldBlockError(int errorCode);

    /**
     * @brief Check if error code is connection-related
     * @param errorCode Error code
     * @return true=is connection error, false=is not
     */
    static bool isConnectionError(int errorCode);

    /**
     * @brief Check if error code is a timeout error
     * @param errorCode Error code
     * @return true=is timeout error, false=is not
     */
    static bool isTimeoutError(int errorCode);

    /**
     * @brief Retrieves a map of all active, non-loopback IPv4 addresses to their interface names.
     * @details This function scans all network interfaces and collects their IPv4 addresses
     *          and corresponding system interface names (e.g., "Ethernet 2" or "eth0").
     *          It excludes virtual machine adapters like VMware/VirtualBox by default.
     * @return A std::map where the key is the IPv4 address string (std::string) and
     *         the value is the interface name. On Windows, the value is std::wstring
     *         to correctly handle non-ASCII characters. On other platforms, it is std::string.
     */
#ifdef _WIN32
    static std::map<std::string, std::wstring> getAllLocalIpAndInterfaceNames();
#else
    static std::map<std::string, std::string> getAllLocalIpAndInterfaceNames();
#endif

    /**
     * @brief Finds the system interface name for a given local IPv4 address.
     * @details This is a convenience function that uses getAllLocalIpAndInterfaceNames()
     *          to look up the interface name corresponding to a specific IP address.
     * @param ipAddress The IPv4 address string to look up (e.g., "192.168.1.87").
     * @return The system interface name. On Windows, this is a std::wstring to correctly
     *         handle non-ASCII characters (e.g., L"以太网 2"). On other platforms,
     *         it is a std::string (e.g., "eth0"). Returns an empty string if not found.
     */
#ifdef _WIN32
    static std::wstring getInterfaceNameByIp(const std::string& ipAddress);
#else
    static std::string getInterfaceNameByIp(const std::string& ipAddress);
#endif

    /**
     * @brief Finds the local IPv4 address that matches a given network segment.
     * @details Scans all active, non-loopback network interfaces to find an IP
     *          address belonging to the specified network.
     * @param targetNetworkSegment The target network segment, e.g., "192.168.1".
     * @return The first matching IP address found (e.g., "192.168.1.100"),
     *         or an empty string if no match is found.
     */
    static std::string findLocalIpForNetwork(const std::string& targetNetworkSegment);
    static std::vector<std::string> findLocalIpsForNetwork(const std::string& targetNetworkSegment);

    /**
     * @brief Gets all active, non-loopback IPv4 addresses from the local machine.
     * @details This function is a simplified wrapper around getAllLocalIpAndInterfaceNames()
     *          that returns only the IP addresses.
     * @return A vector of strings, where each string is a local IPv4 address.
     */
    static std::vector<std::string> getAllLocalIPv4s();

    /**
     * @brief Checks if a given IPv4 address exists on the local machine.
     * @param ip The IPv4 address string to check.
     * @return true if the IP address is configured on any active local interface, false otherwise.
     */
    static bool isLocalIPv4Exists(const std::string& ip);
};

}

#endif // COMMON_UTILS_H
