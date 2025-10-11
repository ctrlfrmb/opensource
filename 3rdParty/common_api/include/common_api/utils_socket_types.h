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
* Version: v1.0.0
* Date: 2022-02-04
*----------------------------------------------------------------------------*/

#ifndef COMMON_UTILS_SOCKET_TYPES_H
#define COMMON_UTILS_SOCKET_TYPES_H

namespace Common {

/**
 * @file utils_socket_types.h
 * @brief Defines socket error codes for TCP/UDP network operations.
 *
 * This header provides a unified error code system for all socket operations
 * used by Utils class and TCP/UDP client implementations. All socket-related
 * functions should return values from these error code definitions.
 */

/**
 * @brief Socket operation error codes.
 *
 * All socket functions return these error codes where:
 * - 0 indicates success
 * - Negative values indicate specific errors
 * - Positive values may indicate system-specific error codes
 */
enum {
    UTILS_SOCKET_SUCCESS = 0,                           ///< Operation completed successfully

    // --- General Socket Errors (1-99) ---
    UTILS_SOCKET_ERROR_INVALID_PARAMETER = -1,          ///< Invalid parameter (null pointer, invalid fd, etc.)
    UTILS_SOCKET_ERROR_INVALID_ADDRESS = -2,            ///< Invalid IP address format
    UTILS_SOCKET_ERROR_INVALID_PORT = -3,               ///< Invalid port number
    UTILS_SOCKET_ERROR_TIMEOUT = -4,                    ///< Operation timed out
    UTILS_SOCKET_ERROR_PERMISSION_DENIED = -5,          ///< Permission denied
    UTILS_SOCKET_ERROR_ADDRESS_IN_USE = -6,             ///< Address already in use
    UTILS_SOCKET_ERROR_NETWORK_UNREACHABLE = -7,        ///< Network unreachable
    UTILS_SOCKET_ERROR_UNKNOWN = -10,                   ///< Unknown error occurred

    // --- Socket Creation and Configuration (100-199) ---
    UTILS_SOCKET_ERROR_CREATE_FAILED = -100,            ///< Failed to create socket
    UTILS_SOCKET_ERROR_BIND_FAILED = -101,              ///< Failed to bind socket to address
    UTILS_SOCKET_ERROR_LISTEN_FAILED = -102,            ///< Failed to put socket in listen mode
    UTILS_SOCKET_ERROR_ACCEPT_FAILED = -103,            ///< Failed to accept connection
    UTILS_SOCKET_ERROR_SET_OPTION_FAILED = -104,        ///< Failed to set socket option
    UTILS_SOCKET_ERROR_GET_OPTION_FAILED = -105,        ///< Failed to get socket option
    UTILS_SOCKET_ERROR_SET_NONBLOCKING_FAILED = -106,   ///< Failed to set non-blocking mode
    UTILS_SOCKET_ERROR_SET_BLOCKING_FAILED = -107,      ///< Failed to set blocking mode
    UTILS_SOCKET_ERROR_GET_NAME_FAILED = -108,          ///< Failed to get socket name
    UTILS_SOCKET_ERROR_CLOSE_FAILED = -109,             ///< Failed to close socket

    // --- TCP Connection Errors (200-299) ---
    UTILS_SOCKET_ERROR_TCP_CONNECT_FAILED = -200,       ///< TCP connection failed
    UTILS_SOCKET_ERROR_TCP_CONNECT_TIMEOUT = -201,      ///< TCP connection timed out
    UTILS_SOCKET_ERROR_TCP_CONNECTION_REFUSED = -202,   ///< TCP connection refused by server
    UTILS_SOCKET_ERROR_TCP_CONNECTION_RESET = -203,     ///< TCP connection reset by peer
    UTILS_SOCKET_ERROR_TCP_CONNECTION_ABORTED = -204,   ///< TCP connection aborted
    UTILS_SOCKET_ERROR_TCP_CONNECTION_CLOSED = -205,    ///< TCP connection closed by peer
    UTILS_SOCKET_ERROR_TCP_SEND_FAILED = -206,          ///< TCP send operation failed
    UTILS_SOCKET_ERROR_TCP_RECEIVE_FAILED = -207,       ///< TCP receive operation failed
    UTILS_SOCKET_ERROR_TCP_SHUTDOWN_FAILED = -208,      ///< TCP shutdown failed
    UTILS_SOCKET_ERROR_TCP_NODELAY_FAILED = -209,       ///< Failed to set TCP_NODELAY
    UTILS_SOCKET_ERROR_TCP_KEEPALIVE_FAILED = -210,     ///< Failed to set TCP keep-alive

    // --- UDP Communication Errors (300-399) ---
    UTILS_SOCKET_ERROR_UDP_SEND_FAILED = -300,          ///< UDP send operation failed
    UTILS_SOCKET_ERROR_UDP_RECEIVE_FAILED = -301,       ///< UDP receive operation failed
    UTILS_SOCKET_ERROR_UDP_SENDTO_FAILED = -302,        ///< UDP sendto operation failed
    UTILS_SOCKET_ERROR_UDP_RECVFROM_FAILED = -303,      ///< UDP recvfrom operation failed
    UTILS_SOCKET_ERROR_UDP_BROADCAST_FAILED = -304,     ///< UDP broadcast failed
    UTILS_SOCKET_ERROR_UDP_MULTICAST_JOIN_FAILED = -305, ///< Failed to join multicast group
    UTILS_SOCKET_ERROR_UDP_MULTICAST_LEAVE_FAILED = -306, ///< Failed to leave multicast group
    UTILS_SOCKET_ERROR_UDP_SET_BROADCAST_FAILED = -307,  ///< Failed to enable broadcast
    UTILS_SOCKET_ERROR_UDP_PACKET_TOO_LARGE = -308,     ///< UDP packet exceeds maximum size

    // --- Buffer and Timeout Configuration (400-499) ---
    UTILS_SOCKET_ERROR_SET_SEND_BUFFER_FAILED = -400,   ///< Failed to set send buffer size
    UTILS_SOCKET_ERROR_SET_RECV_BUFFER_FAILED = -401,   ///< Failed to set receive buffer size
    UTILS_SOCKET_ERROR_GET_SEND_BUFFER_FAILED = -402,   ///< Failed to get send buffer size
    UTILS_SOCKET_ERROR_GET_RECV_BUFFER_FAILED = -403,   ///< Failed to get receive buffer size
    UTILS_SOCKET_ERROR_SET_SEND_TIMEOUT_FAILED = -404,  ///< Failed to set send timeout
    UTILS_SOCKET_ERROR_SET_RECV_TIMEOUT_FAILED = -405,  ///< Failed to set receive timeout
    UTILS_SOCKET_ERROR_SET_LINGER_FAILED = -406,        ///< Failed to set SO_LINGER option
    UTILS_SOCKET_ERROR_SET_REUSEADDR_FAILED = -407,     ///< Failed to set SO_REUSEADDR option

    // --- Platform Initialization Errors (500-599) ---
    UTILS_SOCKET_ERROR_WINSOCK_INIT_FAILED = -500,      ///< Windows Winsock initialization failed
    UTILS_SOCKET_ERROR_WINSOCK_CLEANUP_FAILED = -501,   ///< Windows Winsock cleanup failed
    UTILS_SOCKET_ERROR_SELECT_FAILED = -502,            ///< select() call failed
    UTILS_SOCKET_ERROR_POLL_FAILED = -503,              ///< poll() call failed
    UTILS_SOCKET_ERROR_EPOLL_FAILED = -504,             ///< epoll operation failed

    // --- Address Resolution Errors (600-699) ---
    UTILS_SOCKET_ERROR_HOSTNAME_RESOLUTION_FAILED = -600, ///< Hostname resolution failed
    UTILS_SOCKET_ERROR_ADDRESS_FAMILY_NOT_SUPPORTED = -601, ///< Address family not supported
    UTILS_SOCKET_ERROR_PARSE_ADDRESS_FAILED = -602,     ///< Failed to parse address string
    UTILS_SOCKET_ERROR_GET_LOCAL_ADDRESS_FAILED = -603,  ///< Failed to get local address
    UTILS_SOCKET_ERROR_GET_REMOTE_ADDRESS_FAILED = -604, ///< Failed to get remote address

    // --- Special Return Values (Non-errors) ---
    UTILS_SOCKET_WOULD_BLOCK = 1,                       ///< Operation would block (non-blocking mode)
    UTILS_SOCKET_CONNECTION_IN_PROGRESS = 2             ///< Connection is in progress (async connect)
};

}

#endif // COMMON_UTILS_SOCKET_TYPES_H
