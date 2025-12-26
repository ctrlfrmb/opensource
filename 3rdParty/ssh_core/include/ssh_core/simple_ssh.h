#ifndef SIMPLE_SSH_H
#define SIMPLE_SSH_H

/*-----------------------------------------------------------------------------
*               Copyright Notice
*-----------------------------------------------------------------------------
* Copyright (c) 2025. All rights reserved.
*
* This software is released under the MIT License;
* You may obtain a copy of the License at:
* https://opensource.org/licenses/MIT
*
* This is free software: you are free to use, modify and distribute,
* but must retain the author's copyright notice and license terms.
*
* Author: leiwei E-mail: ctrlfrmb@gmail.com
* Version: v1.4.1
* Date: 2025-06-21
*-----------------------------------------------------------------------------*/

/**
 * @file simple_ssh.h
 * @brief 极简 SSH 客户端 API 接口库。
 *
 * 本库基于 ssh_core 封装，提供了一套简单、统一的 C 风格接口，
 * 支持 SSH 连接管理、同步/异步命令执行以及 SFTP 文件传输。
 *
 * @example Simple Usage
 *
 *   // 1. 初始化日志 (可选)
 *   SimpleSSHOpenLog("ssh.log", 1, 5, 10);
 *
 *   // 2. 建立连接
 *   int id = SimpleSSHConnect("--host 192.168.1.100 --user root --pass 123456");
 *   if (id > 0) {
 *       // 3. 执行同步命令 (例如 ls -l)
 *       char buffer[4096] = {0};
 *       int exitCode = 0;
 *       int ret = SimpleSSHExecuteCmd(id, "ls -l", buffer, sizeof(buffer), &exitCode, 3000);
 *       if (ret == SIMPLE_SSH_SUCCESS) {
 *           printf("Output: %s\n", buffer);
 *       }
 *
 *       // 4. 上传文件
 *       SimpleSSHUploadFile(id, "local.txt", "/tmp/remote.txt");
 *
 *       // 5. 断开连接
 *       SimpleSSHClose(id);
 *   }
 *
 *   // 6. 关闭日志
 *   SimpleSSHCloseLog();
 */

#ifdef _WIN32
#ifdef BUILD_SSH_CORE
#define SIMPLE_SSH_API extern "C" __declspec(dllexport)
#else
#define SIMPLE_SSH_API extern "C" __declspec(dllimport)
#endif
#else
#define SIMPLE_SSH_API extern "C"
#endif

/**
* @brief API返回状态码定义
* 注意：此处数值与 ssh_core_types.h 完全一致，以便直接透传底层返回值。
*/
typedef enum {
    SIMPLE_SSH_SUCCESS = 0,                               ///< 操作成功

    // --- 通用错误 ---
    SIMPLE_SSH_ERROR_INVALID_PARAMETER = -1,              ///< 无效参数 (如空指针、无效ID)
    SIMPLE_SSH_ERROR_INTERNAL = -2,                       ///< 内部错误
    SIMPLE_SSH_ERROR_EXECUTE_FAILED = -3,                 ///< 系统命令执行失败
    SIMPLE_SSH_ERROR_TIMEOUT = -4,                        ///< 操作超时
    SIMPLE_SSH_ERROR_INVALID_STATE = -5,                  ///< 无效状态 (如未连接)

    // --- 连接错误 ---
    SIMPLE_SSH_ERROR_CONNECTION_FAILED = -10,             ///< TCP连接建立失败
    SIMPLE_SSH_ERROR_AUTHENTICATION = -11,                ///< 认证失败 (用户名或密码错误)
    SIMPLE_SSH_ERROR_ALGORITHM = -12,                     ///< 算法协商失败
    SIMPLE_SSH_ERROR_NETWORK = -13,                       ///< 网络通信错误

    // --- 通道与命令错误 ---
    SIMPLE_SSH_ERROR_CHANNEL_FAILURE = -20,               ///< 打开SSH通道失败
    SIMPLE_SSH_ERROR_CHANNEL_REQUEST_FAILED = -21,        ///< 请求执行命令失败
    SIMPLE_SSH_ERROR_CHANNEL_IO = -22,                    ///< 通道读写IO错误

    // --- SFTP 错误 ---
    SIMPLE_SSH_ERROR_SFTP_FAILURE = -30,                  ///< SFTP初始化失败
    SIMPLE_SSH_ERROR_SFTP_OPEN_FAILED = -31,              ///< 打开远程文件失败
    SIMPLE_SSH_ERROR_SFTP_READ_FAILED = -32,              ///< 读取远程文件失败
    SIMPLE_SSH_ERROR_SFTP_WRITE_FAILED = -33,             ///< 写入远程文件失败
    SIMPLE_SSH_ERROR_SFTP_MKDIR_FAILED = -34,             ///< 创建远程目录失败
    SIMPLE_SSH_ERROR_SFTP_STAT_FAILED = -35,              ///< 获取文件状态失败
    SIMPLE_SSH_ERROR_SFTP_LOCAL_FILE_ERROR = -36,         ///< 本地文件操作错误
    SIMPLE_SSH_ERROR_SFTP_NOT_A_DIRECTORY = -37,          ///< 远程路径不是目录
    SIMPLE_SSH_ERROR_SFTP_PERMISSION_DENIED = -38,        ///< 权限不足
    SIMPLE_SSH_ERROR_SFTP_NO_SUCH_FILE = -39,             ///< 远程文件不存在

    // --- 管理器错误 ---
    SIMPLE_SSH_ERROR_INVALID_ID = -51,                    ///< 无效的实例句柄
    SIMPLE_SSH_ERROR_INSTANCE_NOT_FOUND = -52,            ///< 实例ID不存在
    SIMPLE_SSH_ERROR_MAX_CLIENTS_REACHED = -53,           ///< 达到最大连接数限制
    SIMPLE_SSH_ERROR_BUFFER_TOO_SMALL = -54,              ///< 提供的缓冲区太小

    // --- 状态信息 (非错误) ---
    SIMPLE_SSH_STATUS_READ_EMPTY = -100                   ///< 读取缓冲区为空 (无数据)

} SimpleSSHStatus;

/*============================================================================
 * 日志管理
 *============================================================================*/

/**
* @brief 初始化并打开日志功能。
* @param logFile 日志文件路径，例如："logs/ssh_client.log"。
* @param level 日志等级 (-1:默认INFO 0:DEBUG 1:INFO 2:WARN 3:ERROR)。
*              特殊值: -200，表示记录ssh命令执行详细过程
* @param maxSize 单个日志文件最大大小(MB)，范围(1~100)，-1为默认10MB。
* @param maxFiles 保留的历史日志文件数量，范围(1~20)，-1为默认10个。
* @return 0: 成功, <0: 错误码。
*/
SIMPLE_SSH_API int SimpleSSHOpenLog(const char *logFile, int level, int maxSize, int maxFiles);

/**
* @brief 关闭日志功能。
* @return 0: 成功, <0: 错误码。
*/
SIMPLE_SSH_API int SimpleSSHCloseLog();

/*============================================================================
 * 连接管理
 *============================================================================*/

/**
* @brief 创建并建立一个SSH连接实例。
*
* 使用命令行风格的字符串来统一配置SSH连接的所有参数。
* 未在命令中指定的参数将保持其默认值。
*
* @param commands 配置字符串，例如 "--host 192.168.1.100 --user root --pass 123456 --timeout 3000"。
*                 支持的参数及其默认值:
*
*                 **[必需参数]**
*                 - `--host <ip>`: SSH服务器的IP地址或主机名。
*                 - `--user <name>`: 登录用户名。
*                 - `--pass <pwd>`: 登录密码。
*
*                 **[连接配置]**
*                 - `--port <num>`: SSH服务器端口 (默认: 22)。
*                 - `--localIp <ip>`: 指定本机绑定的网卡IP地址。
*                                   **若不指定，系统将自动选择路由。**
*                 - `--timeout <ms>`: 连接超时时间，单位毫秒 (默认: 5000)。
*
*                 **[高级选项]**
*                 - `--crypto <0|1|2>`: 加密算法策略 (默认: 0)。
*                                     **0: Default (推荐)**
*                                     **1: Compatible (兼容模式)**
*                                     **2: Legacy (传统模式)**
*                 - `--compression <0|1>`: 是否启用ZLIB数据压缩 (默认: 1)。
*                                        **0: 禁用, 1: 启用**
*                 - `--bufferSize <bytes>`: 内部接收缓冲区大小 (默认: 2MB)。
*
* @return >0: 成功，返回唯一的实例ID (Instance ID)。
*         <0: 失败，返回错误码 (参考 SimpleSSHStatus)。
*/
SIMPLE_SSH_API int SimpleSSHConnect(const char* commands);

/**
* @brief 关闭SSH连接并释放相关资源。
* @param instanceId 由 SimpleSSHConnect 返回的实例ID。
*/
SIMPLE_SSH_API void SimpleSSHClose(int instanceId);

/**
* @brief 检查指定实例是否处于连接状态。
* @param instanceId 实例ID。
* @return 1: 已连接, 0: 未连接。
*/
SIMPLE_SSH_API int SimpleSSHIsConnected(int instanceId);

/*============================================================================
 * 命令执行 (同步模式)
 *============================================================================*/

/**
* @brief 执行命令并等待结果返回 (同步阻塞)。
*
* 适用于执行时间较短的命令（如 ls, pwd, whoami 等）。
* 函数会阻塞直到命令执行完成或超时。
*
* @param instanceId 实例ID。
* @param cmdStr 要执行的Shell命令字符串。
* @param outputBuffer 用于存储命令标准输出(stdout)和标准错误(stderr)的缓冲区。
* @param bufferSize 缓冲区的大小。
* @param exitCode [out] 接收命令的退出码 (0通常表示成功)。
* @param timeoutMs 超时时间(ms)。
* @param execMode 执行模式， 0：单次执行，上下文无关联  1：关联执行（类XShell)，上下文相关。
* @return 0: 成功, <0: 错误码 (如 BUFFER_TOO_SMALL, TIMEOUT)。
*/
SIMPLE_SSH_API int SimpleSSHExecuteCmd(int instanceId, const char* cmdStr, char* outputBuffer, int bufferSize, int* exitCode, int timeoutMs, int execMode);

/*============================================================================
 * 命令执行 (异步模式)
 *============================================================================*/

/**
* @brief 启动异步命令执行。
*
* 调用此接口后命令会立即在后台开始执行，函数立即返回。
* 需配合 SimpleSSHReadCmdOutputAsync 获取实时输出。
*
* @param instanceId 实例ID。
* @param cmdStr 要执行的Shell命令字符串。
* @param timeoutMs 超时时间(ms)。
* @param execMode 执行模式， 0：单次执行，上下文无关联  1：关联执行（类XShell)，上下文相关。
* @return 0: 命令启动成功, <0: 错误码。
*/
SIMPLE_SSH_API int SimpleSSHStartCmdAsync(int instanceId, const char* cmdStr, int timeoutMs, int execMode);

/**
* @brief 读取异步命令的实时输出。
*
* 从内部缓冲区中取出数据。
*
* @param instanceId 实例ID。
* @param buffer 用于存储输出数据的缓冲区指针。
* @param bufferSize 缓冲区的大小。
* @param bytesRead [out] 实际读取到的字节数。
* @param timeoutMs 等待数据的超时时间(ms)。0表示非阻塞立即返回。
* @return 0: 成功 (SIMPLE_SSH_SUCCESS)。
*         -100: 读取为空 (SIMPLE_SSH_STATUS_READ_EMPTY)。
*         <0: 其他错误码。
*/
SIMPLE_SSH_API int SimpleSSHReadCmdOutputAsync(int instanceId, char* buffer, int bufferSize, int* bytesRead, int timeoutMs);

/**
* @brief 停止当前正在执行的异步命令。
* @param execMode 执行模式， 0：单次执行，上下文无关联  1：关联执行（类XShell)，上下文相关。
* @param instanceId 实例ID。
*/
SIMPLE_SSH_API void SimpleSSHStopCmdAsync(int instanceId, int execMode);

/**
* @brief 清空执行命令输出缓存（异步）
* @param instanceId 实例ID。
*/
SIMPLE_SSH_API void SimpleSSHClearOutputAsync(int instanceId);

/*============================================================================
 * 文件传输 (同步模式)
 *============================================================================*/

/**
* @brief 上传文件到远程主机 (同步阻塞)。
*
* @param instanceId 实例ID。
* @param localPath 本地文件的完整路径。
* @param remotePath 远程保存的完整路径 (包含文件名)。
* @return 0: 上传成功, <0: 错误码。
*/
SIMPLE_SSH_API int SimpleSSHUploadFile(int instanceId, const char* localPath, const char* remotePath);

/**
* @brief 从远程主机下载文件 (同步阻塞)。
*
* @param instanceId 实例ID。
* @param remotePath 远程文件的完整路径。
* @param localPath 本地保存的完整路径 (包含文件名)。
* @return 0: 下载成功, <0: 错误码。
*/
SIMPLE_SSH_API int SimpleSSHDownloadFile(int instanceId, const char* remotePath, const char* localPath);

#endif // SIMPLE_SSH_H
