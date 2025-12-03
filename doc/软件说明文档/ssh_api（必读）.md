# SimpleSSH API 说明文档

> 版本：v1.3.0 | 作者：leiwei | 日期：2025-12-02

## 📖 简介

SimpleSSH API 是一个基于 `libssh2` 和 `Qt5` 封装的高性能 SSH 客户端接口库。它旨在简化复杂的 SSH 协议操作，提供一套线程安全、简单易用的 C 风格接口，适用于自动化运维、远程设备控制及文件传输场景。

**核心特性**：

- **极简接口**：通过单行字符串配置连接参数，集成自动重连机制。
- **灵活执行**：支持同步阻塞执行（适合短命令）和异步流式执行（适合长任务）。
- **智能 SFTP**：集成文件传输功能，支持智能路径识别。
- **多网卡支持**：支持绑定本地特定网卡 IP 进行连接（针对多网段设备调试）。
- **日志系统**：内置分级日志记录，支持自动滚动和归档。
- **跨平台**：底层基于标准 C++ 实现，提供 Windows (x86/x64) 动态库。

## 📁 目录结构

```bash
SimpleSSH/
├── README.pdf                    # 用户说明书
├── SimpleSSHDef.py               # Python 常量定义
├── SimpleSSHReleaseNote.txt      # 版本变更记录
├── SimpleSSHTest.py              # Python 交互式测试工具 (推荐)
├── SimpleSSHWrapper.py           # Python API 封装 (ctypes)
├── include/
│   └── simple_ssh.h              # C/C++ 主头文件
└── lib/
    ├── Winx64/                   # 64位开发库
    │   ├── simple_ssh.dll        # 核心库
    │   ├── simple_ssh.lib        # 导入库
    │   ├── Qt5Core.dll           # Qt5 依赖
    │   ├── ws2_32.dll            # 系统网络库
    │   └── msvcp140.dll...       # VC++ 运行环境
    └── Winx86/                   # 32位开发库
        ├── simple_ssh.dll
        ├── simple_ssh.lib
        └── ...
```

> **注意**：运行时请确保 `lib` 目录下的所有 DLL 文件与您的应用程序位于同一目录。

## 🚀 快速开始

### 1. Python 交互式测试 (推荐)

SimpleSSH 附带了一个功能强大的交互式测试工具，可用于快速验证连接和功能。

**启动方式**：

```bash
# 64位环境
py -3-64 SimpleSSHTest.py

# 32位环境
py -3-32 SimpleSSHTest.py
```

**常用命令演示**：

```bash
# 1. 建立连接 (参数前使用双杠 --)
SimpleSSH >>> --connect --host 192.168.1.120 --user root --pass 123456

# 2. 执行命令 (同步等待结果)
SimpleSSH >>> --exec -id 1 -cmd "ls -la /home"

# 3. 上传文件 (支持智能路径，远程写目录即可)
SimpleSSH >>> --upload -id 1 -local D:\firmware.bin -remote /tmp

# 4. 异步执行 (如 ping) 并读取输出
SimpleSSH >>> --start -id 1 -cmd "ping 8.8.8.8"
SimpleSSH >>> --read -id 1 -timeout 200
SimpleSSH >>> --stop -id 1

# 5. 断开连接
SimpleSSH >>> --close -id 1
```

### 2. C/C++ 开发集成

**引用配置**：

```cpp
#include "simple_ssh.h"
#pragma comment(lib, "simple_ssh.lib")
```

**代码示例**：

```cpp
#include <stdio.h>
#include "simple_ssh.h"

int main() {
    // 1. 初始化日志
    SimpleSSHOpenLog("ssh_client.log", 1, 5, 10);

    // 2. 建立连接 (返回 ID > 0 即成功)
    // 支持指定超时时间和本地网卡IP
    const char* config = "--host 192.168.1.100 --user root --pass 123456 --timeout 3000";
    int id = SimpleSSHConnect(config);

    if (id > 0) {
        printf("连接成功，ID: %d\n", id);

        // 3. 执行命令
        char buffer[4096] = {0};
        int exitCode = 0;
        int ret = SimpleSSHExecuteCmd(id, "uname -a", buffer, sizeof(buffer), &exitCode, 2000);
        
        if (ret == SIMPLE_SSH_SUCCESS) {
            printf("系统信息: %s\n", buffer);
        }

        // 4. 上传文件
        SimpleSSHUploadFile(id, "D:\\test.txt", "/tmp/test.txt");

        // 5. 断开连接
        SimpleSSHClose(id);
    } else {
        printf("连接失败，错误码: %d\n", id);
    }

    SimpleSSHCloseLog();
    return 0;
}
```

## ⚙️ 连接配置详解

`SimpleSSHConnect` 函数采用命令行风格的字符串进行配置，支持以下参数：

| 参数类别 | 参数名          | 说明                                 | 示例/默认值     |
| :------- | :-------------- | :----------------------------------- | :-------------- |
| **必需** | `--host`        | 目标服务器 IP 或主机名               | `192.168.1.100` |
| **必需** | `--user`        | SSH 登录用户名                       | `root`          |
| **必需** | `--pass`        | SSH 登录密码                         | `123456`        |
| 可选     | `--port`        | SSH 端口号                           | 默认 `22`       |
| 可选     | `--localIp`     | **绑定本地网卡 IP** (多网卡环境必选) | 系统自动路由    |
| 可选     | `--timeout`     | 连接超时时间 (毫秒)                  | 默认 `5000`     |
| 可选     | `--crypto`      | 加密策略 (0:默认, 1:兼容, 2:旧版)    | 默认 `0`        |
| 可选     | `--compression` | 启用 ZLIB 压缩 (0:禁用, 1:启用)      | 默认 `1`        |

> **提示**：如果您的电脑连接了多个网络（如同时连接内网和外网），建议使用 `--localIp` 指定与目标设备同网段的本机 IP，以确保连接稳定性。

## 📂 SFTP 文件传输说明

API 内部实现了智能路径处理，以减少常见错误：

1.  **上传 (`SimpleSSHUploadFile`)**：
    *   **场景 A**：远程路径是**目录** (如 `/tmp`)。
        *   行为：自动拼接本地文件名 -> 上传为 `/tmp/local_filename.ext`。
    *   **场景 B**：远程路径是**文件** (如 `/tmp/new_name.log`)。
        *   行为：直接写入该文件，若存在则覆盖。
2.  **下载 (`SimpleSSHDownloadFile`)**：
    *   必须指定精确的远程文件路径。

## 🔍 状态码速查

| 状态码 | 宏定义                               | 说明                        |
| :----- | :----------------------------------- | :-------------------------- |
| `0`    | `SIMPLE_SSH_SUCCESS`                 | 操作成功                    |
| `-4`   | `SIMPLE_SSH_ERROR_TIMEOUT`           | 操作超时                    |
| `-10`  | `SIMPLE_SSH_ERROR_CONNECTION_FAILED` | TCP 连接失败 (检查IP/端口)  |
| `-11`  | `SIMPLE_SSH_ERROR_AUTHENTICATION`    | 认证失败 (检查账号密码)     |
| `-30`  | `SIMPLE_SSH_ERROR_SFTP_FAILURE`      | SFTP 初始化失败             |
| `-31`  | `SIMPLE_SSH_ERROR_SFTP_OPEN_FAILED`  | 无法打开远程文件 (检查权限) |
| `-100` | `SIMPLE_SSH_STATUS_READ_EMPTY`       | 异步读取无数据 (正常状态)   |

## ❓ 常见问题 (FAQ)

**Q1: 运行程序提示找不到 DLL？**
> **A**: 请确保 `lib/Winx64` (或 x86) 目录下的所有文件（包括 `Qt5Core.dll`, `msvcp140.dll` 等）都已拷贝到您的可执行文件同级目录。

**Q2: 连接总是超时 (-4) 或失败 (-10)？**
> **A**: 
> 1. 检查目标 IP 是否能 Ping 通。
> 2. 检查目标 22 端口是否开放。
> 3. 如果本机有多张网卡，尝试在连接字符串中添加 `--localIp <本机IP>`。

**Q3: 上传文件报错 SFTP_FAILURE？**
> **A**: 检查远程目录是否有写权限。新版本 API 已支持直接写目录路径（如 `/tmp`），但建议尽量指定完整路径以避免歧义。

**Q4: 如何获取更详细的错误信息？**
> **A**: 调用 `SimpleSSHOpenLog` 开启日志，日志级别设为 0 (DEBUG)，查看生成的日志文件即可定位底层协议错误。

- **邮箱**: ctrlfrmb@gmail.com

## 📄 版权声明

版权所有 © 2025 leiwei。保留所有权利。  

根据MIT许可证获得许可；
您可以在以下网址获取许可证副本：
https://opensource.org/licenses/MIT

本项目使用了libssh2库(https://www.libssh2.org)，
该库遵循BSD风格许可证。
