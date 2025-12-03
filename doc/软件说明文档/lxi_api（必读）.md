### LXI 数据采集卡 API 快速入门指南

> **版本: v2.1.1** | 日期: 2025-11-03

## 📖 简介

本指南旨在帮助您快速上手 LXI 数据采集卡的软件开发套件 (SDK)。此版本 API 提供了全新的、基于参数化字符串的 C 语言接口，让设备控制和数据读取变得前所未有的简单和高效。

**核心特性:**

*   **现代化接口**: 使用 `--key value` 格式的字符串配置连接和采集，告别复杂的十六进制命令。
*   **内置电压转换**: API 直接输出浮点电压值，无需手动解析。
*   **灵活的数据读取**: 支持**阻塞**和**非阻塞**两种数据读取模式，满足不同应用场景需求。
*   **高性能与跨平台**: 内置高性能数据队列，支持 Windows 和 Linux，并提供完整的依赖库。
*   **易于集成**: 提供清晰的 C 语言头文件和开箱即用的 Python 测试脚本。

> **⚠️ 性能警告**
>
> **请仅在调试时开启日志功能！**
> 高速采集时，开启日志会严重影响性能，可能导致丢包，并快速消耗磁盘空间。

## 📦 API 文件结构说明

```
D:.
│  LxiClient.py         # Python API 封装类
│  LxiClientTest.py     # Python 命令行测试工具
│  LxiMessage.py        # Python 数据结构定义
│
├─include
│      lxiapi.h         # C/C++ API 头文件 (包含函数声明和详细参数说明)
│      lximessage.h     # C/C++ 数据结构定义
│
└─lib
    ├─Winx64            # 64位 Windows 库
    │      lxi_api.dll
    │      lxi_api.lib
    │      ... (所有依赖的.dll文件)
    │
    └─Winx86            # 32位 Windows 库
           ... (文件同上)
```

## 🚀 快速开始 (三步完成)

### 步骤 1: 硬件与网络配置

1.  **硬件连接**: 使用网线将 LXI 设备连接到您的电脑或局域网。
2.  **配置电脑 IP**: 为电脑网口设置一个与 LXI 设备同网段的静态 IP。
    *   **设备默认 IP**: `192.168.201.X` (其中 `X` 是卡槽位置, 如 `192.168.201.0`)
    *   **电脑建议 IP**: `192.168.201.100`
    *   **子网掩码**: `255.255.255.0`
3.  **网络测试**: 打开 CMD 执行 `ping 192.168.201.0` (替换为您的设备IP)，收到回复即表示网络配置成功。

### 步骤 2: Python 快速测试 (推荐)

这是验证 API 功能最快的方式。

1. **运行**: 确保已安装 Python 3，然后在 API 根目录打开终端，运行测试工具。

   ```bash
   python LxiClientTest.py
   ```

2. **测试**: 在 `LXI>` 提示符后，依次输入以下命令：

   ```bash
   # 1. 连接设备 (IP替换为您设备的实际IP)
   connect --serverIp 192.168.201.0
   
   # 2. 启动高速单端采集 (持续采集，不指定 slotId，将自动从IP推断)
   start --rateMode high --channelMode single --channels 1 9 16 24 31 39
   
   # 3. 自动读取数据 (将持续打印接收到的数据)
   autoRead
   ```

   当您看到 `[AutoRead] -> Index=..., SN=..., ...` 格式的数据持续输出时，证明整个系统工作正常。
   使用 `stop` 和 `disconnect` 命令可停止采集和断开连接。

### 步骤 3: C/C++ 集成

1. **项目配置 (Visual Studio)**:

   *   **包含目录**: 添加 `include` 目录。
   *   **库目录**: 添加 `lib/Winx64` (或 `Winx86`) 目录。
   *   **链接器输入**: 添加 `lxi_api.lib`。

2. **示例代码**: 

   ```cpp
   #include "lxiapi.h"
   #include <stdio.h>
   #ifdef _WIN32
   #include <windows.h>
   #define sleep_ms(ms) Sleep(ms)
   #else
   #include <unistd.h>
   #define sleep_ms(ms) usleep(ms * 1000)
   #endif
   
   int main() {
       // 1. 连接设备
       const char* connect_params = "--serverIp 192.168.201.0";
       int instanceId = LXIConnect(connect_params);
   
       if (instanceId <= 0) {
           printf("设备连接失败，错误码: %d\n", instanceId);
           return -1;
       }
       printf("设备连接成功，实例ID: %d\n", instanceId);
   
       // 2. 启动采集 (高速差分，不指定slotId，API将从IP地址自动推断)
       const char* start_params = "--rateMode high --channelMode diff --channels 1 9 16 24 31 39";
       int result = LXIAIStartSampling(instanceId, start_params);
       if (result != LXI_RESULT_OK) {
           printf("启动采集失败，错误码: %d\n", result);
           LXIDisconnect(instanceId);
           return -1;
       }
       printf("开始采集命令已发送。\n");
   
       // (可选) 获取并打印采样信息
       LxiSamplingInfo info;
       if (LXIAIGetSamplingInfo(instanceId, &info) == LXI_RESULT_OK) {
           printf("采样信息: %d 个通道, 每个通道 %d 个点。\n", info.channel_count, info.sample_count_per_channel);
       }
   
       // 3. 循环读取数据 (使用阻塞模式)
       LxiVoltagePacket voltage_buffer[200];
       printf("开始循环读取数据 (持续约5秒)...\n");
       for (int i = 0; i < 50; ++i) { // 循环50次，每次等待最多100ms
           int packets_to_read = 200; // 传入缓冲区大小
           
           // 使用阻塞模式读取，最多等待100ms
           int read_result = LXIAIReadVoltages(instanceId, voltage_buffer, &packets_to_read, 100);
           
           if (read_result == LXI_RESULT_OK) {
               if (packets_to_read > 0) { // 检查实际读取到的数量
                   printf("成功读取 %d 个包. 第一个包: SN=%u, LastSN=%u, V[0]=%.4fV\n",
                          packets_to_read, voltage_buffer[0].serial_code, voltage_buffer[0].last_serial_code, voltage_buffer[0].voltages[0]);
               }
           } else if (read_result == LXI_RESULT_READ_TIMEOUT) {
                // 正常情况：在100ms内没有新数据到达
                printf("读取超时，队列中无新数据。\n");
           } else {
               printf("读取数据时发生严重错误，错误码: %d\n", read_result);
               break;
           }
       }
   
       // 4. 停止并断开
       LXIAIStopSampling(instanceId, "--timeout 100"); // 建议同步等待以确保命令被执行
       printf("停止采集命令已发送。\n");
       LXIDisconnect(instanceId);
       printf("设备已断开。\n");
   
       return 0;
   }
   ```

3. **部署**: 编译后，将 `lib` 目录下的**所有 `.dll` 文件**复制到您程序 `.exe` 所在的目录。

## ⚙️ API 核心函数速查

| 函数名                       | 功能描述                                                     |
| :--------------------------- | :----------------------------------------------------------- |
| `LXIConnect`                 | 连接设备。成功返回大于0的实例ID，失败返回负数错误码。<br>**核心参数**: `--serverIp`, `--serverPort`。 |
| `LXIAIStartSampling`         | 启动数据采集。成功返回 `LXI_RESULT_OK` (0)。<br>**核心参数**: `--rateMode`, `--channelMode`。 **可选参数**: `--slotId`。 |
| `LXIAIStopSampling`          | 停止数据采集。成功返回 `LXI_RESULT_OK` (0)。                 |
| `LXIAIGetSamplingInfo`       | 获取当前采集的通道数和每通道点数，用于解析数据。             |
| `LXIAIReadVoltages`          | **(核心)** 读取数据。支持阻塞和非阻塞模式。<br>**`timeout_ms > 0`**: 阻塞读取，直到有数据或超时。<br>**`timeout_ms <= 0`**: 非阻塞读取，立即返回。 |
| `LXIAIReadResponse`          | 读取命令响应。**返回状态码**，实际读取的字节数通过指针参数返回。 |
| `LXIDisconnect`              | 断开连接并释放资源。                                         |
| `LXIClearAll`                | 强制断开所有连接。                                           |
| `LXIOpenLog` / `LXICloseLog` | (可选) 打开和关闭日志文件，仅用于调试。                      |

> **详细参数说明**：所有函数的详细参数（如量程、采样率等）都已在 `lxiapi.h` 头文件中有清晰的注释。

## 🔍 常见问题 (FAQ)

| 问题                                                         | 可能原因与解决方案                                           |
| :----------------------------------------------------------- | :----------------------------------------------------------- |
| **连接失败 (`LXIConnect` 返回负数)**                         | 1. **网络不通**: 检查 IP 地址，`ping` 设备 IP 确认连通性。 <br> 2. **防火墙**: 临时关闭 Windows 防火墙或杀毒软件再试。 |
| **启动采集后读不到数据**                                     | 1. **参数错误**: 检查 `LXIAIStartSampling` 的参数，特别是 `--rateMode`, `--channelMode` 和 `--channels` 是否按 `lxiapi.h` 的规则填写。 <br> 2. **设备状态**: 检查设备指示灯，确认其是否处于数据发送状态。 |
| **调用 `LXIAIReadVoltages` 总是返回 `LXI_RESULT_READ_TIMEOUT`** | 1. **超时时间过短**: 尝试增加 `timeout_ms` 的值。 <br> 2. **无数据产生**: 确认 `LXIAIStartSampling` 是否已成功调用，并且设备确实在发送数据。 <br> 3. **网络拥堵或丢包**: 在高数据率下，网络问题可能导致数据延迟或丢失。 |
| **Python 脚本提示 "加载 dll 失败"**                          | 1. **架构不匹配**: 确认 Python 是 64 位还是 32 位，脚本会自动加载对应 `Winx64` 或 `Winx86` 目录的库。 <br> 2. **依赖缺失**: 确认 `lib` 目录下的所有 `.dll` 文件都完整无缺。 |

## 📞 技术支持

*   **邮箱**: wei.lei@figkey.com
*   **官网**: https://www.figkey.com

## 📄 版权声明

版权所有 © 2025 丰柯科技。保留所有权利。