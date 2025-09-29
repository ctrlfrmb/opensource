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

/**
* @file ssh_core_api.h
* @brief A thread-safe, high-level C API for SSH and SFTP communication.
*
* This API provides a stable and thread-safe C interface for the SSHCore C++ library.
* It is designed for easy integration into various programming languages. The core
* design uses an instance ID system to manage multiple, concurrent SSH connections
* safely from different threads.
*
* Key Features:
* - Simple Connection Management
* - Asynchronous & Synchronous Command Execution
* - Robust Synchronous SFTP Support
* - License Management
* - Thread-Safe by Design
*
* --- Asynchronous Command Workflow ---
* The recommended workflow for asynchronous commands is as follows:
* 1. Call SSHClientStartCommandAsync() to start a command.
* 2. In a loop, call SSHClientIsCommandRunningAsync() to check if the command is active.
* 3. While it's running, you can call SSHClientReadOutputAsync() to get live output.
* 4. When SSHClientIsCommandRunningAsync() returns 0, the command has finished.
* 5. Call SSHClientGetCommandResultAsync() to get the final exit code.
*
* --- Synchronous Operations ---
* Functions ending with "Sync" are blocking. They will wait until the operation
* is complete or a timeout occurs before returning.
*
*/

#ifndef SSH_CORE_API_H
#define SSH_CORE_API_H

#include "ssh_core_types.h"

// Define platform-specific import/export macros
#ifdef _WIN32
   #ifdef BUILD_SSH_CORE
       #define SSH_CORE_API extern "C" __declspec(dllexport)
   #else
       #define SSH_CORE_API extern "C" __declspec(dllimport)
   #endif
#else
   #define SSH_CORE_API extern "C"
#endif

/**
 * @brief Opens the log for debugging and troubleshooting.
 *
 * @warning This function is for debugging only. It impacts performance and
 *          storage and should not be used in production builds.
 *
 * @param logFile   The path for the log file, e.g., "logs/doip.log".
 * @param level     The log level (-1 for default INFO; 0:DEBUG, 1:INFO, 2:WARN, 3:ERROR).
 * @param maxSize   Max file size in MB (1-20, -1 for default 10MB).
 * @param maxFiles  Max number of files to keep (1-20, -1 for default 10).
 * @return Returns 0 if successful, otherwise returns a non-zero value.
 */
SSH_CORE_API int SSHOpenLog(const char *logFile, int level, int maxSize, int maxFiles);

/**
 * @brief Closes the log.
 * @return Returns 0 on success, otherwise a non-zero value.
 */
SSH_CORE_API int SSHCloseLog();

/*============================================================================
 * Connection Management
 *============================================================================*/

/**
 * @brief Establishes an SSH connection to a remote host.
 * @param connectionString A space-separated string of command-line style arguments.
 *                         Required: --host, --user, --pass.
 *                         Optional: --port, --timeout, --localIp, --crypto, --compression, --bufferSize.
 * @return On success, a positive integer instance ID. On failure, a negative SSH_CORE_STATUS error code.
 */
SSH_CORE_API int SSHClientConnect(const char* connectionString);

/**
 * @brief Closes an SSH connection and releases all associated resources.
 * @param instanceId The ID of the client instance to close.
 */
SSH_CORE_API void SSHClientClose(int instanceId);

/**
 * @brief Checks if a client instance is currently connected.
 * @param instanceId The ID of the client instance to check.
 * @return 1 if connected, 0 if not.
 */
SSH_CORE_API int SSHClientIsConnected(int instanceId);

/*============================================================================
 * Asynchronous Command Execution
 *============================================================================*/

/**
 * @brief Starts a command asynchronously on the remote host.
 * @param instanceId The ID of the client instance.
 * @param command The command string to execute.
 * @param timeoutMs The timeout for the command execution in milliseconds (0 for no timeout).
 * @return SSH_CORE_SUCCESS if the command was successfully started, or an error code.
 */
SSH_CORE_API int SSHClientStartCommandAsync(int instanceId, const char* command, int timeoutMs);

/**
 * @brief Reads available output from the running asynchronous command's buffer.
 * @param instanceId The ID of the client instance.
 * @param buffer A character buffer to store the output.
 * @param bufferSize The size of the provided buffer.
 * @param bytesRead [out] Pointer to an integer that will receive the number of bytes written to the buffer.
 * @param timeoutMs The maximum time to wait for output, in milliseconds (0 for non-blocking).
 * @return SSH_CORE_SUCCESS on success, SSH_CORE_STATUS_READ_EMPTY if no data is available, or an error code.
 */
SSH_CORE_API int SSHClientReadOutputAsync(int instanceId, char* buffer, int bufferSize, int* bytesRead, int timeoutMs);

/**
 * @brief Requests to stop the currently running asynchronous command.
 * @param instanceId The ID of the client instance.
 */
SSH_CORE_API void SSHClientStopCommandAsync(int instanceId);

/**
 * @brief Checks if an asynchronous command is currently running.
 * @param instanceId The ID of the client instance.
 * @return 1 if a command is running, 0 otherwise.
 */
SSH_CORE_API int SSHClientIsCommandRunningAsync(int instanceId);

/*============================================================================
 * Synchronous Command Execution
 *============================================================================*/

/**
 * @brief Executes a command synchronously, waits for it to complete, and captures its output.
 * @param instanceId The ID of the client instance.
 * @param command The command string to execute.
 * @param outputBuffer A character buffer to store the command's combined stdout and stderr.
 * @param bufferSize The size of the provided output buffer.
 * @param exitCode [out] A pointer to an integer that will receive the command's exit code or an error code if execution fails.
 * @param timeoutMs Timeout in milliseconds for the entire operation.
 * @return SSH_CORE_SUCCESS on success, SSH_CORE_ERROR_BUFFER_TOO_SMALL if output exceeds bufferSize, or another error code.
 */
SSH_CORE_API int SSHClientExecuteCommandSync(int instanceId, const char* command, char* outputBuffer, int bufferSize, int* exitCode, int timeoutMs);

/*============================================================================
 * Synchronous SFTP File Operations
 *============================================================================*/

/**
 * @brief Uploads a local file to a remote path using SFTP (synchronous).
 * @param instanceId The ID of the client instance.
 * @param localPath The path to the local file.
 * @param remotePath The destination path on the remote server.
 * @param makeExecutable If 1, attempts to set execute permission on the remote file after upload.
 * @return SSH_CORE_SUCCESS on success, or an SFTP error code.
 */
SSH_CORE_API int SSHClientUploadFileSync(int instanceId, const char* localPath, const char* remotePath, int makeExecutable);

/**
 * @brief Downloads a remote file to a local path using SFTP (synchronous).
 * @param instanceId The ID of the client instance.
 * @param remotePath The path to the remote file.
 * @param localPath The destination path on the local machine.
 * @return SSH_CORE_SUCCESS on success, or an SFTP error code.
 */
SSH_CORE_API int SSHClientDownloadFileSync(int instanceId, const char* remotePath, const char* localPath);

/**
 * @brief Writes content directly to a remote file using SFTP (synchronous).
 * @param instanceId The ID of the client instance.
 * @param content The content to write to the remote file.
 * @param remotePath The destination path on the remote server.
 * @param makeExecutable If 1, attempts to set execute permission on the remote file after upload.
 * @return SSH_CORE_SUCCESS on success, or an SFTP error code.
 */
SSH_CORE_API int SSHClientWriteContentToRemoteFileSync(int instanceId, const char* content, const char* remotePath, int makeExecutable);

/**
 * @brief Ensures that a remote directory exists, creating it if necessary (synchronous).
 * @param instanceId The ID of the client instance.
 * @param dirPath The path to the remote directory.
 * @return SSH_CORE_SUCCESS on success, or an SFTP error code.
 */
SSH_CORE_API int SSHClientEnsureRemoteDirectoryExistsSync(int instanceId, const char* dirPath);

/**
 * @brief Sets execute permission on a remote file (synchronous).
 * @param instanceId The ID of the client instance.
 * @param remotePath The path to the remote file.
 * @return SSH_CORE_SUCCESS on success, or an SFTP error code.
 */
SSH_CORE_API int SSHClientSetRemoteFileExecutableSync(int instanceId, const char* remotePath);

/**
 * @brief Gets the progress of the current file transfer operation (upload/download).
 * @param instanceId The ID of the client instance.
 * @param transferredBytes [out] Pointer to an integer to receive the bytes transferred so far.
 * @param totalBytes [out] Pointer to an integer to receive the total file size.
 * @return SSH_CORE_SUCCESS on success, or other error code.
 */
SSH_CORE_API int SSHClientGetFileProgressAsync(int instanceId, int* transferredBytes, int* totalBytes);

/*============================================================================
 * License Management
 *============================================================================*/

/**
 * @brief Activates a license on the remote host.
 * @param instanceId The ID of the client instance.
 * @return SSH_CORE_SUCCESS on success, or a license error code.
 */
SSH_CORE_API int SSHClientActivateLicense(int instanceId);

/**
 * @brief Validates the license on the remote host.
 * @param instanceId The ID of the client instance.
 * @return SSH_CORE_SUCCESS if license is valid, or a license error code.
 */
SSH_CORE_API int SSHClientValidateLicense(int instanceId);

/**
 * @brief Removes the license from the remote host.
 * @param instanceId The ID of the client instance.
 * @return SSH_CORE_SUCCESS on success, or a license error code.
 */
SSH_CORE_API int SSHClientRemoveLicense(int instanceId);

#endif // SSH_CORE_API_H
