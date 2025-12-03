 # FKVCI API 说明文档

> 版本：v3.0.0 | 作者：leiwei | 日期：2025-10-20

## 📖 简介

FKVCI API 提供用于 CAN/LIN 车辆网络通信的全面接口。支持设备管理、通道配置、消息传输和接收，并包含满足高级协议要求的特殊功能。

**主要特点**：

- 支持 CAN 2.0 和 CAN FD 协议
- 支持 LIN 主从模式
- CAN高级功能配置
- **设备扫描与发现**
- 消息过滤功能
- 周期性消息传输
- 总线负载监控
- 完整的日志系统
- 多设备和多通道管理

## 📁 目录结构

```bash
fkvci_api/
├── README.pdf                     # 用户说明书
├── FKVciReleaseNote.txt           # 版本变更记录
├── FKVciMessage.py                # Python 消息结构定义
├── FKVciClient.py                 # Python API 封装
├── FKVciClientTest.py             # Python 交互式测试工具
├── FKVciCircleTest.py             # Python 循环测试工具
├── include/
│   ├── fkvciapi.h                 # C/C++ 主头文件
│   └── fkvcimessage.h             # C/C++ 消息定义头文件
└── lib/
    ├── Winx64/                    # 64位库文件
    │   ├── fkvci_api.dll
    │   ├── fkvci_api.lib
    │   └── ...依赖DLL
    └── Winx86/                    # 32位库文件
        ├── fkvci_api.dll
        ├── fkvci_api.lib
        └── ...依赖DLL
```

**注意：运行时需要拷贝依赖库，如common_api/Qt库**

## 🚀 快速开始

### Python测试（推荐新手）

1. **启动交互式测试工具**

```bash
python FKVciClientTest.py
# 64位python环境下运行：
py -3-64 FKVciClientTest.py
# 32位python环境下运行：
py -3-32 FKVciClientTest.py
```

2. **基本命令示例**

```bash
>>> --scan					       # 扫描在线设备
>>> --open -dev 0                  # 打开设备索引0
>>> --initCAN -dev 0 -ch 0 -baud 500000  # 初始化CAN通道(500kbps)
>>> --send -dev 0 -ch 0 -id 0x123 -data 11 22 33 44  # 发送CAN消息
>>> --recv -dev 0 -ch 0            # 接收CAN消息
>>> --autoRecvCAN on               # 启动自动接收
>>> --autoRecvCAN off              # 停止自动接收
>>> --resetCAN -dev 0 -ch 0        # 关闭CAN通道
>>> --help                         # 查看帮助
>>> --exit                         # 退出
```

 3.**设备自测**

`FKVciCircleTest.py` 是一个循环测试工具，主要用于测试 FKVCI 设备的稳定性和可靠性。它的主要功能如下：

1. **自动化循环测试** - 可以按指定的时间间隔重复执行设备开关和数据发送测试
2. **可配置测试参数** - 允许设置测试间隔、循环次数、设备号和通道号
3. **测试流程自动化** - 每个测试循环包括：打开设备 → 初始化CAN通道 → 发送CAN数据 → 关闭设备
4. **测试结果统计** - 实时显示测试进度、成功率、失败次数和运行时间
5. **详细测试报告** - 测试完成后提供摘要报告，包括总测试次数、成功率和平均耗时等

这个工具对于验证设备在频繁开关和通信过程中的稳定性非常有用，特别是在产品发布前的压力测试阶段。可以通过命令行选项灵活控制测试参数，适合长时间运行以发现潜在的稳定性问题。

### C/C++开发

1. **包含头文件**

```cpp
#include "fkvciapi.h"
#pragma comment(lib, "fkvci_api.lib")
```

2. **基本使用示例**

```cpp
// 打开设备
int result = FkVciOpenDev(0, 0, 0);  // 打开设备索引0

if (result == 0) {
    // 初始化CAN通道(500kbps)
    FkVciInitCAN(0, 0, 500000);
    
    // 发送CAN消息
    FkVciCanDataType msg = {0};
    msg.CanID = 0x100;
    msg.DLC = 8;
    for (int i = 0; i < 8; i++) msg.Data[i] = i;
    FkVciTransmitCAN(0, 0, &msg, 1);
    
    // 接收CAN消息
    FkVciCanDataType rxMsgs[10];
    UINT32_T msgCount = 10;
    FkVciReceiveCAN(0, 0, rxMsgs, msgCount, 1000);  // 最多等待1秒
    
    // 关闭设备
    FkVciCloseDev(0);
}
```

3. **CANFD高级配置示例**

```cpp
// 使用高级配置结构体初始化CANFD
FkVciCanFdConfig canfdConfig = {0};
canfdConfig.baudRate = 500000;               // 标称波特率500kbps
canfdConfig.fdBaudRate = 2000000;            // 数据波特率2Mbps
canfdConfig.nSeg1 = 0x1F;                    // 标称位时间段1参数
canfdConfig.nSeg2 = 0x08;                    // 标称位时间段2参数
canfdConfig.dSeg1 = 0x0F;                    // 数据位时间段1参数
canfdConfig.dSeg2 = 0x04;                    // 数据位时间段2参数
canfdConfig.terminalResistorEnabled = 1;     // 启用终端电阻

// 使用高级配置初始化CANFD通道
FkVciInitCANFDAdvanced(0, 0, &canfdConfig);
```

## 🧰 日志管理

日志系统用于调试和故障诊断，一般情况下不需要启用。

```cpp
// 启用日志
FkVciOpenLog("logs/fkvci.log", 1, 10, 10);

// 执行操作...

// 关闭日志
FkVciCloseLog();
```

## 🔄 CAN 通信

### 基本功能

```cpp
// 初始化CAN通道
FkVciInitCAN(0, 0, 500000);  // 设备0，通道0，500kbps

// 发送CAN消息
FkVciCanDataType msg = {0};
msg.CanID = 0x100;
msg.DLC = 8;
for (int i = 0; i < 8; i++) msg.Data[i] = i;
FkVciTransmitCAN(0, 0, &msg, 1);

// 接收CAN消息
FkVciCanDataType rxMsgs[10];
UINT32_T msgCount = 10;
FkVciReceiveCAN(0, 0, rxMsgs, msgCount, 1000);  // 最多等待1秒
```

### 周期性发送

```cpp
// 开始周期性发送
int periodId = FkVciStartPeriodCAN(0, 0, &msg, 100);  // 每100毫秒

// 停止周期性发送
FkVciStopPeriodCAN(0, 0, periodId);
```

### 总线负载监控

```cpp
// 获取总线负载率
double busLoad[4] = {0};  // 假设最多4个通道
FkVciGetBusLoadCAN(0, busLoad, 4);
printf("CAN1 负载率: %.2f%%\n", busLoad[0]);
```

## 🔄 LIN 通信

### 基本功能

```cpp
// 以主模式初始化LIN通道
FkVciInitLIN(0, 1, 0, 19200);  // 设备0，通道1，主模式，19200bps

// 发送LIN消息
FkVciLinDataType linMsg = {0};
linMsg.LinID = 0x10;
linMsg.DLC = 8;
linMsg.CheckType = 1;  // 增强型校验和
linMsg.MsgType = 1;    // 主写入
for (int i = 0; i < 8; i++) linMsg.Data[i] = i;
FkVciTransmitLIN(0, 1, &linMsg, 1);

// 接收LIN消息
FkVciLinDataType rxLinMsgs[10];
UINT32_T linMsgCount = 10;
FkVciReceiveLIN(0, 1, rxLinMsgs, linMsgCount, 1000);  // 最多等待1秒
```

## 🛠️ 进阶功能

### 扫描设备

```cpp
int device_list[16];
UINT8_T num_found = 16;
int result = FkVciScanDevice(device_list, &num_found, 500);
if (result == 0) {
    printf("扫描到 %d 个设备。\n", num_found);
}
```

### CAN消息过滤

```cpp
// 设置消息ID过滤
UINT32_T filterIds[3] = {0x100, 0x200, 0x300};
FkVciStartFilterCAN(0, 0, filterIds, 3);

// 取消过滤
FkVciStopFilterCAN(0, 0);
```

### 终端电阻控制

```cpp
// 启用终端电阻
FkVciSetTerminalResistorCAN(0, 0, 1);

// 禁用终端电阻
FkVciSetTerminalResistorCAN(0, 0, 0);
```

### 设备版本和时间戳

```cpp
// 获取设备版本
UINT32_T version = FkVciGetVersion(0);
// 解析版本号(B0:version B1:year B2:month B3:day)
int ver = (version >> 24) & 0xFF;
int year = ((version >> 16) & 0xFF) + 2000;
int month = (version >> 8) & 0xFF;
int day = version & 0xFF;
printf("版本: %d.%04d.%02d.%02d\n", ver, year, month, day);

// 获取设备基准时间
UINT64_T baseTime = FkVciGetBaseTime(0);
```

## ❗ 注意事项

- **设备索引**：设备索引范围为0x00~0x0F，对应IP 192.168.201.130~192.168.201.145
- **本地网卡绑定**：通过`FkVciOpenDev`的`reserved`参数可以绑定本地网卡(192.168.201.reserved)
- **周期性发送**：启动周期性发送后需要记录返回的ID，用于后续停止
- **总线负载监控**：建议每1000ms调用一次，获取更稳定的负载值
- **缓冲区清理**：操作前建议使用`FkVciClearCAN`清空接收缓冲区
- **连接问题**：确保网络连接稳定，设备IP配置正确
- **终端电阻**：根据总线拓扑结构正确配置终端电阻

## 🔍 常见问题

| 问题               | 解决方案                                   |
| ------------------ | ------------------------------------------ |
| 设备打开失败       | 检查设备IP、网络连接和本地网卡配置         |
| 收不到CAN消息      | 检查波特率设置、终端电阻配置和过滤器设置   |
| CAN通道初始化失败  | 确认设备已正确打开，检查通道号和波特率参数 |
| 高速通信数据丢失   | 考虑增大接收缓冲区，降低数据发送频率       |
| LIN通信超时        | 检查LIN网络连接，确认主从模式设置正确      |
| 周期性发送停止失败 | 确保使用正确的周期ID                       |
| 总线负载率异常     | 确认总线上的通信活动，检查波特率设置       |

## 📞 技术支持

- **邮箱**: wei.lei@figkey.com
- **官网**: https://www.figkey.com

## 📄 版权声明

版权所有 © 2025 丰柯科技。保留所有权利。  
