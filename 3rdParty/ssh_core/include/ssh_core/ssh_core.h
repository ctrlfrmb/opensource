#ifndef SSH_CORE_H
#define SSH_CORE_H

#include "common_global.h"
#include <QString>
#include <QObject>
#include <memory>

namespace SSHCore {

// --- 公共数据结构 ---

/**
 * @brief SSH加密算法配置类型
 */
enum CryptoType {
    CRYPTO_DEFAULT = 0,     // 默认，安全性最高
    CRYPTO_COMPATIBLE = 1,  // 兼容模式，支持一些旧算法
    CRYPTO_LEGACY = 2       // 遗留模式，用于非常老的设备
};

/**
 * @brief SSH连接信息
 */
struct ConnectionInfo {
    QString host;
    int port = 22;
    QString username;
    QString password;
    QString localIP;        // 可选，用于绑定本地网卡IP
    int timeoutMs = 5000;   // 连接超时时间

    ConnectionInfo() = default;
    ConnectionInfo(const QString& host, int port, const QString& username, const QString& password,
                   const QString& localIP = QString(), int timeoutMs = 5000)
        : host(host), port(port), username(username), password(password), localIP(localIP), timeoutMs(timeoutMs) {}

    bool isValid() const { return !host.isEmpty() && !username.isEmpty(); }
};


// --- 主接口类 ---

// 前置声明私有实现类
class HelperPrivate;

/**
 * @brief SSH/SFTP 核心功能类
 *
 * 提供SSH连接、命令执行(同步/异步)和SFTP文件传输功能。
 * 所有操作都是线程安全的。
 */
class COMMON_API_EXPORT Helper : public QObject {
    Q_OBJECT

public:
    explicit Helper(QObject *parent = nullptr);
    ~Helper() override;

    bool connectToHost(const ConnectionInfo& info);
    bool connectToHost(const QString &host, int port, const QString &username, const QString &password,
                       const QString &localIP = QString(), int timeoutMs = 5000);
    void disconnectFromHost();
    bool isConnected() const;
    bool isAsyncCommandRunning();
    bool executeCommandAsync(const QString &command, int timeoutMs = 0);
    void stopCommandAsync();
    int executeCommandSync(const QString &command, QString &output, int timeoutMs = 5000);
    int executeCommandSync(const QString &command, int timeoutMs = 5000);
    bool uploadFile(const QString &localFilePath, const QString &remoteFilePath, bool makeExecutable = false);
    bool downloadFile(const QString &remoteFilePath, const QString &localFilePath);
    void setCryptoType(CryptoType type);
    void setCompression(bool enabled);
    void setKnownHostsFile(const QString &filePath);
    bool ensureRemoteDirectoryExists(const QString &dirPath);
    bool writeContentToRemoteFile(const QString &content, const QString &remoteFilePath);
    bool setRemoteFileExecutable(const QString &remoteFilePath);

signals:
    void reconnected();
    void commandOutput(const QString &output);
    void commandError(const QString &error);
    void commandFinished(int exitCode);
    void fileTransferProgress(qint64 bytesTransferred, qint64 totalBytes);

private:
    // 指向私有实现的指针 (PIMPL)
    std::unique_ptr<HelperPrivate> d_ptr;
};

} // namespace SSHCore

#endif // SSH_CORE_H
