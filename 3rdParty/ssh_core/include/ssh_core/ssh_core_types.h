/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2021 leiwei. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v2.2.0
* Date: 2021-12-15
*----------------------------------------------------------------------------*/

#ifndef SSH_CORE_TYPES_H
#define SSH_CORE_TYPES_H

/**
 * @file ssh_core_types.h
 * @brief Defines the public data types and return codes for the SSHCore C API.
 */

/**
 * @brief Defines the return status codes for all API functions.
 *
 * All functions that can fail will return a value from this enumeration.
 * A value of 0 (SSH_CORE_SUCCESS) indicates success, while any negative value
 * indicates an error.
 */
typedef enum {
    SSH_CORE_SUCCESS = 0,                               ///< Operation completed successfully.

    // --- General Errors ---
    SSH_CORE_ERROR_INVALID_PARAMETER = -1,              ///< An invalid parameter was passed to the function (e.g., null pointer, invalid ID).
    SSH_CORE_ERROR_INTERNAL = -2,                       ///< An unexpected internal error occurred in the library.
    SSH_CORE_ERROR_EXECUTE_FAILED = -3,                 ///< System command execution failed
    SSH_CORE_ERROR_TIMEOUT = -4,                        ///< The operation timed out.
    SSH_CORE_ERROR_INVALID_STATE = -5,                  ///< The operation was called in an invalid state (e.g., not connected).

    // --- Connection Errors ---
    SSH_CORE_ERROR_CONNECTION_FAILED = -10,             ///< Failed to establish a TCP connection to the host.
    SSH_CORE_ERROR_AUTHENTICATION = -11,                ///< Authentication (e.g., with password) failed.
    SSH_CORE_ERROR_ALGORITHM = -12,                     ///< Failed to negotiate a common algorithm with the server.
    SSH_CORE_ERROR_NETWORK = -13,                       ///< A generic network error occurred during an operation.

    // --- Channel & Command Errors (-2x) ---
    SSH_CORE_ERROR_CHANNEL_FAILURE = -20,               ///< Failed to open an SSH channel.
    SSH_CORE_ERROR_CHANNEL_REQUEST_FAILED = -21,        ///< Failed to request a PTY or execute a command on the channel.
    SSH_CORE_ERROR_CHANNEL_IO = -22,                    ///< An I/O error occurred while reading from or writing to the channel.

    // --- SFTP Errors (-3x) ---
    SSH_CORE_ERROR_SFTP_FAILURE = -30,                  ///< Failed to initialize the SFTP session.
    SSH_CORE_ERROR_SFTP_OPEN_FAILED = -31,              ///< Failed to open a file or directory on the remote host.
    SSH_CORE_ERROR_SFTP_READ_FAILED = -32,              ///< Failed to read from a remote file.
    SSH_CORE_ERROR_SFTP_WRITE_FAILED = -33,             ///< Failed to write to a remote file.
    SSH_CORE_ERROR_SFTP_MKDIR_FAILED = -34,             ///< Failed to create a directory on the remote host.
    SSH_CORE_ERROR_SFTP_STAT_FAILED = -35,              ///< Failed to get status/attributes of a remote file/directory.
    SSH_CORE_ERROR_SFTP_LOCAL_FILE_ERROR = -36,         ///< An error occurred with the local file (e.g., not found, can't open).
    SSH_CORE_ERROR_SFTP_NOT_A_DIRECTORY = -37,          ///< The remote path exists but is not a directory.
    SSH_CORE_ERROR_SFTP_PERMISSION_DENIED = -38,        ///< SFTP operation failed due to insufficient permissions.
    SSH_CORE_ERROR_SFTP_NO_SUCH_FILE = -39,             ///< The specified remote file or directory does not exist.

    // --- License Errors (-4x) ---
    SSH_CORE_ERROR_LICENSE_ACTIVATION_FAILED = -40,     ///< Failed to activate license on remote host.
    SSH_CORE_ERROR_LICENSE_VALIDATION_FAILED = -41,     ///< License validation failed.
    SSH_CORE_ERROR_LICENSE_REMOVAL_FAILED = -42,        ///< Failed to remove license from remote host.

    // --- Manager Errors (-5x) ---
    SSH_CORE_ERROR_INVALID_ID = -51,                    ///< Invalid instance handle.
    SSH_CORE_ERROR_INSTANCE_NOT_FOUND = -52,            ///< The provided instance ID does not correspond to an active connection.
    SSH_CORE_ERROR_MAX_CLIENTS_REACHED = -53,           ///< Could not connect because the maximum number of clients has been reached.
    SSH_CORE_ERROR_BUFFER_TOO_SMALL = -54,              ///< The provided buffer is too small to hold the result.

    // --- Informational Status (Non-Errors) ---
    SSH_CORE_STATUS_READ_EMPTY = -100                   ///< The read operation found no data in the buffer; this is not an error.

} SSH_CORE_STATUS;

/**
 * @brief Defines standard prefixes for lines pushed to the asynchronous command buffer.
 *
 * These prefixes allow consumers of the API to easily parse the output type.
 */
#define SSH_CORE_PREFIX_CMD  "[cmd] "   ///< Prefix for the command itself.
#define SSH_CORE_PREFIX_OUT  "[out] "   ///< Prefix for standard output lines.
#define SSH_CORE_PREFIX_ERR  "[err] "   ///< Prefix for standard error lines.
#define SSH_CORE_PREFIX_EXIT "[exit] "  ///< Prefix for the final exit code message.

#define SSH_CORE_PREFIX_RECONNECTED  "[reconnected]"   ///< Prefix for reconnection successful.

#endif // SSH_CORE_TYPES_H
