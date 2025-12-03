### WImage 高速图像采集 API 快速入门指南

> **版本: v1.1.0** | 日期: 2025-10-24

## 📖 简介

本指南旨在帮助您快速上手 **WImage 高速图像采集 API v1.1.0** 的软件开发套件 (SDK)。该 SDK 提供了一套高性能的 C 语言 API，用于通过以太网（UDP协议）与 WImage 设备进行通信，实现高速图像采集的控制与数据存储。

**核心特性**:

-   **现代化接口**: 使用 `--key value` 格式的参数化字符串来配置连接。
-   **高性能接收**: 内部采用高性能队列和内存池，最大限度地减少网络丢包。
-   **自动文件存储**: API 自动将接收到的完整图像帧数据保存到本地文件。
-   **跨平台支持**: 提供 Windows (32/64位) 和 Linux 的动态链接库。
-   **易于集成**: 提供清晰的 C 语言头文件和开箱即用的 Python 测试脚本。

> **⚠️ 性能警告**
>
> **请仅在调试时开启日志功能！**
> 高速图像采集会产生海量数据，开启日志会严重影响性能，可能导致丢包，并快速消耗磁盘空间。

## 📦 API 文件结构说明

```
D:.
│  WImageClient.py      # Python API 封装类
│  WImageClientTest.py  # Python 命令行测试工具
│  WImageMessage.py     # Python 数据结构定义
│
├─include
│      wimageapi.h      # C/C++ API 头文件 (包含函数声明和参数说明)
│      wimagemessage.h  # C/C++ 数据结构和错误码定义
│
└─lib
    ├─Winx64            # 64位 Windows 库
    │      wimage_api.dll
    │      wimage_api.lib
    │      ... (所有依赖的.dll文件)
    │
    └─Winx86            # 32位 Windows 库
           ... (文件同上)
```

## 🚀 快速开始 (三步完成)

### 步骤 1: 硬件与网络配置

1.  **硬件连接**: 使用网线将 WImage 设备连接到您的电脑或局域网。
2.  **配置电脑 IP**: 为电脑网口设置一个与 WImage 设备同网段的静态 IP。
    *   **设备 IP 示例**: `192.168.1.10`
    *   **电脑建议 IP**: `192.168.1.100`
    *   **子网掩码**: `255.255.255.0`
3.  **网络测试**: 打开 CMD 执行 `ping <设备IP>`，收到回复即表示网络配置成功。

### 步骤 2: Python 快速测试 (推荐)

这是验证 API 功能最快的方式。

1. **运行**: 确保已安装 Python 3，然后在 API 根目录打开终端，运行测试工具。

   ```bash
   python WImageClientTest.py
   ```

2. **测试**: 在 `WImage>` 提示符后，依次输入以下命令：

   ```bash
   # 1. 连接设备 (IP和保存路径替换为您自己的配置)
   connect --serverIp 192.168.1.10 --savePath ./captured_images
   
   # 2. 启动采集 (Run1 为示例命令)
   start Run1
   
   # (等待一段时间)
   
   # 3. 停止采集 (RESET 为示例命令)
   stop RESET
   ```

   当您看到命令成功执行，并且在 `./captured_images` 目录下开始出现 `image_XXXXXX.bin` 文件时，证明整个系统工作正常。

### 步骤 3: C/C++ 集成

1. **项目配置 (Visual Studio)**:

   *   **包含目录**: 添加 `include` 目录。
   *   **库目录**: 添加 `lib/Winx64` (或 `Winx86`) 目录。
   *   **链接器输入**: 添加 `wimage_api.lib`。

2. **示例代码**: 

   ```cpp
   #include "wimageapi.h"
   #include "wimagemessage.h" // 包含消息定义以便使用 WIMAGE_RESULT_OK
   #include <stdio.h>
   #ifdef _WIN32
   #include <windows.h>
   #define sleep_ms(ms) Sleep(ms)
   #else
   #include <unistd.h>
   #define sleep_ms(ms) usleep(ms * 1000)
   #endif
   
   int main() {
       // 1. (可选) 打开日志
       WImageOpenLog("logs/wimage_api_test.log", 1, 5, 10);
   
       // 2. 连接设备
       const char* connect_params = "--serverIp 192.168.1.10 --savePath ./captured_images";
       int instanceId = WImageConnect(connect_params);
   
       if (instanceId > 0) {
           printf("设备连接成功，实例ID: %d\n", instanceId);
   
           // 3. 发送开始采样命令
           const char* start_cmd = "Run1";
           int start_result = WImageStartSampling(instanceId, start_cmd); 
           if (start_result == WIMAGE_RESULT_OK) {
               printf("开始采样命令已发送。图像将保存到: ./captured_images\n");
   
               // 4. 模拟采集过程
               printf("正在采集图像 (等待5秒)...\n");
               sleep_ms(5000);
   
               // 5. 发送停止采样命令
               const char* stop_cmd = "RESET";
               int stop_result = WImageStopSampling(instanceId, stop_cmd); 
               if (stop_result == WIMAGE_RESULT_OK) {
                   printf("停止采样命令已发送。\n");
               } else {
                   printf("停止采样命令发送失败，错误码: %d\n", stop_result);
               }
           } else {
               printf("开始采样命令发送失败，错误码: %d\n", start_result);
           }
   
           // 6. 断开连接
           WImageDisconnect(instanceId);
           printf("设备已断开。\n");
       } else {
           printf("设备连接失败，错误码: %d\n", instanceId);
       }
   
       // 7. (可选) 关闭日志
       WImageCloseLog();
       return 0;
   }
   ```

3. **部署**: 编译后，将 `lib` 目录下的**所有 `.dll` 文件**复制到您程序 `.exe` 所在的目录。

## ⚙️ API 核心函数速查

| 函数名                | 功能描述                                                     |
| :-------------------- | :----------------------------------------------------------- |
| `WImageConnect`       | 连接设备。成功返回大于0的实例ID，失败返回负数错误码。<br>**核心参数**: `--serverIp`, `--savePath`。 |
| `WImageStartSampling` | 发送开始采样命令。成功返回 `WIMAGE_RESULT_OK` (0)。          |
| `WImageStopSampling`  | 发送停止采样命令。成功返回 `WIMAGE_RESULT_OK` (0)。          |
| `WImageDisconnect`    | 断开与指定实例的连接。                                       |
| `WImageClearAll`      | 断开所有连接并清理所有资源。                                 |
| `WImageOpenLog`       | (可选) 打开日志文件，仅用于调试。                            |
| `WImageCloseLog`      | (可选) 关闭日志系统。                                        |

> **详细参数说明**：所有函数的详细参数都已在 `wimageapi.h` 头文件中有清晰的注释。

## 🔍 常见问题 (FAQ)

| 问题                                                  | 可能原因与解决方案                                           |
| :---------------------------------------------------- | :----------------------------------------------------------- |
| **连接失败 (`WImageConnect` 返回负数)**               | 1. **网络不通**: 检查 IP 地址，`ping` 设备 IP 确认连通性。<br> 2. **防火墙**: 临时关闭 Windows 防火墙或杀毒软件再试。<br> 3. **参数缺失**: 确保 `--serverIp` 和 `--savePath` 这两个必需参数已提供。 |
| **已发送 `start` 命令，但 `savePath` 目录中没有文件** | 1. **命令错误**: 确认发送的开始命令字符串是否正确。<br> 2. **设备状态**: 检查设备指示灯，确认其是否处于数据发送状态。<br> 3. **路径权限**: 确认程序对 `--savePath` 指定的目录有写入权限。 |
| **Python 脚本提示 "加载 dll 失败"**                   | 1. **架构不匹配**: 确认 Python 是 64 位还是 32 位，脚本会自动加载对应 `Winx64` 或 `Winx86` 目录的库。<br> 2. **依赖缺失**: 确认 `lib` 目录下的所有 `.dll` 文件都完整无缺。 |

## 📞 技术支持

-   **邮箱**: wei.lei@figkey.com
-   **官网**: https://www.figkey.com

## 📄 版权声明

版权所有 © 2025 丰柯科技。保留所有权利。
