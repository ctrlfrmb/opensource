// ssh_command_worker.cpp

#include "ssh_command_worker.h"
#include "ssh_core_types.h"
#include "logger_macros.h"
#include "utils.h"

#include <libssh2.h>
#include <chrono>

namespace SSHCore {

CommandWorker::CommandWorker(LIBSSH2_SESSION *session, std::mutex* sessionMutex, const QString &command, int timeoutMs, QObject *parent)
    : QThread(parent)
    , m_session(session)
    , m_sessionMutex(sessionMutex)
    , m_command(command)
    , m_timeoutMs(timeoutMs)
{
}

CommandWorker::~CommandWorker()
{
    stopCommand();
    wait(READ_TIMEOUT_MS * 2);
}

bool CommandWorker::hasExecuted() const
{
    return m_hasExecuted.load(std::memory_order_acquire);
}

int CommandWorker::exitCode() const
{
    return m_exitCode.load(std::memory_order_acquire);
}

void CommandWorker::stopCommand()
{
    if (!m_stopRequested.exchange(true, std::memory_order_release)) {
        m_condition.notify_all();
    }
}

void CommandWorker::run()
{
    m_exitCode.store(SSH_CORE_SUCCESS, std::memory_order_release);

    if (!m_session || !m_sessionMutex) {
        emit sigErrorReceived("Session is not connected.");
        m_exitCode.store(SSH_CORE_ERROR_INVALID_STATE, std::memory_order_release);
        emit sigCommandFinished(m_exitCode.load(std::memory_order_relaxed));
        return;
    }

    LIBSSH2_CHANNEL *sshChannel = nullptr;
    auto timeStart = std::chrono::steady_clock::now();

    // 步骤 1: 快速地建立并配置好 channel
    if (!setupChannel(sshChannel)) {
        emit sigCommandFinished(m_exitCode.load(std::memory_order_relaxed)); // 外部是直连信号，注意锁的使用
        return;
    }

    // 步骤 2: 执行所有耗时的网络 I/O
    emit sigStarted();
    m_hasExecuted.store(true, std::memory_order_release);
    executeMainLoop(sshChannel, timeStart);

    // 步骤 3: 快速地清理 channel 资源
    cleanupChannel(sshChannel);

    // 最终状态处理
    if (m_stopRequested.load(std::memory_order_acquire)) {
        m_exitCode.store(SSH_CORE_SUCCESS, std::memory_order_release);
        LOG_INFO("The user manually stops the command execution");
    }

    emit sigCommandFinished(m_exitCode.load(std::memory_order_relaxed));
    LOG_DEBUG("Execution of command completed in thread {}, final exitCode {}",
              Common::Utils::getThreadIdString(), m_exitCode.load(std::memory_order_relaxed));
}

// TODO MD setupChannel
bool CommandWorker::setupChannel(LIBSSH2_CHANNEL*& sshChannel)
{
    std::lock_guard<std::mutex> lock(*m_sessionMutex);
    if (!m_session) return false;

    // 临时设置为阻塞模式，保证初始化和写入命令顺利完成
    libssh2_session_set_blocking(m_session, 1);

    // 1. 打开会话通道
    sshChannel = libssh2_channel_open_session(m_session);
    if (!sshChannel) {
        logLibSSHError_unsafe("Failed to open channel", SSH_CORE_ERROR_CHANNEL_FAILURE);
        return false;
    }

    // 2. 设置环境变量 (模拟 Xshell)
    libssh2_channel_setenv(sshChannel, "TERM", "xterm");
    libssh2_channel_setenv(sshChannel, "LANG", "en_US.UTF-8");

    // 3. 构建终端模式 (Terminal Modes)
    // 这一点至关重要，它告诉服务器这是一个“健全”的终端，不要轻易因为信号而关闭
    const char terminal_modes[] = {
        53, 0, 0, 75, 0, // OpSpeed 19200
        54, 0, 0, 75, 0, // IpSpeed 19200
        0
    };

    // 4. 请求伪终端 (PTY)
    // 宽 160, 高 48：模拟大窗口，防止日志折行或崩溃
    if (libssh2_channel_request_pty_ex(sshChannel, "xterm", sizeof("xterm") - 1,
                                       terminal_modes, sizeof(terminal_modes),
                                       160, 48, 0, 0) != 0) {
        LOG_WARN("Failed to request PTY xterm, falling back to vanilla");
        libssh2_channel_request_pty(sshChannel, "vanilla");
    }

    // 5. 【关键修改】启动 Shell 模式，而不是 Exec 模式
    // 这样即使命令执行完了，Shell 还在，通道就不会关闭 (EOF)
    if (libssh2_channel_shell(sshChannel) != 0) {
        logLibSSHError_unsafe("Failed to start shell", SSH_CORE_ERROR_CHANNEL_REQUEST_FAILED);
        libssh2_channel_free(sshChannel);
        sshChannel = nullptr;
        return false;
    }

    // 6. 【关键修改】手动将命令写入 Shell (模拟用户打字 + 回车)
    if (!m_command.isEmpty()) {
        QByteArray cmdData = m_command.toUtf8();
        // 如果命令末尾没有换行符，手动补一个，否则命令不会执行
        if (!cmdData.endsWith('\n')) {
            cmdData.append('\n');
        }

        ssize_t written = 0;
        ssize_t total = cmdData.size();
        const char* ptr = cmdData.constData();

        // 循环写入确保命令发送完整
        while (written < total) {
            ssize_t rc = libssh2_channel_write_ex(sshChannel, 0, ptr + written, total - written);
            if (rc < 0) {
                logLibSSHError_unsafe("Failed to write command to shell", SSH_CORE_ERROR_CHANNEL_IO);
                // 注意：这里是否退出取决于策略，一般 shell 打开了就能读，只是命令没发出去
                break;
            }
            written += rc;
        }
    }

    // 恢复非阻塞模式，交给后续的 Loop 进行读取
    libssh2_session_set_blocking(m_session, 0);
    return true;
}
 
// MD
// bool CommandWorker::setupChannel(LIBSSH2_CHANNEL*& sshChannel)
// {
//     std::lock_guard<std::mutex> lock(*m_sessionMutex);
//     if (!m_session) return false;

//     libssh2_session_set_blocking(m_session, 1);

//     sshChannel = libssh2_channel_open_session(m_session);
//     if (!sshChannel) {
//         logLibSSHError_unsafe("Failed to open channel", SSH_CORE_ERROR_CHANNEL_FAILURE);
//         return false;
//     }

//     libssh2_channel_setenv(sshChannel, "STDBUF", "0");

//     // 修改为：模拟 xterm 终端，并设置常见的宽高（Xshell 默认可能是 80x24 或更大）
//     if (libssh2_channel_request_pty_ex(sshChannel, "xterm", sizeof("xterm") - 1, NULL, 0, 80, 24, 0, 0) != 0) {
//         LOG_WARN("Failed to request PTY xterm");
//         // 如果失败回退到 vanilla
//         libssh2_channel_request_pty(sshChannel, "vanilla");
//     }

//     if (libssh2_channel_exec(sshChannel, qPrintable(m_command)) != 0) {
//         logLibSSHError_unsafe("Failed to execute command", SSH_CORE_ERROR_CHANNEL_REQUEST_FAILED);
//         // 需要在这里清理已打开的 channel
//         libssh2_channel_free(sshChannel);
//         sshChannel = nullptr;
//         return false;
//     }

//     libssh2_session_set_blocking(m_session, 0);
//     return true;
// }

void CommandWorker::executeMainLoop(LIBSSH2_CHANNEL* sshChannel, const std::chrono::steady_clock::time_point& timeStart)
{
    char buffer[4096];
    QString stdoutCache, stderrCache;
    stdoutCache.reserve(16384);
    stderrCache.reserve(16384);

    while (m_exitCode.load(std::memory_order_acquire) == SSH_CORE_SUCCESS &&
           !m_stopRequested.load(std::memory_order_acquire)) {

        if (m_timeoutMs > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - timeStart).count();
            if (elapsed >= m_timeoutMs) {
                LOG_WARN("Command execution timed out.");
                emit sigErrorReceived("Command execution timed out.");
                m_exitCode.store(SSH_CORE_ERROR_TIMEOUT, std::memory_order_release);
                break;
            }
        }

        ssize_t stdout_read, stderr_read;
        do {
            stdout_read = libssh2_channel_read(sshChannel, buffer, sizeof(buffer));
            if (stdout_read > 0) stdoutCache.append(QString::fromUtf8(buffer, static_cast<int>(stdout_read)));

            stderr_read = libssh2_channel_read_stderr(sshChannel, buffer, sizeof(buffer));
            if (stderr_read > 0) stderrCache.append(QString::fromUtf8(buffer, static_cast<int>(stderr_read)));
        } while (stdout_read > 0 || stderr_read > 0);

        if (!stdoutCache.isEmpty()) { emit sigOutputReceived(stdoutCache); stdoutCache.clear(); }
        if (!stderrCache.isEmpty()) { emit sigErrorReceived(stderrCache); stderrCache.clear(); }

        if ((stdout_read < 0 && stdout_read != LIBSSH2_ERROR_EAGAIN) ||
            (stderr_read < 0 && stderr_read != LIBSSH2_ERROR_EAGAIN)) {
            // 这里不能调用 logLibSSHError_unsafe 因为它需要锁
            LOG_ERROR("Error reading from channel (code stdout: {}, stderr: {})", stdout_read, stderr_read);
            emit sigErrorReceived(QString("Error reading from channel (code: %1)").arg(stdout_read));
            m_exitCode.store(SSH_CORE_ERROR_CHANNEL_IO, std::memory_order_release);
            break;
        }

        if (libssh2_channel_eof(sshChannel) == 1) {
            LOG_DEBUG("EOF received from server.");
            break;
        }

        std::unique_lock<std::mutex> cvLocker(m_cvMutex);
        m_condition.wait_for(cvLocker, std::chrono::milliseconds(READ_TIMEOUT_MS));
    }
}

void CommandWorker::cleanupChannel(LIBSSH2_CHANNEL* sshChannel)
{
    if (!sshChannel) return;

    if (!m_stopRequested.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(*m_sessionMutex);
        libssh2_session_set_blocking(m_session, 1);
        m_exitCode.store(libssh2_channel_get_exit_status(sshChannel), std::memory_order_release);
        libssh2_session_set_blocking(m_session, 0);
    }

    libssh2_channel_send_eof(sshChannel);
    libssh2_channel_free(sshChannel);
}

void CommandWorker::logLibSSHError_unsafe(const QString &description, int errorCode)
{
    // 这个函数现在在锁的保护下被调用
    int expected_success = SSH_CORE_SUCCESS;
    m_exitCode.compare_exchange_strong(expected_success, errorCode, std::memory_order_release);

    char *errmsg;
    int errlen;
    int err = libssh2_session_last_error(m_session, &errmsg, &errlen, 0);
    QString fullError = QString("%1: %2 (code: %3)").arg(description).arg(errmsg ? errmsg : "Unknown error").arg(err);
    emit sigErrorReceived(fullError);
    LOG_ERROR("{}", qPrintable(fullError));
}

} // namespace SSHCore
