### DoIP API 说明文档 (完整内容 - 已更新)

> 版本：v2.1.0 | 作者：leiwei | 日期：2025-10-11

## 📖 简介

DoIP API 提供用于 ISO 13400 标准定义的 DoIP (Diagnostics over Internet Protocol) 车辆诊断通信的客户端接口。该 API 允许客户端与支持 DoIP 协议的 ECU (电子控制单元) 建立基于 TCP/IP 的诊断通信连接，用于发送和接收诊断数据。

**主要功能**：

- 客户端连接管理（连接、断开、自动重连）
- 路由激活和诊断通道建立
- 常规和周期性诊断消息发送
- 消息接收和缓存管理
- 错误处理和日志记录
- 支持 UDS 诊断数据模式过滤

## 📁 目录结构

```bash
doip_api/
├── README.pdf                    # 用户说明书
├── DoIPClient.py                 # Python API 封装
├── DoIPClientMessage.py          # 消息类型定义和常量
├── DoIPClientTest.py             # Python 客户端测试工具
├── DoIPReleaseNote.txt           # 版本变更记录
├── DoIPServer.py                 # Python 服务端模拟
├── DoIPServerTest.py             # Python 服务端测试工具
├── DoIPUDS.py                    # UDS服务模拟实现
├── include/
│   ├── doipapi.h                 # C/C++ 主头文件
│   └── doipmessage.h             # 消息结构定义
└── lib/
    ├── Winx64/                   # 64位库文件
    │   ├── doip_api.dll
    │   ├── doip_api.lib
    │   └── 依赖DLL文件
    └── Winx86/                   # 32位库文件
        ├── doip_api.dll
        ├── doip_api.lib
        └── 依赖DLL文件
```

注意：运行时需要拷贝依赖库，如common_api、Qt库

## 🚀 快速开始

### 客户端测试工具

1. **启动测试工具**

```bash
python DoIPClientTest.py
# 64位python环境下运行：
py -3-64 DoIPClientTest.py
# 32位python环境下运行：
py -3-32 DoIPClientTest.py
```

2. **基本命令示例**

```bash
>>> --connect -serverIP 192.168.1.100 -source 0x0E80 -target 0x0E01
>>> --send -msg 10 01                   # 发送诊断消息 (启动会话)
>>> --read                              # 读取响应
>>> --addPeriod -time 1000 -msg 3E 80   # 添加周期性消息
>>> --delPeriod -id 1                   # 停止周期消息
>>> --disconnect                        # 断开连接
```

3. **UDS 27 安全访问示例**

```bash
>>> --uds27                             # 执行标准安全访问序列
>>> --uds27 -first 01 -second 02 11 22 33 44  # 自定义安全访问序列
```

### 服务端测试工具

```bash
python DoIPServerTest.py --server-address 0x0E01

>>> setdid 0xF190 0102030405060708     # 设置DID数据
>>> clients                            # 查看已连接客户端
>>> status                             # 查看服务器状态
```

服务端模拟功能：

- UDS服务支持（会话控制、安全访问、DID读写等）
- 自动保活检测机制
- 多客户端并发连接管理
- 可配置DID数据和响应行为

## 💻 C/C++ 开发

```cpp
#include "doipapi.h"
#include "doipmessage.h"
#pragma comment(lib, "doip_api.lib")

// 建立连接
const char* connectCommand = 
    "--serverIp 192.168.1.100 --sourceAddress 0x0E80 --targetAddress 0x0E01";
int instanceId = 0;
DoIPClientConnect(connectCommand, &instanceId);

// 发送诊断消息
unsigned char diagData[] = {0x10, 0x01};
DoIPClientSend(instanceId, diagData, sizeof(diagData));

// 读取响应
DoIPOutputMessage messages[10];
int actualCount = 0;
DoIPClientRead(instanceId, messages, 10, &actualCount);

// 关闭连接
DoIPClientDisconnect(instanceId);
```

## 🔄 连接配置

| 必需参数          | 说明           | 示例                       |
| ----------------- | -------------- | -------------------------- |
| `--serverIp`      | 服务器IP地址   | `--serverIp 192.168.1.100` |
| `--sourceAddress` | 客户端逻辑地址 | `--sourceAddress 0x0E80`   |
| `--targetAddress` | 目标ECU地址    | `--targetAddress 0x0E01`   |

| 可选参数              | 说明                                                         | 默认值 |
| --------------------- | ------------------------------------------------------------ | ------ |
| `--bindLocalIp`       | **绑定本地网卡IP。当PC有多张网卡时，建议指定此参数以确保连接稳定性。** | 空     |
| `--serverTcpPort`     | 服务器TCP端口                                                | 13400  |
| `--routingActivation` | 是否需要路由激活                                             | true   |
| `--timeout`           | 连接超时时间(毫秒)                                           | 2000   |

## 🧰 日志管理

```cpp
// 启用日志
DoIPOpenLog("logs/doip.log", 0, 5, 10); // Debug级别，5MB大小，10个文件

// 执行操作...

// 关闭日志
DoIPCloseLog();
```

## ⚙️ 高级功能

- **UDS模式读取**：`DoIPClientSetReadUDS(instanceId, 1)`
- **自动重连控制**：`DoIPClientSetReconnect(instanceId, 0)`
- **周期性诊断**：用于保持会话或定期监控
- **消息缓存清理**：`DoIPClientClearMessages(instanceId)`

## 🔍 常见问题

| 问题           | 解决方案                                                     |
| -------------- | ------------------------------------------------------------ |
| 连接建立失败   | 1. 检查IP地址、端口号和网络连接状态<br/>2. **如果PC有多张网卡，尝试使用 `--bindLocalIp` 参数指定与目标ECU在同一网段的本地IP地址。** |
| 路由激活失败   | 验证源地址和目标地址是否有效                                 |
| 诊断请求无响应 | 检查目标地址，可能需要增加超时时间                           |
| 会话断开       | 添加周期性 3E 保持会话消息                                   |

## ❗ 注意事项

- 逻辑地址应符合ISO 13400规范，通常为16位十六进制值
- 周期任务必须保存ID以便后续停止
- 确保在应用退出前释放资源
- 运行时依赖Qt库和其他DLL文件

## 📞 技术支持

- **邮箱**: ctrlfrmb@gmail.com

## 📄 版权声明

版权所有 © 2022 - 2042 leiwei。保留所有权利。

本软件根据 MIT 许可证发布；
您可以在以下网址获取许可证副本：
https://opensource.org/licenses/MIT

本软件是自由软件，您可以自由使用、修改和分发，
但需保留原作者版权信息和许可证声明。
