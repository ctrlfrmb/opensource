 # SSH API 说明文档

> 版本：v1.2.1 | 作者：leiwei | 日期：2025-06-09

## 📖 简介

SSH API 提供安全高效的远程命令执行接口，支持日志调试、状态管理等能力，适用于自动化远程运维和控制系统。基于libssh2开源库实现。

**主要功能**：

- 安全稳定的 SSH 连接
- 高效的命令发送与结果读取
- 多种连接方式（基本、高级、本地网卡绑定）
- 命令超时控制
- 日志记录支持
- 线程安全、支持多连接并发
- 跨平台支持（Windows/Linux）

## 📁 目录结构

```bash
ssh_api/
├── README.pdf                    # 用户说明书
├── SSHApiReleaseNote.txt         # 版本变更记录
├── SSHClient.py                  # Python API 封装
├── SSHClientTest.py              # Python 测试工具
├── include/
│   └── sshapi.h                  # C/C++ 头文件
└── lib/
    ├── Winx64/                   # 64位库文件
    │   ├── ssh_api.dll
    │   ├── ssh_api.lib
    └── Winx86/                   # 32位库文件
        ├── ssh_api.dll
        ├── ssh_api.lib
```

注意：运行时需要拷贝依赖库，如Qt/SSH库

## 🚀 快速开始

### Python测试（推荐新手）

1. **启动测试工具**

```bash
python SSHClientTest.py
# 64位python环境下运行：
py -3-64 SSHClientTest.py
# 32位python环境下运行：
py -3-32 SSHClientTest.py
```

2. **基本命令示例**

```bash
>>> --connect -serverIP 192.168.1.100 -port 22 -user root -pass password123        # 建立SSH连接
>>> --send "ls -la /home"          # 发送命令
>>> --read 5000                    # 指定超时读取命令结果
>>> --stop                         # 停止命令执行
>>> --disconnect                   # 断开连接
>>> --help                         # 查看帮助
>>> --exit                         # 退出
```

### C/C++开发

1. **包含头文件**

```cpp
#include "sshapi.h"
#pragma comment(lib, "ssh_api.lib")
```

2. **基本使用示例**

```cpp
int instance = SSHConnect("192.168.1.100", 22, "root", "password123");
if (instance > 0) {
    SSHSendCmd(instance, "ls -la /home");
    char buffer[4096] = {0};
    SSHReceiveCmdResult(instance, buffer, sizeof(buffer), 5000);
    SSHClose(instance);
}
```

3. **高级连接方式**

```cpp
int instance = SSHConnectAdvanced("--host 192.168.1.100 --port 22 --user root --pass password123 --timeout 8000");
```

## 🧰 日志管理

```cpp
ComOpenLog("logs/ssh_api.log", 1, 5, 10);  // 启用日志
// ...执行操作...
ComCloseLog();  // 关闭日志
```

## 🌐 常用命令示例

```bash
# 查看当前目录
ls -l

# 显示系统信息
uname -a

# 网络连通性检查
ping 192.168.1.100
```

## ❗ 注意事项

- **连接格式**：建议使用高级连接接口（命令格式），灵活配置参数
- **超时控制**：所有读取操作应设置合理超时时间，避免阻塞
- **日志开关**：默认关闭，调试时开启可排查问题
- **本地IP绑定**：支持指定网卡IP连接，适用于多网卡设备
- **兼容性要求**：推荐使用64位 Python + 64位 DLL 配合

## 🔍 常见问题

| 问题           | 解决方案                         |
| -------------- | -------------------------------- |
| 无法连接服务器 | 检查IP、端口、SSH服务是否开启    |
| 命令无响应     | 增加超时设置，确认命令是否正确   |
| DLL加载失败    | 确保DLL路径正确，安装VC++运行库  |
| 用户认证失败   | 检查用户名密码是否正确，有无权限 |

## 📞 技术支持

- **邮箱**: ctrlfrmb@gmail.com

## 📄 版权声明

版权所有 © 2025 leiwei。保留所有权利。  

根据MIT许可证获得许可；
您可以在以下网址获取许可证副本：
https://opensource.org/licenses/MIT

本项目使用了libssh2库(https://www.libssh2.org)，
该库遵循BSD风格许可证。
