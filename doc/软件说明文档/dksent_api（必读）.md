 # DkSent API 说明文档

> 版本：v2.0.0 | 作者：leiwei | 日期：2025-08-12

## 📖 简介

DkSent api 提供用于 SENT（Single Edge Nibble Transmission）协议通信的接口，支持汽车传感器数据的发送和接收。

**主要特点**：
- 支持 SENT 协议标准
- 双通道数据处理（通道0-1） 
- 可配置串口波特率
- 多种数据格式（12+12bit, 14+10bit, 16+8bit）
- 跨平台兼容（Windows/Linux）

## 📁 目录结构

```
dksent_api/
├── README（必读）.pdf
├── DKSentReleaseNote.txt
├── include/
│   ├── dksentapi.h
│   └── dksentdef.h
└── lib/
    ├── Winx64/                    # 64位库文件
    └── Winx86/                    # 32位库文件
```

**注意：运行时需要拷贝依赖库，如common_api/Qt库**

## 🚀 快速开始

### Python测试（推荐）

```bash
python DKSentClientTest.py
```

**基本命令：**
```bash
>>> --open -port COM1                          # 打开设备
>>> --init -ch 0 -convert 0 -format 0          # 初始化通道
>>> --write -ch 0 -data A B C D E F            # 写入数据
>>> --read                                     # 读取数据
>>> --close                                    # 关闭设备
>>> --help                                     # 查看帮助
```

### C/C++ 开发

```cpp
#include "dksentapi.h"
#pragma comment(lib, "dksent.lib")

// 打开设备
SHANDLE handle = FkDkSentOpenDev(eCOM, "COM1", 0);
// 或使用自定义波特率
SHANDLE handle = FkDkSentOpenDevWithBaud(eCOM, "COM1", 115200, 0);

if (handle) {
    // 初始化通道
    FkDkSentInit(handle, 0, 0, 0, 0x00);
    
    // 写入数据
    DkMsgWriteDataType data = {0, 0xA, 1, 2, 3, 4, 5, 6};
    FkDkSentWriteChanl(handle, 0, &data);
    
    // 读取数据
    DkMsgReadDataType ch0, ch1;
    FkDkSentRead(handle, &ch0, &ch1);
    
    // 清理资源
    FkDkSentDeInit(handle, 0);
    FkDkSentCloseDev(handle);
}
```

## 📝 主要接口

| 功能     | 接口                        | 说明                     |
| -------- | --------------------------- | ------------------------ |
| 设备管理 | `FkDkSentOpenDev()`         | 打开设备（默认波特率）   |
|          | `FkDkSentOpenDevWithBaud()` | 打开设备（自定义波特率） |
|          | `FkDkSentCloseDev()`        | 关闭设备                 |
| 通道配置 | `FkDkSentInit()`            | 初始化SENT通道           |
|          | `FkDkSentDeInit()`          | 关闭SENT通道             |
| 数据传输 | `FkDkSentWriteChanl()`      | 写入单通道数据           |
|          | `FkDkSentWriteChanlAll()`   | 写入双通道数据           |
|          | `FkDkSentRead()`            | 读取双通道数据           |
| 日志管理 | `ComOpenLog()`              | 打开日志                 |
|          | `ComCloseLog()`             | 关闭日志                 |

## ⚙️ 参数说明

**通道配置参数：**
- `convertMode`: 0=PC直接模式, 1=ADC转SENT模式
- `dataFormat`: 0=12+12bit, 1=14+10bit, 2=16+8bit
- `channelIndex`: 0-1（通道索引）

**常用波特率：**
9600, 19200, 38400, 57600, 115200, 256000, 512000, 921600, 1000000

## 🔍 错误码

| 错误码 | 含义         |
| ------ | ------------ |
| 0      | 操作成功     |
| 5      | 无效设备ID   |
| 7      | 操作失败     |
| 8      | 操作超时     |
| 11     | 设备打开失败 |

## ❗ 注意事项

- 使用前确保串口设备可用
- 根据硬件选择合适的波特率
- 操作完成后记得关闭设备和通道
- 调试时可启用日志记录

## 📞 技术支持

- **邮箱**: wei.lei@figkey.com
- **官网**: https://www.figkey.com

---
版权所有 © 2024 丰柯科技。保留所有权利。
